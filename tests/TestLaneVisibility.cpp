#include <catch2/catch.hpp>
#include <sstream>
#include "MusicGame/LaneVisibility.hpp"
#include "kson/IO/KshIO.hpp"
#include "kson/IO/KsonIO.hpp"

namespace
{
	kson::ChartData MakeLaneToggleChart()
	{
		kson::ChartData chart;
		chart.beat.bpm[0] = 120.0;
		chart.beat.timeSig[0] = { 4, 4 };
		return chart;
	}

	kson::ChartData LoadLaneToggleKsh()
	{
		std::istringstream stream{
			"title=Lane toggle test\n"
			"t=120\n"
			"ver=171\n"
			"--\n"
			"lane_toggle=50\n"
			"1000|00|--\n"
			"0000|00|--\n"
			"0000|00|--\n"
			"0000|00|--\n"
			"--\n"
			"lane_toggle=50\n"
			"0100|00|--\n"
			"0000|00|--\n"
			"0000|00|--\n"
			"0000|00|--\n"
			"--\n"
		};
		return kson::LoadKshChartData(stream);
	}
}

TEST_CASE("Charts without lane_toggle keep the lane visible", "[LaneVisibility]")
{
	const auto chart = MakeLaneToggleChart();
	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	REQUIRE(visibility.hiddenRatioAt(-2.0) == 0.0);
	REQUIRE(visibility.hiddenRatioAt(0.0) == 0.0);
	REQUIRE(visibility.hiddenRatioAt(1000.0) == 0.0);
}

TEST_CASE("KSH lane_toggle=50 hides and restores the lane in 50/192 whole notes", "[LaneVisibility]")
{
	const auto chart = LoadLaneToggleKsh();
	REQUIRE(chart.error == kson::ErrorType::None);
	REQUIRE(chart.compat.kshUnknown.option.at("lane_toggle").size() == 2);
	REQUIRE(chart.note.bt[0].size() == 1);
	REQUIRE(chart.note.bt[1].size() == 1);

	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	const double duration = 2.0 * 50.0 / 192.0;
	REQUIRE(visibility.hiddenRatioAt(-0.01) == 0.0);
	REQUIRE(visibility.hiddenRatioAt(0.0) == 0.0);
	REQUIRE(visibility.hiddenRatioAt(duration / 2) == Approx(0.5));
	REQUIRE(visibility.hiddenRatioAt(duration) == Approx(1.0));
	REQUIRE(visibility.hiddenRatioAt(2.0) == Approx(1.0));
	REQUIRE(visibility.hiddenRatioAt(2.0 + duration / 2) == Approx(0.5));
	REQUIRE(visibility.hiddenRatioAt(2.0 + duration) == Approx(0.0).margin(1e-12));
}

TEST_CASE("KSON round trips preserve lane toggle timing", "[LaneVisibility]")
{
	const auto original = LoadLaneToggleKsh();
	std::stringstream stream;
	REQUIRE(kson::SaveKsonChartData(stream, original) == kson::ErrorType::None);
	const auto chart = kson::LoadKsonChartData(stream);
	REQUIRE(chart.error == kson::ErrorType::None);
	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	REQUIRE(visibility.hiddenRatioAt(0.25) == Approx(0.48));
	REQUIRE(visibility.hiddenRatioAt(2.25) == Approx(0.52));
}

TEST_CASE("Zero duration toggles are immediate and remain finite", "[LaneVisibility]")
{
	auto chart = MakeLaneToggleChart();
	chart.compat.kshUnknown.option["lane_toggle"] = { { 240, "0" }, { 480, "0" } };
	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	REQUIRE(visibility.hiddenRatioAt(0.499) == 0.0);
	REQUIRE(visibility.hiddenRatioAt(0.5) == 1.0);
	REQUIRE(visibility.hiddenRatioAt(0.999) == 1.0);
	REQUIRE(visibility.hiddenRatioAt(1.0) == 0.0);
	REQUIRE(visibility.hiddenRatioAt(100.0) == 0.0);
}

TEST_CASE("Overlapping toggles reverse at the current visibility using the USC transition rate", "[LaneVisibility]")
{
	auto chart = MakeLaneToggleChart();
	chart.compat.kshUnknown.option["lane_toggle"] = { { 0, "192" }, { 240, "48" } };
	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	REQUIRE(visibility.hiddenRatioAt(0.5) == Approx(0.25));
	REQUIRE(visibility.hiddenRatioAt(0.5625) == Approx(0.125));
	REQUIRE(visibility.hiddenRatioAt(0.625) == Approx(0.0));
	REQUIRE(visibility.hiddenRatioAt(10.0) == 0.0);
}

TEST_CASE("Lane visibility can be queried after forward seeks, rewinds, and pauses", "[LaneVisibility]")
{
	const auto chart = LoadLaneToggleKsh();
	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	REQUIRE(visibility.hiddenRatioAt(100.0) == 0.0);
	REQUIRE(visibility.hiddenRatioAt(1.0) == 1.0);
	REQUIRE(visibility.hiddenRatioAt(0.25) == Approx(0.48));
	REQUIRE(visibility.hiddenRatioAt(0.25) == Approx(0.48));
	REQUIRE(visibility.hiddenRatioAt(-1.0) == 0.0);
}

TEST_CASE("Lane transition durations use the BPM at the event rather than the time signature", "[LaneVisibility]")
{
	auto chart = MakeLaneToggleChart();
	chart.beat.bpm[960] = 240.0;
	chart.beat.timeSig[1] = { 3, 4 };
	chart.compat.kshUnknown.option["lane_toggle"] = { { 960, "192" } };
	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	REQUIRE(visibility.hiddenRatioAt(2.0) == 0.0);
	REQUIRE(visibility.hiddenRatioAt(2.5) == Approx(0.5));
	REQUIRE(visibility.hiddenRatioAt(3.0) == 1.0);
}

TEST_CASE("BPM changes during a lane transition do not change its established duration", "[LaneVisibility]")
{
	auto chart = MakeLaneToggleChart();
	chart.beat.bpm[240] = 240.0;
	chart.compat.kshUnknown.option["lane_toggle"] = { { 0, "384" } };
	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	REQUIRE(visibility.hiddenRatioAt(2.0) == Approx(0.5));
	REQUIRE(visibility.hiddenRatioAt(4.0) == 1.0);
}

TEST_CASE("Playback speed scales lane toggle positions and durations with the chart", "[LaneVisibility]")
{
	auto chart = MakeLaneToggleChart();
	chart.beat.bpm[0] *= 2.0;
	chart.compat.kshUnknown.option["lane_toggle"] = { { 960, "50" } };
	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	REQUIRE(visibility.hiddenRatioAt(0.9) == 0.0);
	REQUIRE(visibility.hiddenRatioAt(1.0 + 25.0 / 192.0) == Approx(0.5));
}

TEST_CASE("Invalid lane toggle durations are ignored without changing toggle parity", "[LaneVisibility]")
{
	auto chart = MakeLaneToggleChart();
	chart.compat.kshUnknown.option["lane_toggle"] = {
		{ 0, "" }, { 0, "-1" }, { 0, "invalid" }, { 0, "12x" }, { 0, "50.5" }, { 0, "4294967296" }, { 960, "50" },
	};
	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	REQUIRE(visibility.hiddenRatioAt(1.0) == 0.0);
	REQUIRE(visibility.hiddenRatioAt(3.0) == 1.0);
}

TEST_CASE("Multiple toggles at the same chart row preserve their order", "[LaneVisibility]")
{
	auto chart = MakeLaneToggleChart();
	chart.compat.kshUnknown.option["lane_toggle"] = { { 0, "0" }, { 0, "192" } };
	const MusicGame::LaneVisibility visibility(chart, kson::CreateTimingCache(chart.beat));
	REQUIRE(visibility.hiddenRatioAt(0.0) == 1.0);
	REQUIRE(visibility.hiddenRatioAt(1.0) == Approx(0.5));
	REQUIRE(visibility.hiddenRatioAt(2.0) == 0.0);
}
