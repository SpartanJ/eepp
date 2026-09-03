#include <eterm/terminal/kittygraphicsrenderer.hpp>

#include <algorithm>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <limits>

namespace eterm { namespace Terminal {

static bool isInPass( Int32 zIndex, KittyGraphicsRenderer::Pass pass ) {
	constexpr Int32 VeryNegativeThreshold = std::numeric_limits<Int32>::min() / 2;
	return pass == KittyGraphicsRenderer::Pass::VeryNegative ? zIndex < VeryNegativeThreshold
		   : pass == KittyGraphicsRenderer::Pass::Negative
			   ? zIndex < 0 && zIndex >= VeryNegativeThreshold
			   : zIndex >= 0;
}

bool KittyGraphicsRenderer::applyUpdates( std::vector<TerminalGraphicsUpdate>&& updates ) {
	for ( auto& update : updates ) {
		if ( update.type == TerminalGraphicsUpdateType::Resync ) {
			reset();
			return false;
		}
		if ( update.type != TerminalGraphicsUpdateType::ResetAll &&
			 update.sequence != mLastAppliedSequence + 1 )
			return false;

		switch ( update.type ) {
			case TerminalGraphicsUpdateType::CreateImage:
			case TerminalGraphicsUpdateType::ReplaceImage: {
				if ( !update.rgba || update.imageSize.getWidth() <= 0 ||
					 update.imageSize.getHeight() <= 0 )
					return false;
				auto texture = TextureFactory::instance()->loadFromPixels(
					update.rgba->data(), update.imageSize.getWidth(), update.imageSize.getHeight(),
					4, false, Texture::ClampMode::ClampToEdge, false, false );
				if ( !texture )
					return false;
				texture->setFilter( Texture::Filter::Linear );
				mImages.insert_or_assign( update.imageId,
										  GPUImage{ std::move( texture ), {}, update.imageSize } );
				break;
			}
			case TerminalGraphicsUpdateType::UpdateRegion: {
				auto image = mImages.find( update.imageId );
				if ( image == mImages.end() || !update.rgba )
					return false;
				const int width = update.region.Right - update.region.Left;
				const int height = update.region.Bottom - update.region.Top;
				if ( width <= 0 || height <= 0 || update.region.Left < 0 || update.region.Top < 0 ||
					 update.region.Right > image->second.size.getWidth() ||
					 update.region.Bottom > image->second.size.getHeight() ||
					 update.rgba->size() != static_cast<size_t>( width ) * height * 4 )
					return false;
				image->second.texture->update( update.rgba->data(), width, height,
											   update.region.Left, update.region.Top );
				break;
			}
			case TerminalGraphicsUpdateType::CreateFrame:
			case TerminalGraphicsUpdateType::ReplaceFrame: {
				auto image = mImages.find( update.imageId );
				if ( image == mImages.end() || !update.rgba || update.frameNumber <= 1 ||
					 update.rgba->size() != static_cast<size_t>( image->second.size.getWidth() ) *
												image->second.size.getHeight() * 4 )
					return false;
				auto texture = TextureFactory::instance()->loadFromPixels(
					update.rgba->data(), image->second.size.getWidth(),
					image->second.size.getHeight(), 4, false, Texture::ClampMode::ClampToEdge,
					false, false );
				if ( !texture )
					return false;
				texture->setFilter( Texture::Filter::Linear );
				image->second.frames.insert_or_assign( update.frameNumber, std::move( texture ) );
				break;
			}
			case TerminalGraphicsUpdateType::UpdateFrameRegion: {
				auto image = mImages.find( update.imageId );
				if ( image == mImages.end() || !update.rgba )
					return false;
				auto frame = image->second.frames.find( update.frameNumber );
				if ( frame == image->second.frames.end() )
					return false;
				const int width = update.region.Right - update.region.Left;
				const int height = update.region.Bottom - update.region.Top;
				if ( width <= 0 || height <= 0 || update.region.Left < 0 || update.region.Top < 0 ||
					 update.region.Right > image->second.size.getWidth() ||
					 update.region.Bottom > image->second.size.getHeight() ||
					 update.rgba->size() != static_cast<size_t>( width ) * height * 4 )
					return false;
				frame->second->update( update.rgba->data(), width, height, update.region.Left,
									   update.region.Top );
				break;
			}
			case TerminalGraphicsUpdateType::DeleteFrame: {
				auto image = mImages.find( update.imageId );
				if ( image != mImages.end() )
					image->second.frames.erase( update.frameNumber );
				break;
			}
			case TerminalGraphicsUpdateType::DeleteImage:
				mImages.erase( update.imageId );
				break;
			case TerminalGraphicsUpdateType::ResetScreen:
				break;
			case TerminalGraphicsUpdateType::ResetAll:
				mImages.clear();
				break;
			default:
				return false;
		}
		mLastAppliedSequence = update.sequence;
	}
	return true;
}

void KittyGraphicsRenderer::setPresentation(
	std::shared_ptr<const TerminalGraphicsPresentation> presentation ) {
	mPresentation = std::move( presentation );
}

void KittyGraphicsRenderer::draw( Pass pass, const Vector2f& origin, const Sizef& cellSize,
								  const Sizef& gridSize ) {
	if ( !mPresentation )
		return;
	auto clippingMask = Renderer::instance()->getClippingMask();
	clippingMask->clipEnable( origin.x, origin.y, gridSize.getWidth(), gridSize.getHeight() );
	for ( const auto& placement : mPresentation->placements ) {
		if ( !isInPass( placement.zIndex, pass ) )
			continue;
		auto image = mImages.find( placement.imageId );
		if ( image == mImages.end() || !image->second.texture )
			continue;
		const Vector2f position( origin.x + placement.visibleAnchorCell.x * cellSize.getWidth() +
									 placement.firstCellPixelOffset.x,
								 origin.y + placement.visibleAnchorCell.y * cellSize.getHeight() +
									 placement.firstCellPixelOffset.y );
		const Float width = placement.columns * cellSize.getWidth();
		const Float height = placement.rows * cellSize.getHeight();
		TexturePtr texture = image->second.texture;
		if ( placement.frameNumber > 1 ) {
			auto frame = image->second.frames.find( placement.frameNumber );
			if ( frame != image->second.frames.end() )
				texture = frame->second;
		}
		texture->drawEx( position.x, position.y, width, height, 0, Vector2f::One, Color::White,
						 Color::White, Color::White, Color::White, BlendMode::Alpha(),
						 RENDER_NORMAL, OriginPoint( OriginPoint::OriginTopLeft ),
						 placement.sourcePixels );
	}
	clippingMask->clipDisable();
}

bool KittyGraphicsRenderer::hasPlacements( Pass pass ) const {
	return mPresentation &&
		   std::any_of( mPresentation->placements.begin(), mPresentation->placements.end(),
						[pass]( const TerminalVisiblePlacement& placement ) {
							return isInPass( placement.zIndex, pass );
						} );
}

void KittyGraphicsRenderer::reset() {
	mImages.clear();
	mPresentation.reset();
	mLastAppliedSequence = 0;
}

}} // namespace eterm::Terminal
