#include <eepp/ui/cuiskinsimple.hpp>
#include <eepp/graphics/ctextureatlasmanager.hpp>

namespace EE { namespace UI {

cUISkinSimple::cUISkinSimple( const std::string& Name ) :
	cUISkin( Name, UISkinSimple )
{
	for ( Int32 i = 0; i < cUISkinState::StateCount; i++ )
		mSubTexture[ i ] = NULL;

	SetSkins();
}

cUISkinSimple::~cUISkinSimple() {
}

void cUISkinSimple::Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Width, const eeFloat& Height, const Uint32& Alpha, const Uint32& State ) {
	if ( 0 == Alpha )
		return;

	cSubTexture * tSubTexture = mSubTexture[ State ];
	mTempColor		= mColor[ State ];

	if ( NULL != tSubTexture ) {
		tSubTexture->DestSize( eeSizef( Width, Height ) );

		if ( mTempColor.Alpha != Alpha ) {
			mTempColor.Alpha = (Uint8)( (eeFloat)mTempColor.Alpha * ( (eeFloat)Alpha / 255.f ) );
		}

		tSubTexture->Draw( X, Y, mTempColor );

		tSubTexture->ResetDestSize();
	}
}

void cUISkinSimple::SetSkin( const Uint32& State ) {
	eeASSERT ( State < cUISkinState::StateCount );

	std::string Name( mName + "_" + cUISkin::GetSkinStateName( State ) );

	mSubTexture[ State ] = cTextureAtlasManager::instance()->GetSubTextureByName( Name );
}

cSubTexture * cUISkinSimple::GetSubTexture( const Uint32& State ) const {
	eeASSERT ( State < cUISkinState::StateCount );

	return mSubTexture[ State ];
}

void cUISkinSimple::StateNormalToState( const Uint32& State ) {
	if ( NULL == mSubTexture[ State ] )
		mSubTexture[ State ] = mSubTexture[ cUISkinState::StateNormal ];
}

cUISkinSimple * cUISkinSimple::Copy( const std::string& NewName, const bool& CopyColorsState ) {
	cUISkinSimple * SkinS = eeNew( cUISkinSimple, ( NewName ) );

	if ( CopyColorsState ) {
		SkinS->mColorDefault = mColorDefault;

		memcpy( &SkinS->mColor[0], &mColor[0], cUISkinState::StateCount * sizeof(eeColorA) );
	}

	memcpy( &SkinS->mSubTexture[0], &mSubTexture[0], cUISkinState::StateCount * sizeof(cSubTexture*) );

	return SkinS;
}

cUISkin * cUISkinSimple::Copy() {
	return Copy( mName, true );
}

}}
