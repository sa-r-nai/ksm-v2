#include <catch2/catch.hpp>
#include <sstream>
#include "MusicGame/ChartToggleTimeline.hpp"
#include "MusicGame/LaneVisibility.hpp"
#include "kson/IO/KshIO.hpp"
#include "kson/IO/KsonIO.hpp"

namespace
{
	kson::ChartData LoadColorInvertKsh()
	{
		std::istringstream stream{
			"title=Color inversion test\nt=120\nver=171\n--\n"
			"color_invert=50\n1000|00|--\n0000|00|--\n0000|00|--\n0000|00|--\n--\n"
			"color_invert=50\n0100|00|--\n0000|00|--\n0000|00|--\n0000|00|--\n--\n"
		};
		return kson::LoadKshChartData(stream);
	}
}

TEST_CASE("Color inversion expands and contracts with the lane_toggle time unit", "[ColorInversion]")
{
	const auto chart = LoadColorInvertKsh();
	REQUIRE(chart.error == kson::ErrorType::None);
	const MusicGame::ChartToggleTimeline inversion(chart, kson::CreateTimingCache(chart.beat), "color_invert");
	const double duration = 2.0 * 50.0 / 192.0;
	REQUIRE(inversion.valueAt(-0.01) == 0.0);
	REQUIRE(inversion.valueAt(0.0) == 0.0);
	REQUIRE(inversion.valueAt(duration / 2) == Approx(0.5));
	REQUIRE(inversion.valueAt(duration) == Approx(1.0));
	REQUIRE(inversion.valueAt(2.0) == 1.0);
	REQUIRE(inversion.valueAt(2.0 + duration / 2) == Approx(0.5));
	REQUIRE(inversion.valueAt(2.0 + duration) == Approx(0.0).margin(1e-12));
	REQUIRE(chart.note.bt[0].size() == 1);
	REQUIRE(chart.note.bt[1].size() == 1);
}

TEST_CASE("Lane hiding and color inversion are independent chart commands", "[ColorInversion]")
{
	auto chart = LoadColorInvertKsh();
	chart.compat.kshUnknown.option["lane_toggle"] = { { 480, "0" } };
	const auto cache = kson::CreateTimingCache(chart.beat);
	const MusicGame::LaneVisibility lane(chart, cache);
	const MusicGame::ChartToggleTimeline inversion(chart, cache, "color_invert");
	REQUIRE(lane.hiddenRatioAt(0.75) == 0.0);
	REQUIRE(inversion.valueAt(0.75) == 1.0);
	REQUIRE(lane.hiddenRatioAt(3.0) == 1.0);
	REQUIRE(inversion.valueAt(3.0) == 0.0);
}

TEST_CASE("KSON preserves color inversion commands and their transition timing", "[ColorInversion]")
{
	const auto original = LoadColorInvertKsh();
	std::stringstream stream;
	REQUIRE(kson::SaveKsonChartData(stream, original) == kson::ErrorType::None);
	const auto chart = kson::LoadKsonChartData(stream);
	REQUIRE(chart.error == kson::ErrorType::None);
	const MusicGame::ChartToggleTimeline inversion(chart, kson::CreateTimingCache(chart.beat), "color_invert");
	REQUIRE(inversion.valueAt(0.25) == Approx(0.48));
	REQUIRE(inversion.valueAt(2.25) == Approx(0.52));
}

TEST_CASE("Color inversion supports immediate toggles, seeking, and paused queries", "[ColorInversion]")
{
	auto chart = LoadColorInvertKsh();
	chart.compat.kshUnknown.option["color_invert"] = { { 240, "0" }, { 960, "192" } };
	const MusicGame::ChartToggleTimeline inversion(chart, kson::CreateTimingCache(chart.beat), "color_invert");
	REQUIRE(inversion.valueAt(10.0) == 0.0);
	REQUIRE(inversion.valueAt(0.49) == 0.0);
	REQUIRE(inversion.valueAt(0.5) == 1.0);
	REQUIRE(inversion.valueAt(2.5) == Approx(0.75));
	REQUIRE(inversion.valueAt(2.5) == Approx(0.75));
	REQUIRE(inversion.valueAt(-1.0) == 0.0);
}

TEST_CASE("A new color inversion command reverses an unfinished radial transition", "[ColorInversion]")
{
	auto chart = LoadColorInvertKsh();
	chart.compat.kshUnknown.option["color_invert"] = { { 0, "192" }, { 480, "96" } };
	const MusicGame::ChartToggleTimeline inversion(chart, kson::CreateTimingCache(chart.beat), "color_invert");
	REQUIRE(inversion.valueAt(1.0) == Approx(0.5));
	REQUIRE(inversion.valueAt(1.25) == Approx(0.25));
	REQUIRE(inversion.valueAt(1.5) == 0.0);
}

TEST_CASE("Color inversion uses the event BPM including playback speed scaling", "[ColorInversion]")
{
	auto chart = LoadColorInvertKsh();
	chart.beat.bpm[960] = 240.0;
	for (auto& [pulse, bpm] : chart.beat.bpm)
	{
		bpm *= 2.0;
	}
	const MusicGame::ChartToggleTimeline inversion(chart, kson::CreateTimingCache(chart.beat), "color_invert");
	REQUIRE(inversion.valueAt(1.0) == 1.0);
	REQUIRE(inversion.valueAt(1.0 + 12.5 / 192.0) == Approx(0.5));
}

TEST_CASE("Charts without color inversion retain the original colors", "[ColorInversion]")
{
	auto chart = LoadColorInvertKsh();
	chart.compat.kshUnknown.option.erase("color_invert");
	const MusicGame::ChartToggleTimeline inversion(chart, kson::CreateTimingCache(chart.beat), "color_invert");
	REQUIRE(inversion.valueAt(-1.0) == 0.0);
	REQUIRE(inversion.valueAt(0.0) == 0.0);
	REQUIRE(inversion.valueAt(1000.0) == 0.0);
}
