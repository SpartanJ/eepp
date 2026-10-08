#include "utest.h"

#include <eepp/graphics/image.hpp>

using namespace EE;
using namespace EE::Graphics;

UTEST( ImageDiff, ExactEqualityRGB ) {
	const Uint8 pixels[] = { 12, 34, 56, 78, 90, 123 };
	const Image image1( pixels, 2, 1, 3 );
	const Image image2( pixels, 2, 1, 3 );

	Image::DiffResult visualResult = image1.diff( image2 );
	EXPECT_EQ( visualResult.numDifferentPixels, 0 );
	EXPECT_EQ( visualResult.maxDeltaE, 0.0 );
	ASSERT_TRUE( visualResult.diffImage != nullptr );

	Image::DiffResult compareResult = image1.diff( image2, 2.3f, Color( 255, 0, 255, 255 ), false );
	EXPECT_EQ( compareResult.numDifferentPixels, 0 );
	EXPECT_EQ( compareResult.maxDeltaE, 0.0 );
	EXPECT_TRUE( compareResult.diffImage == nullptr );
}

UTEST( ImageDiff, ExactEqualityRGBA ) {
	const Uint8 pixels[] = { 12, 34, 56, 78 };
	const Image image1( pixels, 1, 1, 4 );
	const Image image2( pixels, 1, 1, 4 );

	Image::DiffResult visualResult = image1.diff( image2 );
	EXPECT_EQ( visualResult.numDifferentPixels, 0 );
	EXPECT_EQ( visualResult.maxDeltaE, 0.0 );
	ASSERT_TRUE( visualResult.diffImage != nullptr );

	Image::DiffResult compareResult = image1.diff( image2, 2.3f, Color( 255, 0, 255, 255 ), false );
	EXPECT_EQ( compareResult.numDifferentPixels, 0 );
	EXPECT_EQ( compareResult.maxDeltaE, 0.0 );
	EXPECT_TRUE( compareResult.diffImage == nullptr );
}

UTEST( ImageDiff, AlphaOnlyDifference ) {
	const Uint8 pixels1[] = { 12, 34, 56, 78 };
	const Uint8 pixels2[] = { 12, 34, 56, 79 };
	const Image image1( pixels1, 1, 1, 4 );
	const Image image2( pixels2, 1, 1, 4 );

	Image::DiffResult result = image1.diff( image2, 2.3f, Color( 255, 0, 255, 255 ), false );
	EXPECT_EQ( result.numDifferentPixels, 1 );
	EXPECT_EQ( result.maxDeltaE, 0.0 );
	EXPECT_TRUE( result.diffImage == nullptr );
}

UTEST( ImageDiff, RGBDifferenceThreshold ) {
	const Uint8 pixels1[] = { 255, 0, 0 };
	const Uint8 pixels2[] = { 0, 0, 255 };
	const Image image1( pixels1, 1, 1, 3 );
	const Image image2( pixels2, 1, 1, 3 );

	Image::DiffResult belowThreshold =
		image1.diff( image2, 1000.f, Color( 255, 0, 255, 255 ), false );
	EXPECT_EQ( belowThreshold.numDifferentPixels, 0 );
	EXPECT_TRUE( belowThreshold.maxDeltaE > 0.0 );

	Image::DiffResult aboveThreshold = image1.diff( image2, 0.f, Color( 255, 0, 255, 255 ), false );
	EXPECT_EQ( aboveThreshold.numDifferentPixels, 1 );
	EXPECT_EQ( aboveThreshold.maxDeltaE, belowThreshold.maxDeltaE );
}

UTEST( ImageDiff, DiffImageRendering ) {
	const Uint8 pixels1[] = { 30, 60, 90, 255, 255, 0, 0, 255 };
	const Uint8 pixels2[] = { 30, 60, 90, 255, 0, 0, 255, 255 };
	const Color diffColor( 1, 2, 3, 4 );
	const Image image1( pixels1, 2, 1, 4 );
	const Image image2( pixels2, 2, 1, 4 );

	Image::DiffResult result = image1.diff( image2, 0.f, diffColor, true );
	ASSERT_TRUE( result.diffImage != nullptr );
	EXPECT_EQ( result.numDifferentPixels, 1 );
	const Uint8* diffPixels = result.diffImage->getPixelsPtr();
	EXPECT_EQ( diffPixels[0], 60 );
	EXPECT_EQ( diffPixels[1], 60 );
	EXPECT_EQ( diffPixels[2], 60 );
	EXPECT_EQ( diffPixels[3], 128 );
	EXPECT_EQ( diffPixels[4], diffColor.r );
	EXPECT_EQ( diffPixels[5], diffColor.g );
	EXPECT_EQ( diffPixels[6], diffColor.b );
	EXPECT_EQ( diffPixels[7], diffColor.a );
}

UTEST( ImageDiff, DifferentDimensions ) {
	const Image image1( 2, 3, 3 );
	const Image image2( 1, 1, 3 );

	Image::DiffResult result = image1.diff( image2 );
	EXPECT_EQ( result.numDifferentPixels, 6 );
	EXPECT_EQ( result.maxDeltaE, 0.0 );
	EXPECT_TRUE( result.diffImage == nullptr );
}
