#pragma once
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include "kson/ChartData.hpp"
#include "kson/Util/TimingUtils.hpp"

namespace MusicGame
{
	/// @brief 192分音符単位の時間を指定する譜面トグルの共通タイムライン
	class ChartToggleTimeline
	{
	private:
		struct Transition
		{
			double timeSec;
			double value;
			double speed;

			[[nodiscard]]
			double valueAt(double currentTimeSec) const
			{
				return std::clamp(value + speed * (currentTimeSec - timeSec), 0.0, 1.0);
			}
		};

		std::vector<Transition> m_transitions;

		[[nodiscard]]
		static std::string_view Trim(std::string_view text)
		{
			constexpr std::string_view whitespace = " \t\r\n";
			const auto first = text.find_first_not_of(whitespace);
			if (first == std::string_view::npos)
			{
				return {};
			}
			return text.substr(first, text.find_last_not_of(whitespace) - first + 1);
		}

		[[nodiscard]]
		static std::optional<std::uint32_t> ParseDuration(std::string_view text)
		{
			text = Trim(text);
			if (text.empty())
			{
				return std::nullopt;
			}
			std::uint32_t duration = 0;
			const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), duration);
			if (error != std::errc{} || end != text.data() + text.size())
			{
				return std::nullopt;
			}
			return duration;
		}

		[[nodiscard]]
		static kson::ByPulseMulti<std::uint32_t> CollectCommands(const kson::ChartData& chartData, std::string_view optionName)
		{
			kson::ByPulseMulti<std::uint32_t> commands;
			std::set<kson::Pulse> explicitPulses;
			const auto optionItr = chartData.compat.kshUnknown.option.find(std::string{ optionName });
			if (optionItr != chartData.compat.kshUnknown.option.end())
			{
				for (const auto& [pulse, value] : optionItr->second)
				{
					if (const auto duration = ParseDuration(value))
					{
						commands.emplace(pulse, *duration);
						explicitPulses.insert(pulse);
					}
				}
			}

			// KSONのeditor.commentを行単位で読み取り、該当する命令だけを実行対象へ追加する。
			// 譜面データや元のコメント本文は変更しない。
			for (const auto& [pulse, comment] : chartData.editor.comment)
			{
				// 既存のオプションと同位置に併記されても、二重トグルしないよう明示指定を優先する。
				if (explicitPulses.contains(pulse))
				{
					continue;
				}

				std::string_view remaining{ comment };
				while (!remaining.empty())
				{
					const auto newline = remaining.find('\n');
					const auto line = Trim(remaining.substr(0, newline));
					const auto equals = line.find('=');
					if (equals != std::string_view::npos && Trim(line.substr(0, equals)) == optionName)
					{
						if (const auto duration = ParseDuration(line.substr(equals + 1)))
						{
							commands.emplace(pulse, *duration);
						}
					}
					if (newline == std::string_view::npos)
					{
						break;
					}
					remaining.remove_prefix(newline + 1);
				}
			}
			return commands;
		}

	public:
		ChartToggleTimeline(const kson::ChartData& chartData, const kson::TimingCache& timingCache, std::string_view optionName)
		{
			bool enabled = false;
			for (const auto& [pulse, duration] : CollectCommands(chartData, optionName))
			{
				const double timeSec = kson::PulseToSec(pulse, chartData.beat, timingCache);
				const double previousValue = valueAt(timeSec);
				enabled = !enabled;

				// USCのlane_toggleと同じく、イベント時点のBPMで全長の遷移速度を決定する。
				const double durationSec = 240.0 / kson::TempoAt(pulse, chartData.beat) * (duration / 192.0);
				m_transitions.push_back(Transition
				{
					.timeSec = timeSec,
					.value = duration == 0 ? (enabled ? 1.0 : 0.0) : previousValue,
					.speed = duration == 0 ? 0.0 : (enabled ? 1.0 : -1.0) / durationSec,
				});
			}
		}

		/// @brief 楽曲時刻から状態を取得する。シーク・一時停止・再開でも同じ状態を再現する。
		[[nodiscard]]
		double valueAt(double currentTimeSec) const
		{
			auto itr = std::upper_bound(m_transitions.begin(), m_transitions.end(), currentTimeSec,
				[](double timeSec, const Transition& transition) { return timeSec < transition.timeSec; });
			if (itr == m_transitions.begin())
			{
				return 0.0;
			}
			return (--itr)->valueAt(currentTimeSec);
		}
	};
}
