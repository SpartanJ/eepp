#include <eepp/ui/cuiskin.hpp>

namespace EE { namespace UI {

const char * UISkinStatesNames[] = {
	"normal",
	"focus",
	"selected",
	"menter",
	"mexit",
	"mdown"
};

const char * cUISkin::GetSkinStateName( const Uint32& State ) {
	return UISkinStatesNames[ State ];
}

cUISkin::cUISkin( const std::string& Name, const Uint32& Type ) :
	mType( Type ),
	mName( Name ),
	mNameHash( String::Hash( mName ) ),
	mTheme(NULL)
{
	eeColorA tColor( 255, 255, 255, 255 );

	mColorDefault	= tColor.GetValue();

	for ( Int32 i = 0; i < cUISkinState::StateCount; i++ ) {
		mColor[ i ] = tColor;
	}
}

cUISkin::~cUISkin() {
}

void cUISkin::SetColor( const Uint32& State, const eeColorA& Color ) {
	eeASSERT ( State < cUISkinState::StateCount );

	BitOp::WriteBitKey( &mColorDefault, State, 0 );

	mColor[ State ] = Color;
}

const eeColorA& cUISkin::GetColor( const Uint32& State ) const {
	eeASSERT ( State < cUISkinState::StateCount );

	return mColor[ State ];
}

const std::string& cUISkin::Name() const {
	return mName;
}

void cUISkin::Name( const std::string& name ) {
	mName = name;
	mNameHash = String::Hash( mName );
}

const Uint32& cUISkin::Id() const {
	return mNameHash;
}

void cUISkin::SetSkins() {
	for ( Int32 i = 0; i < cUISkinState::StateCount; i++ )
		SetSkin( i );
}

cUITheme * cUISkin::Theme() const {
	return mTheme;
}

void cUISkin::Theme( cUITheme * theme ) {
	mTheme = theme;
}

const Uint32& cUISkin::GetType() const {
	return mType;
}

bool cUISkin::GetColorDefault( const Uint32& State ) {
	return BitOp::ReadBitKey( &mColorDefault, State );
}

}}
