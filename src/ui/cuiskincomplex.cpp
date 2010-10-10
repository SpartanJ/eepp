#include "cuiskincomplex.hpp"
#include "../graphics/cshapegroupmanager.hpp"

namespace EE { namespace UI {

static const char SideSuffix[ cUISkinComplex::SideCount ][4] = {
	"ml", "mr","d","u","ul","ur","dl","dr","m"
};

cUISkinComplex::cUISkinComplex( const std::string& Name ) :
	cUISkin( Name )
{
	for ( Int32 x = 0; x < StateCount; x++ )
		for ( Int32 y = 0; y < SideCount; y++ )
			mShape[ x ][ y ] = NULL;

	SetSkins();
}

cUISkinComplex::~cUISkinComplex() {

}

void cUISkinComplex::Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Width, const eeFloat& Height ) {
	cShape * tShape = mShape[ mCurState ][ UpLeft ];

	eeSize uls;

	if ( NULL != tShape ) {
		uls = tShape->RealSize();

		tShape->Draw( X, Y, mColor[ mCurState ] );
	}

	tShape = mShape[ mCurState ][ DownLeft ];

	eeSize dls;

	if ( NULL != tShape ) {
		dls = tShape->RealSize();

		tShape->Draw( X, Y + Height - dls.Height(), mColor[ mCurState ] );
	}

	tShape = mShape[ mCurState ][ UpRight ];

	eeSize urs;

	if ( NULL != tShape ) {
		urs = tShape->RealSize();

		tShape->Draw( X + Width - urs.Width(), Y, mColor[ mCurState ] );
	}

	tShape = mShape[ mCurState ][ DownRight ];

	eeSize drs;

	if ( NULL != tShape ) {
		drs = tShape->RealSize();

		tShape->Draw( X + Width - drs.Width(), Y + Height - drs.Height(), mColor[ mCurState ] );
	}

	tShape = mShape[ mCurState ][ Left ];

	if ( NULL != tShape ) {
		tShape->DestHeight( Height - uls.Height() - dls.Height() );

		tShape->Draw( X, Y + uls.Height(), mColor[ mCurState ] );

		tShape->ResetDestWidthAndHeight();
	}

	tShape = mShape[ mCurState ][ Up ];

	if ( NULL != tShape ) {
		tShape->DestWidth( Width - uls.Width() - urs.Width() );

		tShape->Draw( X + uls.Width(), Y, mColor[ mCurState ] );

		tShape->ResetDestWidthAndHeight();
	}

	tShape = mShape[ mCurState ][ Right ];

	if ( NULL != tShape ) {
		tShape->DestHeight( Height - urs.Height() - drs.Height() );

		tShape->Draw( X + Width - urs.Width(), Y + urs.Height(), mColor[ mCurState ] );

		tShape->ResetDestWidthAndHeight();
	}

	tShape = mShape[ mCurState ][ Down ];

	if ( NULL != tShape ) {
		tShape->DestWidth( Width - dls.Width() - drs.Width() );

		tShape->Draw( X + dls.Width(), Y + Height - tShape->RealSize().Height(), mColor[ mCurState ] );

		tShape->ResetDestWidthAndHeight();
	}

	tShape = mShape[ mCurState ][ Center ];

	if ( NULL != tShape ) {
		tShape->DestWidth( Width - uls.Width() - urs.Width() );
		tShape->DestHeight( Height - uls.Height() - urs.Height() );

		tShape->Draw( X + uls.Width(), Y + uls.Height(), mColor[ mCurState ] );

		tShape->ResetDestWidthAndHeight();
	}
}

void cUISkinComplex::SetSkin( const Uint32& State ) {
	eeASSERT ( State < cUISkin::StateCount );

	for ( Uint32 Side = 0; Side < SideCount; Side++ ) {

		cShape * Shape = cShapeGroupManager::instance()->GetShapeByName( std::string( mName + "_" + cUISkin::GetSkinStateName( State ) + "_" + SideSuffix[ Side ] ) );

		if ( NULL != Shape )
			mShape[ State ][ Side ] = Shape;
	}
}

cShape * cUISkinComplex::GetShape( const Uint32& State ) const {
	eeASSERT ( State < cUISkin::StateCount );

	return mShape[ State ][ 0 ];
}

void cUISkinComplex::SetState( const Uint32& State ) {
	eeASSERT ( State < cUISkin::StateCount );

	if ( mCurState == State )
		return;

	if ( !Read32BitKey( &mColorDefault, State ) || NULL != mShape[ State ][ 0 ] ) {
		StateNormalToState( State );

		mLastState 	= mCurState;
		mCurState 	= State;
	} else
		StateBack( State );
}

void cUISkinComplex::StateNormalToState( const Uint32& State ) {
	if ( NULL == mShape[ State ][ 0 ] ) {
		for ( Uint32 Side = 0; Side < SideCount; Side++ ) {
			mShape[ State ][ Side ] = mShape[ StateNormal ][ Side ];
		}
	}
}

cUISkinComplex * cUISkinComplex::Copy( const std::string& NewName, const bool& CopyColorsState ) {
	cUISkinComplex * SkinC = eeNew( cUISkinComplex, ( NewName ) );

	if ( CopyColorsState ) {
		SkinC->mColorDefault = mColorDefault;

		memcpy( &SkinC->mColor[0], &mColor[0], StateCount * sizeof(eeColorA) );
	}

	memcpy( &SkinC->mShape[0], &mShape[0], StateCount * SideCount * sizeof(cShape*) );

	return SkinC;
}

}}
