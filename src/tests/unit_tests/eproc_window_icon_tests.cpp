#include "../../tools/eproc/window_icon.hpp"
#include "utest.hpp"

using namespace eproc;

UTEST( EProcWindowIcon, ChoosesNearestUsableSizeAndConvertsArgbToRgba ) {
	const unsigned long property[] = {
		1, 1, 0xFF112233,									 // 1x1
		2, 2, 0x80445566, 0xFF000000, 0xFFFFFFFF, 0x00010203 // 2x2
	};
	WindowIconPixels icon =
		decodeWindowIcon( property, sizeof( property ) / sizeof( property[0] ), 2 );
	ASSERT_TRUE( icon.valid() );
	EXPECT_EQ( icon.width, 2u );
	EXPECT_EQ( icon.height, 2u );
	EXPECT_EQ( icon.rgba.size(), 16u );
	EXPECT_EQ( icon.rgba[0], 0x44 );
	EXPECT_EQ( icon.rgba[1], 0x55 );
	EXPECT_EQ( icon.rgba[2], 0x66 );
	EXPECT_EQ( icon.rgba[3], 0x80 );
	EXPECT_EQ( icon.rgba[15], 0x00 );
}

UTEST( EProcWindowIcon, RejectsTruncatedOrInvalidProperty ) {
	const unsigned long truncated[] = { 32, 32, 0xFFFFFFFF };
	EXPECT_FALSE( decodeWindowIcon( truncated, 3, 16 ).valid() );
	const unsigned long zeroWidth[] = { 0, 16 };
	EXPECT_FALSE( decodeWindowIcon( zeroWidth, 2, 16 ).valid() );
	EXPECT_FALSE( decodeWindowIcon( nullptr, 0, 16 ).valid() );
}
