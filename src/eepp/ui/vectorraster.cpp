#include "vectorraster.hpp"

#include <cmath>
#include <eepp/graphics/image.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/drawableresolver.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <limits>

namespace EE { namespace UI {

namespace {

// The scale whose raster covers destination: on the axis that needs the larger scale, the raster
// reaches the destination rounded up. Scales are floats, so the product can fall just short of
// the target and be truncated below it: nudge the scale up until it reaches the target.
Float coveringScale( const Sizef& vectorSize, const Sizef& destination ) {
	const bool byWidth = destination.x / vectorSize.x >= destination.y / vectorSize.y;
	const Float target = std::ceil( ( byWidth ? destination.x : destination.y ) - 0.001f );
	Float scale = target / ( byWidth ? vectorSize.x : vectorSize.y );
	for ( int step = 0; step < 8; ++step ) {
		const auto raster = Image::svgRasterSize( vectorSize, scale );
		if ( !raster || ( byWidth ? raster->x : raster->y ) >= target )
			break;
		scale = std::nextafter( scale, std::numeric_limits<Float>::max() );
	}
	return scale;
}

} // namespace

std::unique_ptr<VectorRaster> VectorRaster::create( UINode* owner, std::string source,
													DrawableAccess currentDrawable ) {
	const std::string_view view( source );
	if ( !view.starts_with( "http://" ) && !view.starts_with( "https://" ) &&
		 !view.starts_with( "data:image/svg+xml" ) && !DrawableResolver::isSvgPath( view ) )
		return {};
	return std::make_unique<VectorRaster>( owner, std::move( source ),
										   std::move( currentDrawable ) );
}

VectorRaster::VectorRaster( UINode* owner, std::string source, DrawableAccess currentDrawable ) :
	mOwner( owner ),
	mCurrentDrawable( std::move( currentDrawable ) ),
	mSource( std::move( source ) ),
	mLifetime( std::make_shared<VectorRaster*>( this ) ) {}

TextureDrawable* VectorRaster::textureDrawableOf( const DrawablePtr& drawable ) {
	return drawable && drawable->getDrawableType() == Drawable::TEXTUREDRAWABLE
			   ? static_cast<TextureDrawable*>( drawable.get() )
			   : nullptr;
}

VectorRaster::~VectorRaster() {
	*mLifetime = nullptr;
	// The owner's destructor removes its actions itself; this covers a helper replaced earlier.
	SceneNode* scene = mOwner->getSceneNode();
	if ( scene && static_cast<Node*>( scene ) != mOwner && scene->getActionManager() ) {
		mOwner->removeActionsByTag( firstTag() );
		mOwner->removeActionsByTag( resizeTag() );
	}
}

Action::UniqueID VectorRaster::firstTag() const {
	return reinterpret_cast<UintPtr>( this );
}

Action::UniqueID VectorRaster::resizeTag() const {
	// The object spans more than one byte, so this address is also unique to it.
	return reinterpret_cast<UintPtr>( this ) + 1;
}

void VectorRaster::update( const Sizef& destinationPixels ) {
	TextureDrawable* drawable = mCurrentDrawable();
	if ( !drawable )
		return;
	// A collapsed consumer must not receive a raster for its previous size.
	if ( destinationPixels.x < 1.f || destinationPixels.y < 1.f ) {
		cancelPending();
		return;
	}
	const TexturePtr& current = drawable->getTexture();
	if ( !current || !current->isVectorRaster() )
		return;

	// A raster already made for this destination: keep it, or give it to a drawable the consumer
	// rebuilt at the base scale.
	if ( mRaster && mRasterDestination == destinationPixels ) {
		if ( current != mRaster )
			drawable->setTexture( mRaster );
		cancelPending();
		return;
	}

	const Sizef& vectorSize = current->getVectorSize();
	if ( vectorSize.x <= 0.f || vectorSize.y <= 0.f )
		return;
	// The raster keeps the SVG's aspect ratio and covers the destination on its larger axis, so
	// it can be larger than the destination on the other one.
	const Float svgScale = coveringScale( vectorSize, destinationPixels );
	const auto rasterPixels = Image::svgRasterSize( vectorSize, svgScale );
	if ( !rasterPixels || rasterPixels->x > MaxRasterSize || rasterPixels->y > MaxRasterSize ||
		 current->getPixelsSize() == rasterPixels->asFloat() ) {
		cancelPending();
		return;
	}

	// Already requested, or the rasterizer could not match this size: don't retry it.
	if ( destinationPixels == mRequestedSize )
		return;
	mRequestedSize = destinationPixels;
	// A raster still loading for an earlier size must not land while this one waits.
	++mRequestId;

	// Consumers report sizes while drawing, so rasterize on the next scene update. Resizing
	// reports many sizes in a row; after the first raster, wait for the size to settle. The two
	// use separate tags because a debounce keeps the delay of the action it restarts. The action
	// can outlive the helper in a scene its owner left, so it checks the lifetime token, and it
	// is dropped once a later size supersedes it.
	const bool first = !mHasRequested;
	mHasRequested = true;
	const Uint64 scheduledId = ++mScheduledId;
	mOwner->debounce(
		[lifetime = std::weak_ptr<VectorRaster*>( mLifetime ), scheduledId, svgScale,
		 destinationPixels] {
			auto self = lifetime.lock();
			if ( self && *self && ( *self )->mScheduledId == scheduledId )
				( *self )->request( svgScale, destinationPixels );
		},
		first ? Time::Zero : Milliseconds( 150 ), first ? firstTag() : resizeTag() );
}

void VectorRaster::cancelPending() {
	if ( mRequestedSize == Sizef{} )
		return;
	// The drawn size went back to a raster already in place: drop the queued request and any
	// raster still in flight for the abandoned size.
	mRequestedSize = {};
	++mScheduledId;
	++mRequestId;
}

void VectorRaster::request( Float svgScale, Sizef destinationPixels ) {
	UISceneNode* scene = mOwner->getUISceneNode();
	if ( !scene )
		return;

	const Uint64 requestId = ++mRequestId;
	TexturePtr texture = scene->getDrawableResolver().resolveSvgRaster(
		mSource, svgScale,
		[lifetime = std::weak_ptr<VectorRaster*>( mLifetime ), requestId,
		 destinationPixels]( TexturePtr raster ) {
			// Runs on the UI thread, after the helper may have been destroyed or asked for another
			// size.
			auto self = lifetime.lock();
			if ( raster && self && *self && ( *self )->mRequestId == requestId )
				( *self )->apply( std::move( raster ), destinationPixels );
		} );
	if ( texture )
		apply( std::move( texture ), destinationPixels );
}

void VectorRaster::apply( TexturePtr texture, const Sizef& destinationPixels ) {
	mRaster = std::move( texture );
	mRasterDestination = destinationPixels;
	if ( TextureDrawable* drawable = mCurrentDrawable() )
		drawable->setTexture( mRaster );
}

}} // namespace EE::UI
