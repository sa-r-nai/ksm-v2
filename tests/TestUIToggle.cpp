#include <catch2/catch.hpp>
#include <sstream>
#include "MusicGame/ChartToggleTimeline.hpp"
#include "kson/IO/KshIO.hpp"
#include "kson/IO/KsonIO.hpp"

namespace
{
	kson::ChartData LoadUIToggleKsh()
	{
		std::istringstream stream{
			"title=UI slide test\nt=120\nver=171\n--\n"
			"ui_toggle=50\n1000|00|--\n0000|00|--\n0000|00|--\n0000|00|--\n--\n"
			"ui_toggle=50\n0100|00|--\n0000|00|--\n0000|00|--\n0000|00|--\n--\n"
		};
		return kson::LoadKshChartData(stream);
	}
}

TEST_CASE("ui_toggle=50 moves the HUD out and back using the common chart time unit", "[UIToggle]")
{
	const auto chart = LoadUIToggleKsh();
	REQUIRE(chart.error == kson::ErrorType::None);
	const MusicGame::ChartToggleTimeline ui(chart, kson::CreateTimingCache(chart.beat), "ui_toggle");
	const double duration = 2.0 * 50.0 / 192.0;
	REQUIRE(ui.valueAt(-1.0) == 0.0);
	REQUIRE(ui.valueAt(0.0) == 0.0);
	REQUIRE(ui.valueAt(duration / 2) == Approx(0.5));
	REQUIRE(ui.valueAt(duration) == 1.0);
	REQUIRE(ui.valueAt(2.0) == 1.0);
	REQUIRE(ui.valueAt(2.0 + duration / 2) == Approx(0.5));
	REQUIRE(ui.valueAt(3.0) == 0.0);
}

TEST_CASE("KSON preserves UI slide commands", "[UIToggle]")
{
	const auto original = LoadUIToggleKsh();
	std::stringstream stream;
	REQUIRE(kson::SaveKsonChartData(stream, original) == kson::ErrorType::None);
	const auto chart = kson::LoadKsonChartData(stream);
	REQUIRE(chart.error == kson::ErrorType::None);
	const MusicGame::ChartToggleTimeline ui(chart, kson::CreateTimingCache(chart.beat), "ui_toggle");
	REQUIRE(ui.valueAt(0.25) == Approx(0.48));
	REQUIRE(ui.valueAt(2.25) == Approx(0.52));
}

TEST_CASE("UI slides can seek, pause, and reverse in the middle of a transition", "[UIToggle]")
{
	auto chart = LoadUIToggleKsh();
	chart.compat.kshUnknown.option["ui_toggle"] = { { 0, "192" }, { 480, "96" } };
	const MusicGame::ChartToggleTimeline ui(chart, kson::CreateTimingCache(chart.beat), "ui_toggle");
	REQUIRE(ui.valueAt(10.0) == 0.0);
	REQUIRE(ui.valueAt(1.0) == Approx(0.5));
	REQUIRE(ui.valueAt(1.25) == Approx(0.25));
	REQUIRE(ui.valueAt(1.25) == Approx(0.25));
	REQUIRE(ui.valueAt(-1.0) == 0.0);
}

TEST_CASE("UI toggles operate independently of lane visibility and color inversion", "[UIToggle]")
{
	auto chart = LoadUIToggleKsh();
	chart.compat.kshUnknown.option["lane_toggle"] = { { 480, "0" } };
	chart.compat.kshUnknown.option["color_invert"] = { { 480, "0" } };
	const auto cache = kson::CreateTimingCache(chart.beat);
	const MusicGame::ChartToggleTimeline ui(chart, cache, "ui_toggle");
	const MusicGame::ChartToggleTimeline lane(chart, cache, "lane_toggle");
	const MusicGame::ChartToggleTimeline color(chart, cache, "color_invert");
	REQUIRE(ui.valueAt(0.75) == 1.0);
	REQUIRE(lane.valueAt(0.75) == 0.0);
	REQUIRE(color.valueAt(0.75) == 0.0);
	REQUIRE(ui.valueAt(3.0) == 0.0);
	REQUIRE(lane.valueAt(3.0) == 1.0);
	REQUIRE(color.valueAt(3.0) == 1.0);
}

TEST_CASE("UI toggles support immediate movement and charts without commands remain visible", "[UIToggle]")
{
	auto chart = LoadUIToggleKsh();
	chart.compat.kshUnknown.option["ui_toggle"] = { { 240, "0" }, { 480, "0" } };
	const MusicGame::ChartToggleTimeline ui(chart, kson::CreateTimingCache(chart.beat), "ui_toggle");
	REQUIRE(ui.valueAt(0.49) == 0.0);
	REQUIRE(ui.valueAt(0.5) == 1.0);
	REQUIRE(ui.valueAt(1.0) == 0.0);

	chart.compat.kshUnknown.option.erase("ui_toggle");
	const MusicGame::ChartToggleTimeline defaultUI(chart, kson::CreateTimingCache(chart.beat), "ui_toggle");
	REQUIRE(defaultUI.valueAt(100.0) == 0.0);
}
