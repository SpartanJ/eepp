#include <eepp/ui/uiskincomplex.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>

namespace EE { namespace UI {

static const char SideSuffix[ UISkinComplex::SideCount ][4] = {
	"ml", "mr","d","u","ul","ur","dl","dr","m"
};

std::string UISkinComplex::GetSideSuffix( const Uint32& Side ) {
	eeASSERT( Side < UISkinComplex::SideCount );

	return std::string( SideSuffix[ Side ] );
}

UISkinComplex::UISkinComplex( const std::string& Name ) :
	UISkin( Name, SkinComplex )
{
	for ( Int32 x = 0; x < UISkinState::StateCount; x++ )
		for ( Int32 y = 0; y < SideCount; y++ )
			mSubTexture[ x ][ y ] = NULL;

	SetSkins();
}

UISkinComplex::~UISkinComplex() {

}

void UISkinComplex::Draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State ) {
	if ( 0 == Alpha )
		return;

	SubTexture * tSubTexture = mSubTexture[ State ][ UpLeft ];
	mTempColor		= mColor[ State ];

	if ( mTempColor.Alpha != Alpha ) {
		mTempColor.Alpha = (Uint8)( (Float)mTempColor.Alpha * ( (Float)Alpha / 255.f ) );
	}

	Sizei uls;

	if ( NULL != tSubTexture ) {
		uls = tSubTexture->realSize();

		tSubTexture->draw( X, Y, mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ DownLeft ];

	Sizei dls;

	if ( NULL != tSubTexture ) {
		dls = tSubTexture->realSize();

		tSubTexture->draw( X, Y + Height - dls.height(), mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ UpRight ];

	Sizei urs;

	if ( NULL != tSubTexture ) {
		urs = tSubTexture->realSize();

		tSubTexture->draw( X + Width - urs.width(), Y, mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ DownRight ];

	Sizei drs;

	if ( NULL != tSubTexture ) {
		drs = tSubTexture->realSize();

		tSubTexture->draw( X + Width - drs.width(), Y + Height - drs.height(), mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ Left ];

	if ( NULL != tSubTexture ) {
		tSubTexture->destSize( Sizef( tSubTexture->destSize().x, Height - uls.height() - dls.height() ) );

		tSubTexture->draw( X, Y + uls.height(), mTempColor );

		tSubTexture->resetDestSize();

		if ( uls.width() == 0 )
			uls.x = tSubTexture->realSize().width();
	}

	tSubTexture = mSubTexture[ State ][ Up ];

	if ( NULL != tSubTexture ) {
		tSubTexture->destSize( Sizef( Width - uls.width() - urs.width(), tSubTexture->destSize().y ) );

		tSubTexture->draw( X + uls.width(), Y, mTempColor );

		tSubTexture->resetDestSize();

		if ( urs.height() == 0 )
			urs.y = tSubTexture->realSize().height();

		if ( uls.height() == 0 )
			uls.y = tSubTexture->realSize().height();
	}

	tSubTexture = mSubTexture[ State ][ Right ];

	if ( NULL != tSubTexture ) {
		if ( urs.width() == 0 )
			urs.x = tSubTexture->realSize().width();

		tSubTexture->destSize( Sizef( tSubTexture->destSize().x, Height - urs.height() - drs.height() ) );

		tSubTexture->draw( X + Width - tSubTexture->realSize().width(), Y + urs.height(), mTempColor );

		tSubTexture->resetDestSize();
	}

	tSubTexture = mSubTexture[ State ][ Down ];

	if ( NULL != tSubTexture ) {
		tSubTexture->destSize( Sizef( Width - dls.width() - drs.width(), tSubTexture->destSize().y ) );

		tSubTexture->draw( X + dls.width(), Y + Height - tSubTexture->realSize().height(), mTempColor );

		tSubTexture->resetDestSize();

		if ( dls.height() == 0 && drs.height() == 0 )
			dls.height( tSubTexture->realSize().height() );
	}

	tSubTexture = mSubTexture[ State ][ Center ];

	if ( NULL != tSubTexture ) {
		tSubTexture->destSize( Sizef( Width - uls.width() - urs.width(), Height - uls.height() - dls.height() ) );

		tSubTexture->draw( X + uls.width(), Y + uls.height(), mTempColor );

		tSubTexture->resetDestSize();
	}
}

void UISkinComplex::SetSkin( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	for ( Uint32 Side = 0; Side < SideCount; Side++ ) {

		SubTexture * SubTexture = TextureAtlasManager::instance()->getSubTextureByName( std::string( mName + "_" + UISkin::GetSkinStateName( State ) + "_" + SideSuffix[ Side ] ) );

		if ( NULL != SubTexture )
			mSubTexture[ State ][ Side ] = SubTexture;
	}
}

SubTexture * UISkinComplex::GetSubTexture( const Uint32& State ) const {
	eeASSERT ( State < UISkinState::StateCount );

	return mSubTexture[ State ][ Center ];
}

SubTexture * UISkinComplex::GetSubTextureSide( const Uint32& State, const Uint32& Side ) {
	eeASSERT ( State < UISkinState::StateCount && Side < UISkinComplex::SideCount );

	return mSubTexture[ State ][ Side ];
}

void UISkinComplex::StateNormalToState( const Uint32& State ) {
	if ( NULL == mSubTexture[ State ][ 0 ] ) {
		for ( Uint32 Side = 0; Side < SideCount; Side++ ) {
			mSubTexture[ State ][ Side ] = mSubTexture[ UISkinState::StateNormal ][ Side ];
		}
	}
}

UISkinComplex * UISkinComplex::Copy( const std::string& NewName, const bool& CopyColorsState ) {
	UISkinComplex * SkinC = eeNew( UISkinComplex, ( NewName ) );

	if ( CopyColorsState ) {
		SkinC->mColorDefault = mColorDefault;

		memcpy( &SkinC->mColor[0], &mColor[0], UISkinState::StateCount * sizeof(ColorA) );
	}

	memcpy( &SkinC->mSubTexture[0], &mSubTexture[0], UISkinState::StateCount * SideCount * sizeof(SubTexture*) );

	return SkinC;
}

UISkin * UISkinComplex::Copy() {
	return Copy( mName, true );
}

}}
