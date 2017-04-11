#include <eepp/ui/uiskinsimple.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/drawablesearcher.hpp>

namespace EE { namespace UI {

UISkinSimple * UISkinSimple::New( const std::string& name ) {
	return eeNew( UISkinSimple, ( name ) );
}

UISkinSimple::UISkinSimple(const std::string& name ) :
	UISkin( name, SkinSimple )
{
	for ( Int32 i = 0; i < UISkinState::StateCount; i++ )
		mDrawable[ i ] = NULL;

	setSkins();
}

UISkinSimple::~UISkinSimple() {
}

void UISkinSimple::draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha, const Uint32& State ) {
	if ( 0 == Alpha )
		return;

	Drawable * tDrawable = mDrawable[ State ];
	mTempColor	= mColor[ State ];

	if ( NULL != tDrawable ) {
		if ( mTempColor.a != Alpha ) {
			mTempColor.a = (Uint8)( (Float)mTempColor.a * ( (Float)Alpha / 255.f ) );
		}

		tDrawable->setColor( mTempColor );
		tDrawable->draw( Vector2f( X, Y ), Sizef( Width, Height ) );
		tDrawable->clearColor();
	}
}

void UISkinSimple::setSkin( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	std::string Name( mName + "_" + UISkin::getSkinStateName( State ) );

	mDrawable[ State ] = DrawableSearcher::searchByName( Name );
}

bool UISkinSimple::stateExists( const Uint32 & state ) {
	return NULL != mDrawable[ state ];
}

void UISkinSimple::stateNormalToState( const Uint32& State ) {
	if ( NULL == mDrawable[ State ] )
		mDrawable[ State ] = mDrawable[ UISkinState::StateNormal ];
}

UISkinSimple * UISkinSimple::clone( const std::string& NewName, const bool& CopyColorsState ) {
	UISkinSimple * SkinS = UISkinSimple::New( NewName );

	if ( CopyColorsState ) {
		SkinS->mColorDefault = mColorDefault;

		memcpy( &SkinS->mColor[0], &mColor[0], UISkinState::StateCount * sizeof(Color) );
	}

	memcpy( &SkinS->mDrawable[0], &mDrawable[0], UISkinState::StateCount * sizeof(Drawable*) );

	return SkinS;
}

UISkin * UISkinSimple::clone() {
	return clone( mName, true );
}

Sizei UISkinSimple::getSize( const Uint32 & state ) {
	if ( NULL != mDrawable[ state ] ) {
		Sizef s( mDrawable[ state ]->getSize() );
		return Sizei( (Int32)s.x, (Int32)s.y );
	}

	return Sizei();
}

Sizei UISkinSimple::getBorderSize( const Uint32 & state ) {
	return Sizei();
}

}}
