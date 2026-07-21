#include <algorithm>
#include <cmath>

#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/math/math.hpp>
#include <eepp/ui/lineargradientdrawable.hpp>

namespace EE { namespace UI {

LinearGradientDrawable* LinearGradientDrawable::New() {
	return eeNew( LinearGradientDrawable, () );
}

LinearGradientDrawable* LinearGradientDrawable::NewRepeating() {
	return eeNew( LinearGradientDrawable, ( Graphics::Drawable::REPEATINGLINEARGRADIENT ) );
}

LinearGradientDrawable::LinearGradientDrawable( Graphics::Drawable::Type drawableType ) :
	Drawable( drawableType ) {}

Sizef LinearGradientDrawable::getSize() {
	return mSize;
}

Sizef LinearGradientDrawable::getPixelsSize() {
	return mSize;
}

void LinearGradientDrawable::draw() {
	draw( mPosition, mSize );
}

void LinearGradientDrawable::draw( const Vector2f& position ) {
	draw( position, mSize );
}

void LinearGradientDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( mColorStops.size() < 2 || size.getWidth() <= 0 || size.getHeight() <= 0 )
		return;

	Float w = size.getWidth();
	Float h = size.getHeight();

	Float aRad = mAngle * EE_PI / 180.f;
	Float dx = std::sin( aRad );
	Float dy = -std::cos( aRad );

	Float cx = w * 0.5f;
	Float cy = h * 0.5f;

	// CSS: gradient-line endpoints are found by projecting each corner
	// of the box onto the gradient direction.  For diagonal angles this
	// gives a corner-to-corner line, whereas intersecting with the four
	// edges would produce a shorter line — and degenerate perpendicular
	// segments near the corners.
	Float corners[4][2] = { { 0, 0 }, { w, 0 }, { w, h }, { 0, h } };
	Float tMin = ( corners[0][0] - cx ) * dx + ( corners[0][1] - cy ) * dy;
	Float tMax = tMin;
	for ( int i = 1; i < 4; i++ ) {
		Float t = ( corners[i][0] - cx ) * dx + ( corners[i][1] - cy ) * dy;
		if ( t < tMin )
			tMin = t;
		if ( t > tMax )
			tMax = t;
	}

	if ( std::abs( tMax - tMin ) < 0.0001f )
		return;

	Float txLen = tMax - tMin;

	// Normalize all stops to [0,1] using the gradient-line pixel length
	std::vector<ColorStop> stops;
	stops.reserve( mColorStops.size() );
	for ( const auto& s : mColorStops )
		stops.push_back(
			ColorStop( s.getNormalized( txLen ), s.color, CSS::StyleSheetLength::Percentage ) );

	std::sort( stops.begin(), stops.end(),
			   []( const ColorStop& a, const ColorStop& b ) { return a.value < b.value; } );

	Float px = -dy;
	Float py = dx;

	BatchRenderer* sBR = GlobalBatchRenderer::instance();
	sBR->setTexture( NULL );
	sBR->setBlendMode( BlendMode::Alpha() );
	sBR->quadsBegin();

	// Helper: compute a perpendicular segment that always spans the box.
	// When the perpendicular line through (gx,gy) only grazes a corner we
	// extend the segment so the quad-to-quad tiling fills the whole box.
	auto perpSeg = [&]( Float gx, Float gy ) {
		Float ds[4];
		// signed dist along perpendicular from (gx,gy) to each corner
		ds[0] = ( -gx ) * px + ( -gy ) * py;
		ds[1] = ( w - gx ) * px + ( -gy ) * py;
		ds[2] = ( w - gx ) * px + ( h - gy ) * py;
		ds[3] = ( -gx ) * px + ( h - gy ) * py;
		Float dMin = ds[0];
		Float dMax = ds[0];
		for ( int i = 1; i < 4; i++ ) {
			if ( ds[i] < dMin )
				dMin = ds[i];
			if ( ds[i] > dMax )
				dMax = ds[i];
		}
		Vector2f a( gx + dMin * px + position.x, gy + dMin * py + position.y );
		Vector2f b( gx + dMax * px + position.x, gy + dMax * py + position.y );
		return std::make_pair( a, b );
	};

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
		const int MAX_REPEATS = 128;
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

				Float bw = p1 - p0;
				Float frac0 = ( clip0 - p0 ) / bw;
				Float frac1 = ( clip1 - p0 ) / bw;
				const Color& sc0 = stops[i].color;
				const Color& sc1 = stops[i + 1].color;
				Color cc0( (Uint8)( (Float)sc0.r + frac0 * (Float)( sc1.r - sc0.r ) ),
						   (Uint8)( (Float)sc0.g + frac0 * (Float)( sc1.g - sc0.g ) ),
						   (Uint8)( (Float)sc0.b + frac0 * (Float)( sc1.b - sc0.b ) ),
						   (Uint8)( (Float)sc0.a + frac0 * (Float)( sc1.a - sc0.a ) ) );
				Color cc1( (Uint8)( (Float)sc0.r + frac1 * (Float)( sc1.r - sc0.r ) ),
						   (Uint8)( (Float)sc0.g + frac1 * (Float)( sc1.g - sc0.g ) ),
						   (Uint8)( (Float)sc0.b + frac1 * (Float)( sc1.b - sc0.b ) ),
						   (Uint8)( (Float)sc0.a + frac1 * (Float)( sc1.a - sc0.a ) ) );

				Float tc0 = tMin + clip0 * txLen;
				Float tc1 = tMin + clip1 * txLen;
				Float gx0 = cx + tc0 * dx;
				Float gy0 = cy + tc0 * dy;
				Float gx1 = cx + tc1 * dx;
				Float gy1 = cy + tc1 * dy;

				auto seg0 = perpSeg( gx0, gy0 );
				Vector2f a0 = seg0.first;
				Vector2f b0 = seg0.second;

				auto seg1 = perpSeg( gx1, gy1 );
				Vector2f a1 = seg1.first;
				Vector2f b1 = seg1.second;

				Color fc0 = ( mColor.a == 255 ) ? cc0 : Color( cc0 ).blendAlpha( mColor.a );
				Color fc1 = ( mColor.a == 255 ) ? cc1 : Color( cc1 ).blendAlpha( mColor.a );
				sBR->quadsSetColorFree( fc0, fc1, fc1, fc0 );
				sBR->batchQuadFree( a0.x, a0.y, a1.x, a1.y, b1.x, b1.y, b0.x, b0.y );
			}
		}
	} else {
		// CSS: first and last color extend to the box boundaries (pad mode)
		if ( stops.front().value > 0.f )
			stops.insert( stops.begin(), ColorStop( 0.f, stops.front().color ) );
		if ( stops.back().value < 1.f )
			stops.push_back( ColorStop( 1.f, stops.back().color ) );

		struct PerpSeg {
			Vector2f a;
			Vector2f b;
		};
		std::vector<PerpSeg> segments;
		segments.reserve( stops.size() );

		for ( size_t i = 0; i < stops.size(); i++ ) {
			Float t = tMin + stops[i].value * txLen;
			Float gx = cx + t * dx;
			Float gy = cy + t * dy;
			auto segPair = perpSeg( gx, gy );
			PerpSeg seg;
			seg.a = segPair.first;
			seg.b = segPair.second;
			segments.push_back( seg );
		}

		for ( size_t i = 0; i < stops.size() - 1; i++ ) {
			const Color& c0 = stops[i].color;
			const Color& c1 = stops[i + 1].color;

			Color fc0 = ( mColor.a == 255 ) ? c0 : Color( c0 ).blendAlpha( mColor.a );
			Color fc1 = ( mColor.a == 255 ) ? c1 : Color( c1 ).blendAlpha( mColor.a );

			sBR->quadsSetColorFree( fc0, fc1, fc1, fc0 );

			const PerpSeg& s0 = segments[i];
			const PerpSeg& s1 = segments[i + 1];

			sBR->batchQuadFree( s0.a.x, s0.a.y, s1.a.x, s1.a.y, s1.b.x, s1.b.y, s0.b.x, s0.b.y );
		}
	}

	sBR->draw();
}

const std::vector<LinearGradientDrawable::ColorStop>&
LinearGradientDrawable::getColorStops() const {
	return mColorStops;
}

void LinearGradientDrawable::setColorStops( std::vector<ColorStop> stops ) {
	mColorStops = std::move( stops );
}

Float LinearGradientDrawable::getAngle() const {
	return mAngle;
}

void LinearGradientDrawable::setAngle( Float angleDegrees ) {
	mAngle = angleDegrees;
}

void LinearGradientDrawable::setSize( const Sizef& size ) {
	mSize = size;
}

bool LinearGradientDrawable::isRepeating() const {
	return mDrawableType == Graphics::Drawable::REPEATINGLINEARGRADIENT;
}

}} // namespace EE::UI
