#include <eepp/graphics/ninepatch.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uistate.hpp>

namespace EE { namespace UI {

ResourcePtr<UISkin> UISkin::New( const std::string& name ) {
	return ResourcePtr<UISkin>( eeNew( UISkin, ( name ) ), ResourceDeleter<UISkin>() );
}

UISkin::UISkin( const std::string& name ) : StateListDrawable( SKIN, name ) {
	mCurrentState = UIState::StateFlagNormal;
}

UISkin::~UISkin() {}

Sizef UISkin::getSize() {
	return getSize( UIState::StateFlagNormal );
}

Sizef UISkin::getPixelsSize() {
	return StateListDrawable::getPixelsSize( UIState::StateFlagNormal );
}

Sizef UISkin::getSize( const Uint32& state ) {
	return StateListDrawable::getSize( state );
}

Sizef UISkin::getPixelsSize( const Uint32& state ) {
	return StateListDrawable::getPixelsSize( state );
}

ResourcePtr<UISkin> UISkin::clone( const std::string& newName ) const {
	auto skin = ResourcePtr<UISkin>( eeNew( UISkin, ( newName ) ), ResourceDeleter<UISkin>() );
	skin->setColor( mColor );
	skin->setPosition( mPosition );
	for ( const auto& state : mDrawables ) {
		if ( !state.second )
			continue;
		DrawablePtr drawable = state.second->clone();
		if ( !drawable )
			return {};
		skin->setStateDrawable( state.first, std::move( drawable ) );
	}
	skin->mDrawableColors = mDrawableColors;
	skin->setState( mCurrentState );
	return skin;
}

ResourcePtr<UISkin> UISkin::cloneSkin() const {
	return clone( mName );
}

DrawablePtr UISkin::clone() const {
	return cloneSkin();
}

Rectf UISkin::getBorderSize() {
	return getBorderSize( UIState::StateFlagNormal );
}

Rectf UISkin::getBorderSize( const Uint32& state ) {
	if ( hasDrawableState( state ) &&
		 mDrawables[state]->getDrawableType() == EE::Graphics::Drawable::Type::NINEPATCH ) {
		NinePatch* ninePatch( static_cast<NinePatch*>( mDrawables[state].get() ) );
		TextureRegion* stl( ninePatch->getTextureRegion( NinePatch::Left ) );
		TextureRegion* str( ninePatch->getTextureRegion( NinePatch::Right ) );
		TextureRegion* stt( ninePatch->getTextureRegion( NinePatch::Up ) );
		TextureRegion* stb( ninePatch->getTextureRegion( NinePatch::Down ) );
		Rectf size( stl->getPixelsSize().getWidth(), stt->getPixelsSize().getHeight(),
					str->getPixelsSize().getWidth(), stb->getPixelsSize().getHeight() );
		return size;
	}

	return Rectf::Zero;
}

}} // namespace EE::UI
