#include <eepp/ui/uiskin.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/graphics/ninepatch.hpp>

namespace EE { namespace UI {

UISkin * UISkin::New( const std::string& name ) {
	return eeNew( UISkin, ( name ) );
}

const char * UISkinStatesNames[] = {
	"normal",
	"focus",
	"selected",
	"hover",
	"pressed",
	"disabled"
};

const char * UISkin::getSkinStateName( const Uint32& State ) {
	return UISkinStatesNames[ State ];
}

int UISkin::getStateNumber( const std::string& State ) {
	for ( int i = 0; i < UISkinState::StateCount; i++ ) {
		if ( State == UISkinStatesNames[i] ) {
			return i;
		}
	}

	return -1;
}

bool UISkin::isStateName( const std::string& State ) {
	for ( int i = 0; i < UISkinState::StateCount; i++ ) {
		if ( State == UISkinStatesNames[i] ) {
			return true;
		}
	}

	return false;
}

UISkin::UISkin( const std::string& name ) :
	mName( name ),
	mNameHash( String::hash( mName ) ),
	mTheme(NULL)
{
}

UISkin::~UISkin() {
}

Sizef UISkin::getSize() {
	return getSize( 1 << UISkinState::StateNormal );
}

const std::string& UISkin::getName() const {
	return mName;
}

void UISkin::setName( const std::string& name ) {
	mName = name;
	mNameHash = String::hash( mName );
}

const Uint32& UISkin::getId() const {
	return mNameHash;
}

UITheme * UISkin::getTheme() const {
	return mTheme;
}

void UISkin::setTheme( UITheme * theme ) {
	mTheme = theme;
}

Rectf UISkin::getBorderSize() {
	return getBorderSize( 1 << UISkinState::StateNormal );
}

UISkin * UISkin::clone( const std::string& NewName ) {
	UISkin * SkinS = UISkin::New( NewName );


	return SkinS;
}

UISkin * UISkin::clone() {
	return clone( mName );
}

Sizef UISkin::getSize( const Uint32 & state ) {
	if ( NULL != mDrawables[ state ] ) {
		return mDrawables[ state ]->getSize();
	}

	return Sizef();
}

Rectf UISkin::getBorderSize( const Uint32 & state ) {
	if ( NULL != mDrawables[ state ] && mDrawables[ state ]->getDrawableType() == EE::Graphics::Drawable::Type::NINEPATCH ) {
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
