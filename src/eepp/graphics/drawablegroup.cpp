#include <eepp/graphics/drawablegroup.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/math/rect.hpp>

namespace EE { namespace Graphics {

ResourcePtr<DrawableGroup> DrawableGroup::New() {
	return makeResource<DrawableGroup>();
}

DrawableGroup::DrawableGroup() :
	Drawable( Drawable::GROUP ), mNeedsUpdate( true ), mClipEnabled( false ) {}

DrawableGroup::~DrawableGroup() {
	clearDrawables();
}

DrawablePtr DrawableGroup::clone() const {
	auto instance = makeResource<DrawableGroup>();
	instance->mPosition = mPosition;
	instance->mColor = mColor;
	instance->mSize = mSize;
	instance->mClipEnabled = mClipEnabled;

	for ( const auto& drawable : mGroup ) {
		if ( !drawable )
			continue;
		DrawablePtr drawableInstance = drawable->clone();
		if ( !drawableInstance )
			return {};
		instance->addDrawable( std::move( drawableInstance ) );
	}

	return instance;
}

void DrawableGroup::clearDrawables() {
	mGroup.clear();
	mPos.clear();
}

DrawablePtr DrawableGroup::addDrawable( DrawablePtr drawable ) {
	if ( !drawable )
		return {};
	mPos.push_back( drawable->getPosition() );
	mGroup.push_back( std::move( drawable ) );
	return mGroup.back();
}

Uint32 DrawableGroup::getDrawableCount() const {
	return mGroup.size();
}

bool DrawableGroup::isClipEnabled() const {
	return mClipEnabled;
}

void DrawableGroup::setClipEnabled( bool clipEnabled ) {
	mClipEnabled = clipEnabled;
}

std::vector<DrawablePtr>& DrawableGroup::getGroup() {
	return mGroup;
}

Sizef DrawableGroup::getSize() {
	return mSize;
}

Sizef DrawableGroup::getPixelsSize() {
	return mSize;
}

void DrawableGroup::draw( const Vector2f& position, const Sizef& size ) {
	if ( position != mPosition ) {
		mPosition = position;
		mNeedsUpdate = true;
	}

	if ( size != mSize ) {
		mSize = size;
		mNeedsUpdate = true;
	}

	if ( mNeedsUpdate )
		update();

	if ( mGroup.empty() )
		return;

	if ( mClipEnabled )
		GLi->getClippingMask()->clipPlaneEnable( mPosition.x, mPosition.y, mSize.x, mSize.y );

	for ( std::size_t i = 0; i < mGroup.size(); i++ ) {
		Drawable* drawable = mGroup[i].get();
		drawable->draw();
	}

	if ( mClipEnabled )
		GLi->getClippingMask()->clipPlaneDisable();
}

void DrawableGroup::draw( const Vector2f& position ) {
	draw( position, mSize );
}

void DrawableGroup::draw() {
	draw( mPosition, mSize );
}

void DrawableGroup::onPositionChange() {
	mNeedsUpdate = true;
}

void DrawableGroup::onAlphaChange() {
	for ( std::size_t i = 0; i < mGroup.size(); i++ ) {
		Drawable* drawable = mGroup[i].get();
		drawable->setAlpha( getAlpha() );
	}
}

void DrawableGroup::update() {
	Sizef nSize( mSize );

	for ( std::size_t i = 0; i < mGroup.size(); i++ ) {
		Drawable* drawable = mGroup[i].get();
		Vector2f pos( mPosition + mPos[i] );
		Sizef s( mPos[i] + drawable->getSize() );

		drawable->setPosition( pos );

		nSize.x = eemax( nSize.x, s.x );
		nSize.y = eemax( nSize.y, s.y );
	}

	if ( Vector2f::Zero == mSize ) {
		mSize = nSize;
	}

	mNeedsUpdate = false;
}

}} // namespace EE::Graphics
