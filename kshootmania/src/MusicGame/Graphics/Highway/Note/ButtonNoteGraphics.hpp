#pragma once
#include "MusicGame/GameStatus.hpp"
#include "MusicGame/ViewStatus.hpp"
#include "MusicGame/PlayOption.hpp"
#include "MusicGame/Scroll/HighwayScroll.hpp"
#include "MusicGame/Graphics/Highway/HighwayRenderTexture.hpp"

namespace MusicGame::Graphics
{
	class ButtonNoteGraphics
	{
	private:
		const TiledTexture m_chipBTNoteTexture;
		const Texture m_longBTNoteTexture;
		const Texture m_longBTNoteStartTexture;

		const TiledTexture m_chipFXNoteTexture;
		const TiledTexture m_chipFXSENoteTexture;
		const Texture m_longFXNoteTexture;
		const Texture m_longFXNoteStartTexture;

		void drawChipNotesCommon(const kson::ChartData& chartData, const ViewStatus& viewStatus, const PlayOption& playOption, const Scroll::HighwayScrollContext& highwayScrollContext, const RenderTexture& target, bool isBT, bool eraseFX = false) const;

		void drawChipBTNotes(const kson::ChartData& chartData, const ViewStatus& viewStatus, const PlayOption& playOption, const Scroll::HighwayScrollContext& highwayScrollContext, const HighwayRenderTexture& target) const;

		void drawChipFXNotes(const kson::ChartData& chartData, const ViewStatus& viewStatus, const PlayOption& playOption, const Scroll::HighwayScrollContext& highwayScrollContext, const HighwayRenderTexture& target) const;

		void drawLongNotesCommon(const kson::ChartData& chartData, const GameStatus& gameStatus, const ViewStatus& viewStatus, const Scroll::HighwayScrollContext& highwayScrollContext, const RenderTexture& additiveTarget, const RenderTexture& invMultiplyTarget, bool isBT, bool eraseFX = false) const;

		void drawLongBTNotes(const kson::ChartData& chartData, const GameStatus& gameStatus, const ViewStatus& viewStatus, const Scroll::HighwayScrollContext& highwayScrollContext, const HighwayRenderTexture& target) const;

		void drawLongFXNotes(const kson::ChartData& chartData, const GameStatus& gameStatus, const ViewStatus& viewStatus, const Scroll::HighwayScrollContext& highwayScrollContext, const HighwayRenderTexture& target) const;

	public:
		ButtonNoteGraphics();

		void draw(const kson::ChartData& chartData, const GameStatus& gameStatus, const ViewStatus& viewStatus, const PlayOption& playOption, const Scroll::HighwayScrollContext& highwayScrollContext, const HighwayRenderTexture& target) const;

		void drawBT(const kson::ChartData& chartData, const GameStatus& gameStatus, const ViewStatus& viewStatus, const PlayOption& playOption, const Scroll::HighwayScrollContext& highwayScrollContext, const HighwayRenderTexture& target) const;

		/// @brief 色反転対象外のFXを描画する。上に重なるBTの領域は透明にする。
		void drawUninvertedFX(const kson::ChartData& chartData, const GameStatus& gameStatus, const ViewStatus& viewStatus, const PlayOption& playOption, const Scroll::HighwayScrollContext& highwayScrollContext, const RenderTexture& target) const;
	};
}
