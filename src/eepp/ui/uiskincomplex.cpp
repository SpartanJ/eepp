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

	setSkins();
	cacheSize();
}

UISkinComplex::~UISkinComplex() {

}

void UISkinComplex::draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State ) {
	if ( 0 == Alpha )
		return;

	SubTexture * tSubTexture = mSubTexture[ State ][ UpLeft ];
	mTempColor		= mColor[ State ];

	if ( mTempColor.Alpha != Alpha ) {
		mTempColor.Alpha = (Uint8)( (Float)mTempColor.Alpha * ( (Float)Alpha / 255.f ) );
	}

	Sizei uls;

	if ( NULL != tSubTexture ) {
		uls = tSubTexture->getRealSize();

		tSubTexture->draw( X, Y, mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ DownLeft ];

	Sizei dls;

	if ( NULL != tSubTexture ) {
		dls = tSubTexture->getRealSize();

		tSubTexture->draw( X, Y + Height - dls.getHeight(), mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ UpRight ];

	Sizei urs;

	if ( NULL != tSubTexture ) {
		urs = tSubTexture->getRealSize();

		tSubTexture->draw( X + Width - urs.getWidth(), Y, mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ DownRight ];

	Sizei drs;

	if ( NULL != tSubTexture ) {
		drs = tSubTexture->getRealSize();

		tSubTexture->draw( X + Width - drs.getWidth(), Y + Height - drs.getHeight(), mTempColor );
	}

	tSubTexture = mSubTexture[ State ][ Left ];

	if ( NULL != tSubTexture ) {
		tSubTexture->setDestSize( Sizef( tSubTexture->getDestSize().x, Height - uls.getHeight() - dls.getHeight() ) );

		tSubTexture->draw( X, Y + uls.getHeight(), mTempColor );

		tSubTexture->resetDestSize();

		if ( uls.getWidth() == 0 )
			uls.x = tSubTexture->getRealSize().getWidth();
	}

	tSubTexture = mSubTexture[ State ][ Up ];

	if ( NULL != tSubTexture ) {
		tSubTexture->setDestSize( Sizef( Width - uls.getWidth() - urs.getWidth(), tSubTexture->getDestSize().y ) );

		tSubTexture->draw( X + uls.getWidth(), Y, mTempColor );

		tSubTexture->resetDestSize();

		if ( urs.getHeight() == 0 )
			urs.y = tSubTexture->getRealSize().getHeight();

		if ( uls.getHeight() == 0 )
			uls.y = tSubTexture->getRealSize().getHeight();
	}

	tSubTexture = mSubTexture[ State ][ Right ];

	if ( NULL != tSubTexture ) {
		if ( urs.getWidth() == 0 )
			urs.x = tSubTexture->getRealSize().getWidth();

		tSubTexture->setDestSize( Sizef( tSubTexture->getDestSize().x, Height - urs.getHeight() - drs.getHeight() ) );

		tSubTexture->draw( X + Width - tSubTexture->getRealSize().getWidth(), Y + urs.getHeight(), mTempColor );

		tSubTexture->resetDestSize();
	}

	tSubTexture = mSubTexture[ State ][ Down ];

	if ( NULL != tSubTexture ) {
		tSubTexture->setDestSize( Sizef( Width - dls.getWidth() - drs.getWidth(), tSubTexture->getDestSize().y ) );

		tSubTexture->draw( X + dls.getWidth(), Y + Height - tSubTexture->getRealSize().getHeight(), mTempColor );

		tSubTexture->resetDestSize();

		if ( dls.getHeight() == 0 && drs.getHeight() == 0 )
			dls.setHeight( tSubTexture->getRealSize().getHeight() );
	}

	tSubTexture = mSubTexture[ State ][ Center ];

	if ( NULL != tSubTexture ) {
		tSubTexture->setDestSize( Sizef( Width - uls.getWidth() - urs.getWidth(), Height - uls.getHeight() - dls.getHeight() ) );

		tSubTexture->draw( X + uls.getWidth(), Y + uls.getHeight(), mTempColor );

		tSubTexture->resetDestSize();
	}
}

void UISkinComplex::setSkin( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	for ( Uint32 Side = 0; Side < SideCount; Side++ ) {
		SubTexture * SubTexture = TextureAtlasManager::instance()->getSubTextureByName( std::string( mName + "_" + UISkin::getSkinStateName( State ) + "_" + SideSuffix[ Side ] ) );

		if ( NULL != SubTexture )
			mSubTexture[ State ][ Side ] = SubTexture;
	}
}

SubTexture * UISkinComplex::getSubTexture( const Uint32& State ) const {
	eeASSERT ( State < UISkinState::StateCount );

	return mSubTexture[ State ][ Center ];
}

SubTexture * UISkinComplex::getSubTextureSide( const Uint32& State, const Uint32& Side ) {
	eeASSERT ( State < UISkinState::StateCount && Side < UISkinComplex::SideCount );

	return mSubTexture[ State ][ Side ];
}

void UISkinComplex::stateNormalToState( const Uint32& State ) {
	if ( NULL == mSubTexture[ State ][ Center ] ) {
		for ( Uint32 Side = 0; Side < SideCount; Side++ ) {
			mSubTexture[ State ][ Side ] = mSubTexture[ UISkinState::StateNormal ][ Side ];
		}
	}
}

UISkinComplex * UISkinComplex::clone( const std::string& NewName, const bool& CopyColorsState ) {
	UISkinComplex * SkinC = eeNew( UISkinComplex, ( NewName ) );

	if ( CopyColorsState ) {
		SkinC->mColorDefault = mColorDefault;

		memcpy( &SkinC->mColor[0], &mColor[0], UISkinState::StateCount * sizeof(ColorA) );
	}

	memcpy( &SkinC->mSubTexture[0], &mSubTexture[0], UISkinState::StateCount * SideCount * sizeof(SubTexture*) );

	return SkinC;
}

UISkin * UISkinComplex::clone() {
	return clone( mName, true );
}

Sizei UISkinComplex::getSize( const Uint32 & state ) {
	return mSize[ state ];
}

void UISkinComplex::cacheSize() {
	for ( Int32 state = UISkinState::StateNormal; state < UISkinState::StateCount; state++ ) {
		Int32 w = 0;
		Int32 h = 0;

		SubTexture * tSubTexture = mSubTexture[ state ][ Center ];

		if ( NULL != tSubTexture ) {
			w += tSubTexture->getRealSize().x;
			h += tSubTexture->getRealSize().y;
		}

		tSubTexture = mSubTexture[ state ][ Up ];

		if ( NULL != tSubTexture ) {
			h += tSubTexture->getRealSize().y;
		}

		tSubTexture = mSubTexture[ state ][ Down ];

		if ( NULL != tSubTexture ) {
			h += tSubTexture->getRealSize().y;
		}

		tSubTexture = mSubTexture[ state ][ Left ];

		if ( NULL != tSubTexture ) {
			w += tSubTexture->getRealSize().x;
		}

		tSubTexture = mSubTexture[ state ][ Right ];

		if ( NULL != tSubTexture ) {
			w += tSubTexture->getRealSize().x;
		}

		mSize[ state ] = Sizei( w, h );
	}
}

}}
