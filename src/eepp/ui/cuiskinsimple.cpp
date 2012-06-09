#include <eepp/ui/cuiskinsimple.hpp>
#include <eepp/graphics/cshapegroupmanager.hpp>

namespace EE { namespace UI {

cUISkinSimple::cUISkinSimple( const std::string& Name ) :
	cUISkin( Name, UISkinSimple )
{
	for ( Int32 i = 0; i < cUISkinState::StateCount; i++ )
		mShape[ i ] = NULL;

	SetSkins();
}

cUISkinSimple::~cUISkinSimple() {
}

void cUISkinSimple::Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Width, const eeFloat& Height, const Uint32& Alpha, const Uint32& State ) {
	if ( 0 == Alpha )
		return;

	cShape * tShape = mShape[ State ];
	mTempColor		= mColor[ State ];

	if ( NULL != tShape ) {
		tShape->DestWidth( Width );
		tShape->DestHeight( Height );

		if ( mTempColor.Alpha != Alpha ) {
			mTempColor.Alpha = (Uint8)( (eeFloat)mTempColor.Alpha * ( (eeFloat)Alpha / 255.f ) );
		}

		tShape->Draw( X, Y, mTempColor );

		tShape->ResetDestWidthAndHeight();
	}
}

void cUISkinSimple::SetSkin( const Uint32& State ) {
	eeASSERT ( State < cUISkinState::StateCount );

	std::string Name( mName + "_" + cUISkin::GetSkinStateName( State ) );

	mShape[ State ] = cShapeGroupManager::instance()->GetShapeByName( Name );
}

cShape * cUISkinSimple::GetShape( const Uint32& State ) const {
	eeASSERT ( State < cUISkinState::StateCount );

	return mShape[ State ];
}

void cUISkinSimple::StateNormalToState( const Uint32& State ) {
	if ( NULL == mShape[ State ] )
		mShape[ State ] = mShape[ cUISkinState::StateNormal ];
}

cUISkinSimple * cUISkinSimple::Copy( const std::string& NewName, const bool& CopyColorsState ) {
	cUISkinSimple * SkinS = eeNew( cUISkinSimple, ( NewName ) );

	if ( CopyColorsState ) {
		SkinS->mColorDefault = mColorDefault;

		memcpy( &SkinS->mColor[0], &mColor[0], cUISkinState::StateCount * sizeof(eeColorA) );
	}

	memcpy( &SkinS->mShape[0], &mShape[0], cUISkinState::StateCount * sizeof(cShape*) );

	return SkinS;
}

cUISkin * cUISkinSimple::Copy() {
	return Copy( mName, true );
}

}}
