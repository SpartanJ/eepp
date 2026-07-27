#include "utest.h"

#include <eepp/ui/border.hpp>

using namespace EE;
using namespace EE::UI;

static BorderRadiuses equalRadiuses( Float x, Float y ) {
	return { { x, y }, { x, y }, { x, y }, { x, y } };
}

#define EXPECT_RADIUS( radiusValue, expectedX, expectedY ) \
	do {                                                   \
		EXPECT_NEAR( expectedX, radiusValue.x, 0.001f );   \
		EXPECT_NEAR( expectedY, radiusValue.y, 0.001f );   \
	} while ( false )

UTEST( Borders, NormalizeEqualCircularRadiuses ) {
	const BorderRadiuses input = equalRadiuses( 9999.f, 9999.f );
	const auto square = Borders::normalizeRadiuses( input, { 100.f, 100.f } );
	const auto capsule = Borders::normalizeRadiuses( input, { 200.f, 100.f } );

	EXPECT_RADIUS( square.topLeft, 50.f, 50.f );
	EXPECT_RADIUS( square.topRight, 50.f, 50.f );
	EXPECT_RADIUS( square.bottomRight, 50.f, 50.f );
	EXPECT_RADIUS( square.bottomLeft, 50.f, 50.f );
	EXPECT_RADIUS( capsule.topLeft, 50.f, 50.f );
	EXPECT_RADIUS( capsule.topRight, 50.f, 50.f );
	EXPECT_RADIUS( capsule.bottomRight, 50.f, 50.f );
	EXPECT_RADIUS( capsule.bottomLeft, 50.f, 50.f );
}

UTEST( Borders, NormalizeAsymmetricHorizontalRadiusesProportionally ) {
	BorderRadiuses input;
	input.topLeft = { 80.f, 12.f };
	input.topRight = { 40.f, 6.f };

	const auto used = Borders::normalizeRadiuses( input, { 100.f, 100.f } );
	EXPECT_RADIUS( used.topLeft, 66.6667f, 10.f );
	EXPECT_RADIUS( used.topRight, 33.3333f, 5.f );
}

UTEST( Borders, NormalizeUsesSmallestEdgeConstraintForEveryComponent ) {
	BorderRadiuses bottomLimited = { { 10.f, 5.f }, { 20.f, 6.f }, { 60.f, 8.f }, { 90.f, 7.f } };
	const auto bottom = Borders::normalizeRadiuses( bottomLimited, { 100.f, 100.f } );
	EXPECT_RADIUS( bottom.topLeft, 100.f / 15.f, 10.f / 3.f );
	EXPECT_RADIUS( bottom.bottomLeft, 40.f, 16.f / 3.f );

	BorderRadiuses leftLimited = { { 12.f, 60.f }, { 8.f, 10.f }, { 4.f, 90.f }, { 6.f, 20.f } };
	const auto left = Borders::normalizeRadiuses( leftLimited, { 200.f, 100.f } );
	EXPECT_RADIUS( left.topRight, 16.f / 3.f, 20.f / 3.f );
	EXPECT_RADIUS( left.bottomLeft, 8.f / 3.f, 60.f );

	BorderRadiuses rightLimited = { { 5.f, 10.f }, { 10.f, 80.f }, { 15.f, 20.f }, { 20.f, 40.f } };
	const auto right = Borders::normalizeRadiuses( rightLimited, { 200.f, 100.f } );
	EXPECT_RADIUS( right.topLeft, 25.f / 6.f, 25.f / 3.f );
	EXPECT_RADIUS( right.topRight, 25.f / 3.f, 200.f / 3.f );
}

UTEST( Borders, NormalizePreservesEllipticalAspectRatio ) {
	const BorderRadiuses input = equalRadiuses( 120.f, 30.f );
	const auto used = Borders::normalizeRadiuses( input, { 100.f, 200.f } );
	EXPECT_RADIUS( used.topLeft, 50.f, 12.5f );
	EXPECT_NEAR( input.topLeft.x / input.topLeft.y, used.topLeft.x / used.topLeft.y, 0.001f );
}

UTEST( Borders, NormalizeHandlesZeroAndNegativeRadiuses ) {
	BorderRadiuses input;
	input.topLeft = { -10.f, -20.f };
	const auto used = Borders::normalizeRadiuses( input, { 100.f, 100.f } );
	EXPECT_RADIUS( used.topLeft, 0.f, 0.f );
	EXPECT_RADIUS( used.topRight, 0.f, 0.f );
}

UTEST( Borders, NormalizeResizeStartsFromSpecifiedRadiuses ) {
	const BorderRadiuses specified = equalRadiuses( 80.f, 80.f );
	const auto small = Borders::normalizeRadiuses( specified, { 100.f, 100.f } );
	const auto large = Borders::normalizeRadiuses( specified, { 200.f, 200.f } );

	EXPECT_RADIUS( small.topLeft, 50.f, 50.f );
	EXPECT_RADIUS( large.topLeft, 80.f, 80.f );
	EXPECT_RADIUS( specified.topLeft, 80.f, 80.f );
}

#undef EXPECT_RADIUS
