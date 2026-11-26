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

KittyGraphicsRenderer::UpdateResult
KittyGraphicsRenderer::applyUpdates( const std::vector<TerminalGraphicsUpdate>& updates ) {
	using enum UpdateResult;
#if EE_PLATFORM == EE_PLATFORM_WIN
	using GetGLError = unsigned( __stdcall* )();
#else
	using GetGLError = unsigned ( * )();
#endif
	for ( const auto& update : updates ) {
		if ( update.type == TerminalGraphicsUpdateType::ResetAll ) {
			mRecoveryImages.emplace();
			mRecoveryPresentation = update.recoveryPresentation;
			mRecoverySequence = update.sequence;
			mRecoveryEndSequence = mRecoveryPresentation
									   ? mRecoveryPresentation->requiredUpdateSequence
									   : update.sequence;
		} else if ( update.type == TerminalGraphicsUpdateType::Resync ) {
			mRecoveryImages.reset();
			mRecoveryPresentation.reset();
			return ResyncNeeded;
		} else if ( mRecoveryImages ) {
			// The texture factory can return a GL name even when GL rejected the upload.
			static const auto getError = reinterpret_cast<GetGLError>(
				Renderer::instance()->getProcAddress( "glGetError" ) );
			if ( getError ) {
				while ( getError() != 0 ) {
				}
			}
			const bool applied =
				update.sequence == mRecoverySequence + 1 && applyUpdate( *mRecoveryImages, update );
			if ( !applied || ( getError && getError() != 0 ) ) {
				mRecoveryImages.reset();
				mRecoveryPresentation.reset();
				return RecoveryFailed;
			}
			mRecoverySequence = update.sequence;
		} else {
			// Ignore the remainder of an abandoned replay. A later incremental update
			// can request a fresh replay if it no longer matches the visible state.
			if ( mRecoveryEndSequence && update.sequence <= mRecoveryEndSequence )
				continue;
			if ( update.sequence != mLastAppliedSequence + 1 || !applyUpdate( mImages, update ) )
				return ResyncNeeded;
			mLastAppliedSequence = update.sequence;
		}

		if ( mRecoveryImages && mRecoverySequence == mRecoveryEndSequence ) {
			mImages.swap( *mRecoveryImages );
			mRecoveryImages.reset();
			mLastAppliedSequence = mRecoverySequence;
			if ( mRecoveryPresentation ) {
				mPresentation = std::move( mRecoveryPresentation );
				mPresentationMinSequence = mRecoverySequence;
			}
			mRecoveryEndSequence = 0;
		}
	}
	return Applied;
}

bool KittyGraphicsRenderer::applyUpdate( Images& images, const TerminalGraphicsUpdate& update ) {
	switch ( update.type ) {
		case TerminalGraphicsUpdateType::ReplaceImage: {
			if ( !update.pixels || update.imageSize.getWidth() <= 0 ||
				 update.imageSize.getHeight() <= 0 ||
				 ( update.channels != 3 && update.channels != 4 ) ||
				 update.pixels->size() != static_cast<size_t>( update.imageSize.getWidth() ) *
											  update.imageSize.getHeight() * update.channels )
				return false;
			auto existing = images.find( update.imageId );
			if ( existing != images.end() && existing->second.texture &&
				 existing->second.size == update.imageSize &&
				 existing->second.channels == update.channels ) {
				existing->second.texture->update( update.pixels->data(),
												  update.imageSize.getWidth(),
												  update.imageSize.getHeight(), 0, 0,
												  Image::channelsToPixelFormat( update.channels ) );
				existing->second.frames.clear();
				break;
			}
			[[fallthrough]];
		}
		case TerminalGraphicsUpdateType::CreateImage: {
			if ( !update.pixels || update.imageSize.getWidth() <= 0 ||
				 update.imageSize.getHeight() <= 0 ||
				 ( update.channels != 3 && update.channels != 4 ) ||
				 update.pixels->size() != static_cast<size_t>( update.imageSize.getWidth() ) *
											  update.imageSize.getHeight() * update.channels )
				return false;
			auto texture = TextureFactory::instance()->loadFromPixels(
				update.pixels->data(), update.imageSize.getWidth(), update.imageSize.getHeight(),
				update.channels, false, Texture::ClampMode::ClampToEdge, false, false );
			if ( !texture )
				return false;
			texture->setFilter( Texture::Filter::Linear );
			images.insert_or_assign(
				update.imageId,
				GPUImage{ std::move( texture ), {}, update.imageSize, update.channels } );
			break;
		}
		case TerminalGraphicsUpdateType::UpdateRegion: {
			auto image = images.find( update.imageId );
			if ( image == images.end() || !update.pixels )
				return false;
			const int width = update.region.Right - update.region.Left;
			const int height = update.region.Bottom - update.region.Top;
			if ( width <= 0 || height <= 0 || update.region.Left < 0 || update.region.Top < 0 ||
				 update.region.Right > image->second.size.getWidth() ||
				 update.region.Bottom > image->second.size.getHeight() ||
				 update.pixels->size() != static_cast<size_t>( width ) * height * 4 )
				return false;
			image->second.texture->update( update.pixels->data(), width, height, update.region.Left,
										   update.region.Top );
			break;
		}
		case TerminalGraphicsUpdateType::CreateFrame:
		case TerminalGraphicsUpdateType::ReplaceFrame: {
			auto image = images.find( update.imageId );
			if ( image == images.end() || !update.pixels || update.frameNumber <= 1 ||
				 update.pixels->size() != static_cast<size_t>( image->second.size.getWidth() ) *
											  image->second.size.getHeight() * 4 )
				return false;
			auto texture = TextureFactory::instance()->loadFromPixels(
				update.pixels->data(), image->second.size.getWidth(),
				image->second.size.getHeight(), 4, false, Texture::ClampMode::ClampToEdge, false,
				false );
			if ( !texture )
				return false;
			texture->setFilter( Texture::Filter::Linear );
			image->second.frames.insert_or_assign( update.frameNumber, std::move( texture ) );
			break;
		}
		case TerminalGraphicsUpdateType::UpdateFrameRegion: {
			auto image = images.find( update.imageId );
			if ( image == images.end() || !update.pixels )
				return false;
			auto frame = image->second.frames.find( update.frameNumber );
			if ( frame == image->second.frames.end() )
				return false;
			const int width = update.region.Right - update.region.Left;
			const int height = update.region.Bottom - update.region.Top;
			if ( width <= 0 || height <= 0 || update.region.Left < 0 || update.region.Top < 0 ||
				 update.region.Right > image->second.size.getWidth() ||
				 update.region.Bottom > image->second.size.getHeight() ||
				 update.pixels->size() != static_cast<size_t>( width ) * height * 4 )
				return false;
			frame->second->update( update.pixels->data(), width, height, update.region.Left,
								   update.region.Top );
			break;
		}
		case TerminalGraphicsUpdateType::DeleteFrame: {
			auto image = images.find( update.imageId );
			if ( image != images.end() )
				image->second.frames.erase( update.frameNumber );
			break;
		}
		case TerminalGraphicsUpdateType::DeleteImage:
			images.erase( update.imageId );
			break;
		case TerminalGraphicsUpdateType::ResetScreen:
			break;
		default:
			return false;
	}
	return true;
}

void KittyGraphicsRenderer::setPresentation(
	std::shared_ptr<const TerminalGraphicsPresentation> presentation ) {
	if ( !presentation || presentation->requiredUpdateSequence >= mPresentationMinSequence )
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

}} // namespace eterm::Terminal
