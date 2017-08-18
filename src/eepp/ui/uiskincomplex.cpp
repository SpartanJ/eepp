#include <eepp/ui/uiskincomplex.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/drawablesearcher.hpp>

namespace EE { namespace UI {

static const char SideSuffix[ UISkinComplex::SideCount ][4] = {
	"ml", "mr","d","u","ul","ur","dl","dr","m"
};

UISkinComplex * UISkinComplex::New(const std::string & name) {
	return eeNew( UISkinComplex, ( name ) );
}

std::string UISkinComplex::getSideSuffix( const Uint32& Side ) {
	eeASSERT( Side < UISkinComplex::SideCount );

	return std::string( SideSuffix[ Side ] );
}

bool UISkinComplex::isSideSuffix( const std::string& suffix ) {
	for ( int i = 0; i < UISkinComplex::SideCount; i++ ) {
		if ( suffix == SideSuffix[i] ) {
			return true;
		}
	}

	return false;
}

UISkinComplex::UISkinComplex(const std::string& name ) :
	UISkin( name, SkinComplex )
{
	for ( Int32 x = 0; x < UISkinState::StateCount; x++ )
		for ( Int32 y = 0; y < SideCount; y++ )
			mDrawable[ x ][ y ] = NULL;

	setSkins();
	cacheSize();
}

UISkinComplex::~UISkinComplex() {

}

#define DRAWABLE_PX_SIZE PixelDensity::dpToPx( tDrawable->getSize() ).ceil()

void UISkinComplex::draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State ) {
	if ( 0 == Alpha )
		return;

	mTempColor		= mColor[ State ];

	if ( mTempColor.a != Alpha ) {
		mTempColor.a = (Uint8)( (Float)mTempColor.a * ( (Float)Alpha / 255.f ) );
	}

	Drawable * tDrawable = mDrawable[ State ][ UpLeft ];

	Sizef uls;

	if ( NULL != tDrawable ) {
		uls = DRAWABLE_PX_SIZE;

		tDrawable->setColor( mTempColor );
		tDrawable->draw( Vector2f( X, Y ), uls );
		tDrawable->clearColor();
	}

	tDrawable = mDrawable[ State ][ DownLeft ];

	Sizef dls;

	if ( NULL != tDrawable ) {
		dls = DRAWABLE_PX_SIZE;

		tDrawable->setColor( mTempColor );
		tDrawable->draw( Vector2f( X, Y + Height - dls.getHeight() ), dls );
		tDrawable->clearColor();
	}

	tDrawable = mDrawable[ State ][ UpRight ];

	Sizef urs;

	if ( NULL != tDrawable ) {
		urs = DRAWABLE_PX_SIZE;

		tDrawable->setColor( mTempColor );
		tDrawable->draw( Vector2f( X + Width - urs.getWidth() , Y ), urs );
		tDrawable->clearColor();
	}

	tDrawable = mDrawable[ State ][ DownRight ];

	Sizef drs;

	if ( NULL != tDrawable ) {
		drs = DRAWABLE_PX_SIZE;

		tDrawable->setColor( mTempColor );
		tDrawable->draw( Vector2f( X + Width - drs.getWidth(), Y + Height - drs.getHeight() ), drs );
		tDrawable->clearColor();
	}

	tDrawable = mDrawable[ State ][ Left ];

	if ( NULL != tDrawable ) {
		Sizef dpxs( DRAWABLE_PX_SIZE );
		Sizef ns( dpxs.getWidth(), Height - uls.getHeight() - dls.getHeight() );

		tDrawable->setColor( mTempColor );
		tDrawable->draw( Vector2f( X, Y + uls.getHeight() ), ns );
		tDrawable->clearColor();

		if ( uls.getWidth() == 0 )
			uls.x = dpxs.getWidth();
	}

	tDrawable = mDrawable[ State ][ Up ];

	if ( NULL != tDrawable ) {
		Sizef dpxs( DRAWABLE_PX_SIZE );
		Sizef ns( Width - uls.getWidth() - urs.getWidth(), dpxs.getHeight() );

		tDrawable->setColor( mTempColor );
		tDrawable->draw( Vector2f( X + uls.getWidth(), Y ), ns );
		tDrawable->clearColor();

		if ( urs.getHeight() == 0 )
			urs.y = dpxs.getHeight();

		if ( uls.getHeight() == 0 )
			uls.y = dpxs.getHeight();
	}

	tDrawable = mDrawable[ State ][ Right ];

	if ( NULL != tDrawable ) {
		Sizef dpxs( DRAWABLE_PX_SIZE );

		if ( urs.getWidth() == 0 )
			urs.x = dpxs.getWidth();

		Sizef ns( dpxs.x, Height - urs.getHeight() - drs.getHeight() );

		tDrawable->setColor( mTempColor );
		tDrawable->draw( Vector2f( X + Width - dpxs.getWidth(), Y + urs.getHeight() ), ns );
		tDrawable->clearColor();
	}

	tDrawable = mDrawable[ State ][ Down ];

	if ( NULL != tDrawable ) {
		Sizef dpxs( DRAWABLE_PX_SIZE );
		Sizef ns( Width - dls.getWidth() - drs.getWidth(), dpxs.getHeight() );

		tDrawable->setColor( mTempColor );
		tDrawable->draw( Vector2f( X + dls.getWidth(), Y + Height - dpxs.getHeight() ), ns );
		tDrawable->clearColor();

		if ( dls.getHeight() == 0 && drs.getHeight() == 0 )
			dls.setHeight( dpxs.getHeight() );
	}

	tDrawable = mDrawable[ State ][ Center ];

	if ( NULL != tDrawable ) {
		Sizef ns( Width - uls.getWidth() - urs.getWidth(), Height - uls.getHeight() - dls.getHeight() );

		tDrawable->setColor( mTempColor );
		tDrawable->draw( Vector2f( X + uls.getWidth(), Y + uls.getHeight() ), ns );
		tDrawable->clearColor();
	}
}

void UISkinComplex::setSkin( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	for ( Uint32 Side = 0; Side < SideCount; Side++ ) {
		Drawable * tDrawable = DrawableSearcher::searchByName( std::string( mName + "_" + UISkin::getSkinStateName( State ) + "_" + SideSuffix[ Side ] ) );

		if ( NULL != tDrawable )
			mDrawable[ State ][ Side ] = tDrawable;
	}
}

bool UISkinComplex::stateExists( const Uint32 & state ) {
	return NULL != mDrawable[ state ];
}

Sizei UISkinComplex::getSideSize( const Uint32& State, const Uint32& Side ) {
	eeASSERT ( State < UISkinState::StateCount && Side < UISkinComplex::SideCount );

	if ( NULL != mDrawable[ State ][ Side ] ) {
		Sizef ns( mDrawable[ State ][ Side ]->getSize() );
		return Sizei( (Int32)ns.x, (Int32)ns.y );
	}
	return Sizei();
}

void UISkinComplex::stateNormalToState( const Uint32& State ) {
	if ( NULL == mDrawable[ State ][ Center ] ) {
		for ( Uint32 Side = 0; Side < SideCount; Side++ ) {
			mDrawable[ State ][ Side ] = mDrawable[ UISkinState::StateNormal ][ Side ];
		}
	}
}

UISkinComplex * UISkinComplex::clone( const std::string& NewName, const bool& CopyColorsState ) {
	UISkinComplex * SkinC = UISkinComplex::New( NewName );

	if ( CopyColorsState ) {
		SkinC->mColorDefault = mColorDefault;

		memcpy( &SkinC->mColor[0], &mColor[0], UISkinState::StateCount * sizeof(Color) );
	}

	memcpy( &SkinC->mDrawable[0], &mDrawable[0], UISkinState::StateCount * SideCount * sizeof(Drawable*) );

	return SkinC;
}

UISkin * UISkinComplex::clone() {
	return clone( mName, true );
}

Sizei UISkinComplex::getSize( const Uint32 & state ) {
	return mSize[ state ];
}

Sizei UISkinComplex::getBorderSize(const Uint32 & state) {
	return mBorderSize[ state ];
}

void UISkinComplex::cacheSize() {
	for ( Int32 state = UISkinState::StateNormal; state < UISkinState::StateCount; state++ ) {
		Int32 w = 0, bw = 0;
		Int32 h = 0, bh = 0;

		Drawable * tDrawable = mDrawable[ state ][ Center ];

		if ( NULL != tDrawable ) {
			w += tDrawable->getSize().x;
			h += tDrawable->getSize().y;
		}

		tDrawable = mDrawable[ state ][ Up ];

		if ( NULL != tDrawable ) {
			h += tDrawable->getSize().y;
			bh += tDrawable->getSize().y;
		}

		tDrawable = mDrawable[ state ][ Down ];

		if ( NULL != tDrawable ) {
			h += tDrawable->getSize().y;
			bh += tDrawable->getSize().y;
		}

		tDrawable = mDrawable[ state ][ Left ];

		if ( NULL != tDrawable ) {
			w += tDrawable->getSize().x;
			bw += tDrawable->getSize().x;
		}

		tDrawable = mDrawable[ state ][ Right ];

		if ( NULL != tDrawable ) {
			w += tDrawable->getSize().x;
			bw += tDrawable->getSize().x;
		}

		mSize[ state ] = Sizei( w, h );
		mBorderSize[ state ] = Sizei( bw, bh );
	}
}

}}
