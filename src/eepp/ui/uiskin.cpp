#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uistate.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/graphics/ninepatch.hpp>

namespace EE { namespace UI {

UISkin * UISkin::New( const std::string& name ) {
	return eeNew( UISkin, ( name ) );
}

UISkin::UISkin( const std::string& name ) :
	StateListDrawable( SKIN, name )
{}

UISkin::~UISkin() {
}

Sizef UISkin::getSize() {
	return getSize( UIState::StateFlagNormal );
}

Sizef UISkin::getSize( const Uint32 & state ) {
	return StateListDrawable::getSize( state );
}

UISkin * UISkin::clone( const std::string& NewName ) {
	UISkin * SkinS = UISkin::New( NewName );

	if ( !mDrawableOwner ) {
		SkinS->mColor = mColor;
		SkinS->mPosition = mPosition;
		SkinS->mDrawables = mDrawables;
		SkinS->mCurrentState = mCurrentState;
		SkinS->mCurrentDrawable = mCurrentDrawable;
	}

	return SkinS;
}

UISkin * UISkin::clone() {
	return clone( mName );
}

Rectf UISkin::getBorderSize() {
	return getBorderSize( UIState::StateFlagNormal );
}

Rectf UISkin::getBorderSize( const Uint32 & state ) {
	if ( hasDrawableState( state ) && mDrawables[ state ]->getDrawableType() == EE::Graphics::Drawable::Type::NINEPATCH ) {
		NinePatch * ninePatch( static_cast<NinePatch*>( mDrawables[ state ] ) );
		TextureRegion * stl( ninePatch->getTextureRegion( NinePatch::Left ) );
		TextureRegion * str( ninePatch->getTextureRegion( NinePatch::Right ) );
		TextureRegion * stt( ninePatch->getTextureRegion( NinePatch::Up ) );
		TextureRegion * stb( ninePatch->getTextureRegion( NinePatch::Down ) );
		Rectf size( stl->getPxSize().getWidth(), stt->getPxSize().getHeight(), str->getPxSize().getWidth(), stb->getPxSize().getHeight() );
		return size;
	}

	return Rectf();
}

}}
