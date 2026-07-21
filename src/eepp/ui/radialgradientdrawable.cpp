#include <algorithm>
#include <cmath>

#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/math/math.hpp>
#include <eepp/ui/radialgradientdrawable.hpp>

namespace EE { namespace UI {

RadialGradientDrawable* RadialGradientDrawable::New() {
	return eeNew( RadialGradientDrawable, () );
}

RadialGradientDrawable* RadialGradientDrawable::NewRepeating() {
	return eeNew( RadialGradientDrawable, ( Graphics::Drawable::REPEATINGRADIALGRADIENT ) );
}

RadialGradientDrawable::RadialGradientDrawable( Graphics::Drawable::Type drawableType ) :
	Drawable( drawableType ) {}

Sizef RadialGradientDrawable::getSize() {
	return mSize;
}

Sizef RadialGradientDrawable::getPixelsSize() {
	return mSize;
}

void RadialGradientDrawable::draw() {
	draw( mPosition, mSize );
}

void RadialGradientDrawable::draw( const Vector2f& position ) {
	draw( position, mSize );
}

static Float computeRadius( Float cx, Float cy, Float w, Float h,
							RadialGradientDrawable::Extent extent ) {
	Float dTL = std::sqrt( cx * cx + cy * cy );
	Float dTR = std::sqrt( ( w - cx ) * ( w - cx ) + cy * cy );
	Float dBL = std::sqrt( cx * cx + ( h - cy ) * ( h - cy ) );
	Float dBR = std::sqrt( ( w - cx ) * ( w - cx ) + ( h - cy ) * ( h - cy ) );

	switch ( extent ) {
		case RadialGradientDrawable::CLOSEST_SIDE:
			return eemin( eemin( cx, w - cx ), eemin( cy, h - cy ) );
		case RadialGradientDrawable::FARTHEST_SIDE:
			return eemax( eemax( cx, w - cx ), eemax( cy, h - cy ) );
		case RadialGradientDrawable::CLOSEST_CORNER:
			return eemin( eemin( dTL, dTR ), eemin( dBL, dBR ) );
		case RadialGradientDrawable::FARTHEST_CORNER:
		default:
			return eemax( eemax( dTL, dTR ), eemax( dBL, dBR ) );
	}
}

void RadialGradientDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( mColorStops.size() < 2 || size.getWidth() <= 0 || size.getHeight() <= 0 )
		return;

	Float w = size.getWidth();
	Float h = size.getHeight();

	Float cx = mCenter.x * w;
	Float cy = mCenter.y * h;
	Float maxR = computeRadius( cx, cy, w, h, mExtent );

	// Normalize all stops to [0,1] using the gradient ray pixel length
	std::vector<ColorStop> stops;
	stops.reserve( mColorStops.size() );
	for ( const auto& s : mColorStops )
		stops.push_back(
			ColorStop( s.getNormalized( maxR ), s.color, CSS::StyleSheetLength::Percentage ) );

	std::sort( stops.begin(), stops.end(),
			   []( const ColorStop& a, const ColorStop& b ) { return a.value < b.value; } );


	const int SEGMENTS = 48;
	Float angleStep = 2.f * EE_PI / (Float)SEGMENTS;

	BatchRenderer* sBR = GlobalBatchRenderer::instance();
	sBR->setTexture( NULL );
	sBR->setBlendMode( BlendMode::Alpha() );
	sBR->quadsBegin();

	Float posX = position.x;
	Float posY = position.y;

	Float cosVals[SEGMENTS + 1];
	Float sinVals[SEGMENTS + 1];
	for ( int j = 0; j <= SEGMENTS; j++ ) {
		Float a = (Float)j * angleStep;
		cosVals[j] = std::cos( a );
		sinVals[j] = std::sin( a );
	}

	if ( isRepeating() ) {
		Float firstPos = stops.front().value;
		Float lastPos = stops.back().value;
		Float patternLen = lastPos - firstPos;
		if ( patternLen < 0.0001f ) {
			sBR->draw();
			return;
		}

		int rStart = (int)std::floor( ( -firstPos ) / patternLen );
		int rEnd = (int)std::ceil( ( 1.f - lastPos ) / patternLen );
		const int MAX_REPEATS = 64;
		if ( rEnd - rStart > MAX_REPEATS )
			rEnd = rStart + MAX_REPEATS;

		for ( int r = rStart; r <= rEnd; r++ ) {
			Float repeatOff = (Float)r * patternLen;

			for ( size_t i = 0; i + 1 < stops.size(); i++ ) {
				Float p0 = stops[i].value + repeatOff;
				Float p1 = stops[i + 1].value + repeatOff;

				if ( p1 <= 0.f || p0 >= 1.f )
					continue;

				Float clip0 = eemax( 0.f, p0 );
				Float clip1 = eemin( 1.f, p1 );
				if ( clip1 - clip0 < 0.0001f )
					continue;

				Float r0 = clip0 * maxR;
				Float r1 = clip1 * maxR;

				Float bw = p1 - p0;
				Float frac0 = ( clip0 - p0 ) / bw;
				Float frac1 = ( clip1 - p0 ) / bw;
				const Color& sc0 = stops[i].color;
				const Color& sc1 = stops[i + 1].color;
				Color cc0(
					(Uint8)( (Float)sc0.r + frac0 * (Float)( sc1.r - sc0.r ) ),
					(Uint8)( (Float)sc0.g + frac0 * (Float)( sc1.g - sc0.g ) ),
					(Uint8)( (Float)sc0.b + frac0 * (Float)( sc1.b - sc0.b ) ),
					(Uint8)( (Float)sc0.a + frac0 * (Float)( sc1.a - sc0.a ) ) );
				Color cc1(
					(Uint8)( (Float)sc0.r + frac1 * (Float)( sc1.r - sc0.r ) ),
					(Uint8)( (Float)sc0.g + frac1 * (Float)( sc1.g - sc0.g ) ),
					(Uint8)( (Float)sc0.b + frac1 * (Float)( sc1.b - sc0.b ) ),
					(Uint8)( (Float)sc0.a + frac1 * (Float)( sc1.a - sc0.a ) ) );

				Color fc0 = ( mColor.a == 255 ) ? cc0 : Color( cc0 ).blendAlpha( mColor.a );
				Color fc1 = ( mColor.a == 255 ) ? cc1 : Color( cc1 ).blendAlpha( mColor.a );
				sBR->quadsSetColorFree( fc0, fc1, fc1, fc0 );

				for ( int j = 0; j < SEGMENTS; j++ ) {
					Float cj = cosVals[j], sj = sinVals[j];
					Float cj1 = cosVals[j + 1], sj1 = sinVals[j + 1];

					sBR->batchQuadFree( cx + r0 * cj + posX, cy + r0 * sj + posY,
										cx + r1 * cj + posX, cy + r1 * sj + posY,
										cx + r1 * cj1 + posX, cy + r1 * sj1 + posY,
										cx + r0 * cj1 + posX, cy + r0 * sj1 + posY );
				}
			}
		}
	} else {
		for ( size_t i = 0; i + 1 < stops.size(); i++ ) {
			Float r0 = stops[i].value * maxR;
			Float r1 = stops[i + 1].value * maxR;

			const Color& c0 = stops[i].color;
			const Color& c1 = stops[i + 1].color;
			Color fc0 = ( mColor.a == 255 ) ? c0 : Color( c0 ).blendAlpha( mColor.a );
			Color fc1 = ( mColor.a == 255 ) ? c1 : Color( c1 ).blendAlpha( mColor.a );

			sBR->quadsSetColorFree( fc0, fc1, fc1, fc0 );

			for ( int j = 0; j < SEGMENTS; j++ ) {
				Float cj = cosVals[j], sj = sinVals[j];
				Float cj1 = cosVals[j + 1], sj1 = sinVals[j + 1];

				sBR->batchQuadFree( cx + r0 * cj + posX, cy + r0 * sj + posY,
									cx + r1 * cj + posX, cy + r1 * sj + posY,
									cx + r1 * cj1 + posX, cy + r1 * sj1 + posY,
									cx + r0 * cj1 + posX, cy + r0 * sj1 + posY );
			}
		}
	}

	sBR->draw();
}

const std::vector<RadialGradientDrawable::ColorStop>&
RadialGradientDrawable::getColorStops() const {
	return mColorStops;
}

void RadialGradientDrawable::setColorStops( std::vector<ColorStop> stops ) {
	mColorStops = std::move( stops );
}

RadialGradientDrawable::ShapeType RadialGradientDrawable::getShape() const {
	return mShape;
}

void RadialGradientDrawable::setShape( ShapeType shape ) {
	mShape = shape;
}

RadialGradientDrawable::Extent RadialGradientDrawable::getExtent() const {
	return mExtent;
}

void RadialGradientDrawable::setExtent( Extent extent ) {
	mExtent = extent;
}

const Vector2f& RadialGradientDrawable::getCenter() const {
	return mCenter;
}

void RadialGradientDrawable::setCenter( const Vector2f& centerNormalized ) {
	mCenter = centerNormalized;
}

void RadialGradientDrawable::setSize( const Sizef& size ) {
	mSize = size;
}

bool RadialGradientDrawable::isRepeating() const {
	return mDrawableType == Graphics::Drawable::REPEATINGRADIALGRADIENT;
}

}} // namespace EE::UI
