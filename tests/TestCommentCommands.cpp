#include <catch2/catch.hpp>
#include <sstream>
#include "MusicGame/ChartToggleTimeline.hpp"
#include "MusicGame/LaneVisibility.hpp"
#include "kson/IO/KsonIO.hpp"

namespace
{
	kson::ChartData LoadCommentCommandKson()
	{
		std::istringstream stream{ R"({
			"format_version": 1,
			"beat": { "bpm": [[0, 120.0]], "time_sig": [[0, [4, 4]]] },
			"editor": { "comment": [
				[240, "ui_toggle=50\nlane_toggle=50\ncolor_invert=50"],
				[1200, "ui_toggle=50\nlane_toggle=50\ncolor_invert=50"]
			] }
		})" };
		return kson::LoadKsonChartData(stream);
	}
}

TEST_CASE("KSON editor comments trigger all three effects at their comment pulse", "[CommentCommands]")
{
	const auto chart = LoadCommentCommandKson();
	REQUIRE(chart.error == kson::ErrorType::None);
	REQUIRE(chart.compat.kshUnknown.option.empty());
	const auto cache = kson::CreateTimingCache(chart.beat);
	for (const auto* name : { "ui_toggle", "lane_toggle", "color_invert" })
	{
		INFO(name);
		const MusicGame::ChartToggleTimeline timeline(chart, cache, name);
		const double duration = 2.0 * 50.0 / 192.0;
		REQUIRE(timeline.valueAt(0.49) == 0.0);
		REQUIRE(timeline.valueAt(0.5) == 0.0);
		REQUIRE(timeline.valueAt(0.5 + duration / 2) == Approx(0.5));
		REQUIRE(timeline.valueAt(1.5) == 1.0);
		REQUIRE(timeline.valueAt(2.5 + duration / 2) == Approx(0.5));
		REQUIRE(timeline.valueAt(3.5) == 0.0);
	}
	const MusicGame::LaneVisibility lane(chart, cache);
	REQUIRE(lane.hiddenRatioAt(1.5) == 1.0);
}

TEST_CASE("Comment commands support whitespace, CRLF, and separate comments at one pulse", "[CommentCommands]")
{
	auto chart = LoadCommentCommandKson();
	chart.editor.comment = {
		{ 0, "  \tui_toggle \t= 0 \r\n\r\n Notes for the chart\r\n lane_toggle=0\t " },
		{ 0, " color_invert = 0 " },
	};
	const auto cache = kson::CreateTimingCache(chart.beat);
	for (const auto* name : { "ui_toggle", "lane_toggle", "color_invert" })
	{
		const MusicGame::ChartToggleTimeline timeline(chart, cache, name);
		REQUIRE(timeline.valueAt(0.0) == 1.0);
	}
}

TEST_CASE("Memo text, examples, unknown names, and malformed durations do not execute", "[CommentCommands]")
{
	auto chart = LoadCommentCommandKson();
	chart.editor.comment = { { 0,
		"\n\t\r\nA note about ui_toggle=0\nui_toggle_extra=0\n"
		"//ui_toggle=0\nui_toggle=\nui_toggle=-1\nui_toggle=1.5\n"
		"ui_toggle=4294967296\nui_toggle=50ms\nui_toggle=50 # memo\n"
		"ui_toggle=0=color_invert=0\nother_command=0\n" } };
	const auto cache = kson::CreateTimingCache(chart.beat);
	for (const auto* name : { "ui_toggle", "lane_toggle", "color_invert" })
	{
		const MusicGame::ChartToggleTimeline timeline(chart, cache, name);
		REQUIRE(timeline.valueAt(100.0) == 0.0);
	}
}

TEST_CASE("Executing comments preserves their text and KSON save round trips", "[CommentCommands]")
{
	const auto chart = LoadCommentCommandKson();
	const auto originalComments = chart.editor.comment;
	const auto originalOptions = chart.compat.kshUnknown.option;
	const auto cache = kson::CreateTimingCache(chart.beat);
	for (const auto* name : { "ui_toggle", "lane_toggle", "color_invert" })
	{
		const MusicGame::ChartToggleTimeline timeline(chart, cache, name);
		REQUIRE(timeline.valueAt(1.5) == 1.0);
	}
	REQUIRE(chart.editor.comment == originalComments);
	REQUIRE(chart.compat.kshUnknown.option == originalOptions);

	std::stringstream stream;
	REQUIRE(kson::SaveKsonChartData(stream, chart) == kson::ErrorType::None);
	const auto reloaded = kson::LoadKsonChartData(stream);
	REQUIRE(reloaded.error == kson::ErrorType::None);
	REQUIRE(reloaded.editor.comment == originalComments);
	REQUIRE(reloaded.compat.kshUnknown.option.empty());
	const MusicGame::ChartToggleTimeline timeline(reloaded, kson::CreateTimingCache(reloaded.beat), "ui_toggle");
	REQUIRE(timeline.valueAt(0.75) == Approx(0.48));
}

TEST_CASE("Legacy options and comment commands merge in chronological order", "[CommentCommands]")
{
	auto chart = LoadCommentCommandKson();
	chart.editor.comment = { { 0, "ui_toggle=0" }, { 1920, "ui_toggle=0" } };
	chart.compat.kshUnknown.option["ui_toggle"] = { { 960, "0" } };
	const MusicGame::ChartToggleTimeline timeline(chart, kson::CreateTimingCache(chart.beat), "ui_toggle");
	REQUIRE(timeline.valueAt(1.0) == 1.0);
	REQUIRE(timeline.valueAt(3.0) == 0.0);
	REQUIRE(timeline.valueAt(5.0) == 1.0);
}

TEST_CASE("An explicit option overrides a duplicate comment without a double toggle", "[CommentCommands]")
{
	auto chart = LoadCommentCommandKson();
	chart.editor.comment = { { 0, "ui_toggle=192\ncolor_invert=0" }, { 960, "ui_toggle=192" } };
	chart.compat.kshUnknown.option["ui_toggle"] = { { 0, "0" }, { 960, "0" } };
	const auto cache = kson::CreateTimingCache(chart.beat);
	const MusicGame::ChartToggleTimeline ui(chart, cache, "ui_toggle");
	const MusicGame::ChartToggleTimeline color(chart, cache, "color_invert");
	REQUIRE(ui.valueAt(0.0) == 1.0);
	REQUIRE(ui.valueAt(2.0) == 0.0);
	REQUIRE(color.valueAt(0.0) == 1.0);
}

TEST_CASE("An invalid explicit option does not suppress a valid comment command", "[CommentCommands]")
{
	auto chart = LoadCommentCommandKson();
	chart.editor.comment = { { 0, "ui_toggle=0" } };
	chart.compat.kshUnknown.option["ui_toggle"] = { { 0, "invalid" } };
	const MusicGame::ChartToggleTimeline timeline(chart, kson::CreateTimingCache(chart.beat), "ui_toggle");
	REQUIRE(timeline.valueAt(0.0) == 1.0);
}

TEST_CASE("Repeated comment commands at one pulse keep their original order", "[CommentCommands]")
{
	auto chart = LoadCommentCommandKson();
	chart.editor.comment = { { 0, "ui_toggle=0\nui_toggle=192" } };
	const MusicGame::ChartToggleTimeline timeline(chart, kson::CreateTimingCache(chart.beat), "ui_toggle");
	REQUIRE(timeline.valueAt(0.0) == 1.0);
	REQUIRE(timeline.valueAt(1.0) == Approx(0.5));
	REQUIRE(timeline.valueAt(2.0) == 0.0);
}

TEST_CASE("Comment command time follows event BPM, playback speed, seeking, and pauses", "[CommentCommands]")
{
	auto chart = LoadCommentCommandKson();
	chart.beat.bpm[0] = 240.0;
	chart.beat.bpm[960] = 480.0;
	chart.editor.comment = { { 960, "ui_toggle=192" }, { 1920, "ui_toggle=192" } };
	const MusicGame::ChartToggleTimeline timeline(chart, kson::CreateTimingCache(chart.beat), "ui_toggle");
	REQUIRE(timeline.valueAt(10.0) == 0.0);
	REQUIRE(timeline.valueAt(1.25) == Approx(0.5));
	REQUIRE(timeline.valueAt(1.25) == Approx(0.5));
	REQUIRE(timeline.valueAt(1.75) == Approx(0.5));
	REQUIRE(timeline.valueAt(0.5) == 0.0);
}
