#include <eepp/ui/uibackground.hpp>
#include <eepp/graphics/primitives.hpp>

using namespace EE::Graphics;

namespace EE { namespace UI {

UIBackground * UIBackground::New() {
	return eeNew( UIBackground, () );
}

UIBackground::UIBackground() :
	mBlendMode( ALPHA_NORMAL ),
	mCorners(0),
	mDrawable( NULL ),
	mOwnIt( false )
{
	mColor.push_back( Color::Transparent );
}

UIBackground::~UIBackground() {
	if ( mOwnIt )
		eeSAFE_DELETE( mDrawable );
}

Color& UIBackground::getColor( const unsigned int& index  ) {
	if ( index < mColor.size() )
		return	mColor[ index ];

	return mColor[ 0 ];
}

UIBackground * UIBackground::setColorsTo( const Color& Color ) {
	for ( unsigned int i = 0; i < mColor.size(); i++ )
		mColor[i] = Color;

	return this;
}

UIBackground * UIBackground::setColors( const Color& TopLeftColor, const Color& BottomLeftColor, const Color& BottomRightColor, const Color& TopRightColor ) {
	mColor[0] = TopLeftColor;

	if ( mColor.size() < 2 )
		mColor.push_back( BottomLeftColor );
	else
		mColor[1] = BottomLeftColor;

	if ( mColor.size() < 3 )
		mColor.push_back( BottomRightColor );
	else
		mColor[2] = BottomRightColor;

	if ( mColor.size() < 4 )
		mColor.push_back( TopRightColor );
	else
		mColor[3] = TopRightColor;

	return this;
}

const std::vector<Color>& UIBackground::getColors() {
	return	mColor;
}

UIBackground * UIBackground::setColor( const Color& Col ) {
	mColor[0] = Col;
	return this;
}

const EE_BLEND_MODE& UIBackground::getBlendMode() const {
	return mBlendMode;
}

UIBackground * UIBackground::setBlendMode( const EE_BLEND_MODE& blend ) {
	mBlendMode = blend;
	return this;
}

const unsigned int& UIBackground::getCorners() const {
	return mCorners;
}

UIBackground * UIBackground::setCorners( const unsigned int& corners ) {
	mCorners = corners;
	return this;
}

void UIBackground::draw( Rectf R, const Float& alpha ) {
	if ( mColor[0] != Color::Transparent || mColor.size() > 1 ) {
		Primitives P;
		P.setBlendMode( mBlendMode );

		if ( 255 == alpha ) {
			P.setColor( mColor[0] );

			if ( 4 == mColor.size() ) {
				if ( mCorners ) {
					P.drawRoundedRectangle( R, mColor[0], mColor[1], mColor[2], mColor[3], mCorners );
				} else {
					P.drawRectangle( R, mColor[0], mColor[1], mColor[2], mColor[3] );
				}
			} else {
				if ( mCorners ) {
					P.drawRoundedRectangle( R, 0.f, Vector2f::One, mCorners );
				} else {
					P.drawRectangle( R );
				}
			}
		} else {
			std::vector<Color> color( mColor );

			color[0].a = static_cast<Uint8>( (Float)color[0].a * ( alpha / 255.f ) );

			P.setColor( color[0] );

			if ( 4 == mColor.size() ) {
				for ( size_t i = 0; i < color.size(); i++ )
					color[i].a = static_cast<Uint8>( (Float)color[i].a * ( alpha / 255.f ) );

				if ( mCorners ) {
					P.drawRoundedRectangle( R, color[0], color[1], color[2], color[3], mCorners );
				} else {
					P.drawRectangle( R, color[0], color[1], color[2], color[3] );
				}
			} else {
				if ( mCorners ) {
					P.drawRoundedRectangle( R, 0.f, Vector2f::One, mCorners );
				} else {
					P.drawRectangle( R );
				}
			}
		}
	}

	if ( NULL != mDrawable ) {
		mDrawable->draw( R.getPosition(), R.getSize() );
	}
}

Drawable * UIBackground::getDrawable() const {
	return mDrawable;
}

void UIBackground::setDrawable( Drawable * drawable , bool ownIt ) {
	if ( mOwnIt )
		eeSAFE_DELETE( mDrawable );

	mDrawable = drawable;
	mOwnIt = ownIt;
}

}}
