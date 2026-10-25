#include "HighwayRenderTexture.hpp"
#include "MusicGame/Graphics/GraphicsDefines.hpp"
#include "MusicGame/Camera/CameraMath.hpp"

namespace MusicGame::Graphics
{
	namespace
	{
		constexpr StringView kBaseTextureFilename = U"base.gif";

		RenderTexture CreateBaseTexture(int32 column, const Color& clearColor)
		{
			const Texture textureAsset = TextureAsset(kBaseTextureFilename);

			RenderTexture texture(kHighwayTextureSizeWide);
			texture.clear(clearColor);

			// 中央へ描画
			{
				const ScopedRenderTarget2D renderTarget(texture);
				const ScopedRenderStates2D samplerState(SamplerState::ClampNearest);
				textureAsset(kHighwayTextureSize.x * column, 0, kHighwayTextureSize).draw(kHighwayTextureOffsetX, 0);
			}

			return texture;
		}
	}

	HighwayRenderTexture::HighwayRenderTexture(bool createBaseTextures)
		: m_additiveTexture(kHighwayTextureSizeWide)
		, m_invMultiplyTexture(kHighwayTextureSizeWide)
		, m_additiveBaseTexture(createBaseTextures ? CreateBaseTexture(kTextureColumnMain, Palette::Black) : RenderTexture{})
		, m_invMultiplyBaseTexture(createBaseTextures ? CreateBaseTexture(kTextureColumnSub, Palette::Black) : RenderTexture{})
	{
	}

	void HighwayRenderTexture::clear() const
	{
		m_additiveTexture.clear(Palette::Black);
		m_invMultiplyTexture.clear(Palette::Black);
	}

	void HighwayRenderTexture::draw3D(const DynamicMesh& mesh) const
	{
		{
			const ScopedRenderStates3D renderState(kInvMultiply);
			mesh.draw(m_invMultiplyTexture);
		}

		{
			const ScopedRenderStates3D renderState(BlendState::Additive);
			mesh.draw(m_additiveTexture);
		}
	}

	const RenderTexture& HighwayRenderTexture::additiveTexture() const
	{
		return m_additiveTexture;
	}

	const RenderTexture& HighwayRenderTexture::invMultiplyTexture() const
	{
		return m_invMultiplyTexture;
	}

	void HighwayRenderTexture::clearHiddenLane(double hiddenRatio) const
	{
		if (hiddenRatio <= 0.0)
		{
			return;
		}

		// 奥から判定ラインへ向かってレーンと光沢を消す。黒は両ブレンド方式の中立色。
		// 小節線・ノーツ・キービームはこの処理の後に描画する。
		const RectF hiddenArea{ 0, 0, kHighwayTextureSizeWide.x, kHighwayTextureSize.y * Clamp(hiddenRatio, 0.0, 1.0) };
		const ScopedRenderStates2D renderState(BlendState::Opaque);
		{
			const ScopedRenderTarget2D renderTarget(m_additiveTexture);
			hiddenArea.draw(Palette::Black);
		}
		{
			const ScopedRenderTarget2D renderTarget(m_invMultiplyTexture);
			hiddenArea.draw(Palette::Black);
		}
	}

	void HighwayRenderTexture::drawBaseTexture(double centerSplit) const
	{
		if (MathUtils::AlmostEquals(centerSplit, 0.0)) [[likely]]
		{
			// center_splitが0の場合は一発で描画
			Shader::Copy(m_additiveBaseTexture, m_additiveTexture);
			Shader::Copy(m_invMultiplyBaseTexture, m_invMultiplyTexture);
			return;
		}

		const double centerSplitShiftX = Camera::CenterSplitShiftX(centerSplit);
		constexpr double xOffset = (kHighwayTextureSizeWide.x - kHighwayTextureSize.x) / 2;
		constexpr SizeF halfHighwayTextureSize{ kHighwayTextureSize.x / 2, kHighwayTextureSize.y };

		const ScopedRenderStates2D samplerState{ SamplerState::ClampNearest };

		// center_splitが指定されている場合は左右別々に描画

		m_additiveTexture.clear(Palette::Black);
		{
			const ScopedRenderTarget2D renderTarget(m_additiveTexture);
			m_additiveBaseTexture(xOffset, 0, halfHighwayTextureSize).draw(xOffset - centerSplitShiftX, 0); // 左側
			m_additiveBaseTexture(xOffset + halfHighwayTextureSize.x, 0, halfHighwayTextureSize).draw(xOffset + halfHighwayTextureSize.x + centerSplitShiftX, 0); // 右側
		}

		m_invMultiplyTexture.clear(Palette::Black);
		{
			const ScopedRenderTarget2D renderTarget(m_invMultiplyTexture);
			m_invMultiplyBaseTexture(xOffset, 0, halfHighwayTextureSize).draw(xOffset - centerSplitShiftX, 0); // 左側
			m_invMultiplyBaseTexture(xOffset + halfHighwayTextureSize.x, 0, halfHighwayTextureSize).draw(xOffset + halfHighwayTextureSize.x + centerSplitShiftX, 0); // 右側
		}
	}
}
