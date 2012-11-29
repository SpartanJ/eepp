#include <eepp/ui/cuiskincomplex.hpp>
#include <eepp/graphics/ctextureatlasmanager.hpp>

namespace EE { namespace UI {

static const char SideSuffix[ cUISkinComplex::SideCount ][4] = {
	"ml", "mr","d","u","ul","ur","dl","dr","m"
};

std::string cUISkinComplex::GetSideSuffix( const Uint32& Side ) {
	eeASSERT( Side < cUISkinComplex::SideCount );

	return std::string( SideSuffix[ Side ] );
}

cUISkinComplex::cUISkinComplex( const std::string& Name ) :
	cUISkin( Name, UISkinComplex )
{
	for ( Int32 x = 0; x < cUISkinState::StateCount; x++ )
		for ( Int32 y = 0; y < SideCount; y++ )
			mSubTexture[ x ][ y ] = NULL;

	SetSkins();
}

cUISkinComplex::~cUISkinComplex() {

}

void cUISkinComplex::Draw( const eeFloat& X, const eeFloat& Y, const eeFloat& Width, const eeFloat& Height, const Uint32& Alpha, const Uint32& State ) {
	if ( 0 == Alpha )
		return;

	cSubTexture * tSubTexture = mSubTexture[ State ][ UpLeft ];
	mTempColor		= mColor[ State ];

	if ( mTempColor.Alpha != Alpha ) {
		mTempColor.Alpha = (Uint8)( (eeFloat)mTempColor.Alpha * ( (eeFloat)Alpha / 255.f ) );
	}

	eeSize uls;

	if ( NULL != tSubTexture ) {
		uls = tSubTexture->RealSize();

		tSubTexture->Draw( X, Y, mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ DownLeft ];

	eeSize dls;

	if ( NULL != tSubTexture ) {
		dls = tSubTexture->RealSize();

		tSubTexture->Draw( X, Y + Height - dls.Height(), mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ UpRight ];

	eeSize urs;

	if ( NULL != tSubTexture ) {
		urs = tSubTexture->RealSize();

		tSubTexture->Draw( X + Width - urs.Width(), Y, mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ DownRight ];

	eeSize drs;

	if ( NULL != tSubTexture ) {
		drs = tSubTexture->RealSize();

		tSubTexture->Draw( X + Width - drs.Width(), Y + Height - drs.Height(), mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ Left ];

	if ( NULL != tSubTexture ) {
		tSubTexture->DestHeight( Height - uls.Height() - dls.Height() );

		tSubTexture->Draw( X, Y + uls.Height(), mTempColor );

		tSubTexture->ResetDestWidthAndHeight();

		if ( uls.Width() == 0 )
			uls.x = tSubTexture->RealSize().Width();
	}

	tSubTexture = mSubTexture[ State ][ Up ];

	if ( NULL != tSubTexture ) {
		tSubTexture->DestWidth( Width - uls.Width() - urs.Width() );

		tSubTexture->Draw( X + uls.Width(), Y, mTempColor );

		tSubTexture->ResetDestWidthAndHeight();

		if ( urs.Height() == 0 )
			urs.y = tSubTexture->RealSize().Height();

		if ( uls.Height() == 0 )
			uls.y = tSubTexture->RealSize().Height();
	}

	tSubTexture = mSubTexture[ State ][ Right ];

	if ( NULL != tSubTexture ) {
		if ( urs.Width() == 0 )
			urs.x = tSubTexture->RealSize().Width();

		tSubTexture->DestHeight( Height - urs.Height() - drs.Height() );

		tSubTexture->Draw( X + Width - tSubTexture->RealSize().Width(), Y + urs.Height(), mTempColor );

		tSubTexture->ResetDestWidthAndHeight();
	}

	tSubTexture = mSubTexture[ State ][ Down ];

	if ( NULL != tSubTexture ) {
		tSubTexture->DestWidth( Width - dls.Width() - drs.Width() );

		tSubTexture->Draw( X + dls.Width(), Y + Height - tSubTexture->RealSize().Height(), mTempColor );

		tSubTexture->ResetDestWidthAndHeight();

		if ( dls.Height() == 0 && drs.Height() == 0 )
			dls.Height( tSubTexture->RealSize().Height() );
	}

	tSubTexture = mSubTexture[ State ][ Center ];

	if ( NULL != tSubTexture ) {
		tSubTexture->DestWidth( Width - uls.Width() - urs.Width() );
		tSubTexture->DestHeight( Height - uls.Height() - dls.Height() );

		tSubTexture->Draw( X + uls.Width(), Y + uls.Height(), mTempColor );

		tSubTexture->ResetDestWidthAndHeight();
	}
}

void cUISkinComplex::SetSkin( const Uint32& State ) {
	eeASSERT ( State < cUISkinState::StateCount );

	for ( Uint32 Side = 0; Side < SideCount; Side++ ) {

		cSubTexture * SubTexture = cTextureAtlasManager::instance()->GetSubTextureByName( std::string( mName + "_" + cUISkin::GetSkinStateName( State ) + "_" + SideSuffix[ Side ] ) );

		if ( NULL != SubTexture )
			mSubTexture[ State ][ Side ] = SubTexture;
	}
}

cSubTexture * cUISkinComplex::GetSubTexture( const Uint32& State ) const {
	eeASSERT ( State < cUISkinState::StateCount );

	return mSubTexture[ State ][ Center ];
}

cSubTexture * cUISkinComplex::GetSubTextureSide( const Uint32& State, const Uint32& Side ) {
	eeASSERT ( State < cUISkinState::StateCount && Side < cUISkinComplex::SideCount );

	return mSubTexture[ State ][ Side ];
}

void cUISkinComplex::StateNormalToState( const Uint32& State ) {
	if ( NULL == mSubTexture[ State ][ 0 ] ) {
		for ( Uint32 Side = 0; Side < SideCount; Side++ ) {
			mSubTexture[ State ][ Side ] = mSubTexture[ cUISkinState::StateNormal ][ Side ];
		}
	}
}

cUISkinComplex * cUISkinComplex::Copy( const std::string& NewName, const bool& CopyColorsState ) {
	cUISkinComplex * SkinC = eeNew( cUISkinComplex, ( NewName ) );

	if ( CopyColorsState ) {
		SkinC->mColorDefault = mColorDefault;

		memcpy( &SkinC->mColor[0], &mColor[0], cUISkinState::StateCount * sizeof(eeColorA) );
	}

	memcpy( &SkinC->mSubTexture[0], &mSubTexture[0], cUISkinState::StateCount * SideCount * sizeof(cSubTexture*) );

	return SkinC;
}

cUISkin * cUISkinComplex::Copy() {
	return Copy( mName, true );
}

}}
