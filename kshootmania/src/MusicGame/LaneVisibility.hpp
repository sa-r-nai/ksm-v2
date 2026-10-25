#pragma once
#include "ChartToggleTimeline.hpp"

namespace MusicGame
{
	/// @brief USC互換のlane_toggleによるレーン非表示アニメーション
	class LaneVisibility
	{
	private:
		ChartToggleTimeline m_timeline;

	public:
		LaneVisibility(const kson::ChartData& chartData, const kson::TimingCache& timingCache)
			: m_timeline(chartData, timingCache, "lane_toggle")
		{
		}

		/// @brief 楽曲時刻から非表示割合を取得する。シークや一時停止でも状態を再現できる。
		[[nodiscard]]
		double hiddenRatioAt(double currentTimeSec) const
		{
			return m_timeline.valueAt(currentTimeSec);
		}
	};
}
