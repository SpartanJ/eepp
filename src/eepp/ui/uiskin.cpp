#include <eepp/ui/uiskin.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

const char * UISkinStatesNames[] = {
	"normal",
	"focus",
	"selected",
	"menter",
	"mexit",
	"mdown"
};

const char * UISkin::getSkinStateName( const Uint32& State ) {
	return UISkinStatesNames[ State ];
}

bool UISkin::isStateName( const std::string& State ) {
	for ( int i = 0; i < UISkinState::StateCount; i++ ) {
		if ( State == UISkinStatesNames[i] ) {
			return true;
		}
	}

	return false;
}

UISkin::UISkin( const std::string& name, const Uint32& Type ) :
	mType( Type ),
	mName( name ),
	mNameHash( String::hash( mName ) ),
	mTheme(NULL)
{
	Color tColor( 255, 255, 255, 255 );

	mColorDefault	= tColor.getValue();

	for ( Int32 i = 0; i < UISkinState::StateCount; i++ ) {
		mColor[ i ] = tColor;
	}
}

UISkin::~UISkin() {
}

Sizei UISkin::getSize() {
	return getSize( UISkinState::StateNormal );
}

void UISkin::setColor( const Uint32& State, const Color& Color ) {
	eeASSERT ( State < UISkinState::StateCount );

	BitOp::writeBitKey( &mColorDefault, State, 0 );

	mColor[ State ] = Color;
}

const Color& UISkin::getColor( const Uint32& State ) const {
	eeASSERT ( State < UISkinState::StateCount );

	return mColor[ State ];
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

void UISkin::setSkins() {
	for ( Int32 i = 0; i < UISkinState::StateCount; i++ )
		setSkin( i );
}

UITheme * UISkin::getTheme() const {
	return mTheme;
}

void UISkin::setTheme( UITheme * theme ) {
	mTheme = theme;
}

const Uint32& UISkin::getType() const {
	return mType;
}

Sizei UISkin::getBorderSize() {
	return getBorderSize( UISkinState::StateNormal );
}

bool UISkin::getColorDefault( const Uint32& State ) {
	return BitOp::readBitKey( &mColorDefault, State );
}

}}
