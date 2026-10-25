#include "exif_test_utils.hpp"
#include "utest.hpp"
#include <array>
#include <cstring>
#include <eepp/graphics/exif.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/iostreamstring.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::System;

using exiftest::tiff;

UTEST( Exif, AllOrientationsAndByteOrdersPreservePosition ) {
	for ( bool little : { false, true } ) {
		for ( Uint16 value = 1; value <= 8; ++value ) {
			auto bytes = tiff( value, little );
			IOStreamMemory stream( bytes.data(), bytes.size() );
			stream.seek( 7 );
			EXPECT_EQ( static_cast<int>( Exif::read( stream ).orientation ), int( value ) );
			EXPECT_EQ( stream.tell(), 7 );
		}
	}
}

UTEST( Exif, JPEGPNGAndWebPContainers ) {
	auto payload = tiff( 6, true );
	std::string jpeg( "\xff\xd8\xff\xe1\0\x22"
					  "Exif\0\0",
					  12 );
	jpeg += payload;
	IOStreamMemory j( jpeg.data(), jpeg.size() );
	EXPECT_TRUE( Exif::read( j ).orientation == ImageOrientation::Rotate90 );
	std::string png( "\x89PNG\r\n\x1a\n\0\0\0\x1a"
					 "eXIf",
					 16 );
	png += payload;
	png.append( 4, '\0' );
	IOStreamMemory p( png.data(), png.size() );
	EXPECT_TRUE( Exif::read( p ).orientation == ImageOrientation::Rotate90 );
	std::string webp( "RIFF\0\0\0\0WEBPEXIF\x1a\0\0\0", 20 );
	webp += payload;
	IOStreamMemory w( webp.data(), webp.size() );
	EXPECT_TRUE( Exif::read( w ).orientation == ImageOrientation::Rotate90 );
}

UTEST( Exif, MalformedOffsetsCountsAndValuesAreIgnored ) {
	auto bytes = tiff( 9, true );
	IOStreamMemory stream( bytes.data(), bytes.size() );
	EXPECT_TRUE( Exif::read( stream ).orientation == ImageOrientation::Normal );
	for ( size_t size = 0; size < bytes.size(); ++size ) {
		IOStreamMemory truncated( bytes.data(), size );
		EXPECT_TRUE( Exif::read( truncated ).orientation == ImageOrientation::Normal );
	}
	bytes[4] = char( 0xff );
	EXPECT_TRUE( Exif::read( stream ).orientation == ImageOrientation::Normal );
	bytes = tiff( 6, true );
	bytes[8] = bytes[9] = char( 0xff );
	IOStreamMemory invalid( bytes.data(), bytes.size() );
	EXPECT_TRUE( Exif::read( invalid ).orientation == ImageOrientation::Normal );
}

UTEST( Exif, EmbeddedJPEGBoundsAndIndependentThumbnailOrientation ) {
	auto bytes = tiff( 6, true );
	bytes.resize( 86, '\0' );
	bytes[22] = 26; // IFD1 starts after IFD0.
	bytes[26] = 4;
	const auto entry = [&]( size_t pos, Uint16 tag, Uint16 type, Uint32 value ) {
		bytes[pos] = tag & 255;
		bytes[pos + 1] = tag >> 8;
		bytes[pos + 2] = type;
		bytes[pos + 4] = 1;
		for ( int i = 0; i < 4; ++i )
			bytes[pos + 8 + i] = char( value >> ( 8 * i ) );
	};
	entry( 28, 0x103, 3, 6 );
	entry( 40, 0x112, 3, 1 );
	entry( 52, 0x201, 4, 80 );
	entry( 64, 0x202, 4, 6 );
	bytes[80] = char( 0xff );
	bytes[81] = char( 0xd8 );
	IOStreamMemory stream( bytes.data(), bytes.size() );
	auto metadata = Exif::read( stream );
	EXPECT_TRUE( metadata.orientation == ImageOrientation::Rotate90 );
	EXPECT_TRUE( metadata.thumbnailOrientation == ImageOrientation::Normal );
	EXPECT_EQ( metadata.thumbnailOffset, Uint64{ 80 } );
	EXPECT_EQ( metadata.thumbnailLength, Uint64{ 6 } );
	bytes[72] = char( 0xff ); // Thumbnail length exceeds the metadata payload.
	EXPECT_FALSE( Exif::read( stream ).hasThumbnail() );
}

UTEST( Image, PixelOrientationMatchesAllEightEXIFTransforms ) {
	const Uint8 pixels[] = { 1, 2, 3, 4, 5, 6 };
	const std::array<std::array<Uint8, 6>, 8> expected{ { { { 1, 2, 3, 4, 5, 6 } },
														  { { 3, 2, 1, 6, 5, 4 } },
														  { { 6, 5, 4, 3, 2, 1 } },
														  { { 4, 5, 6, 1, 2, 3 } },
														  { { 1, 4, 2, 5, 3, 6 } },
														  { { 4, 1, 5, 2, 6, 3 } },
														  { { 6, 3, 5, 2, 4, 1 } },
														  { { 3, 6, 2, 5, 1, 4 } } } };
	for ( int value = 1; value <= 8; ++value ) {
		Image image( pixels, 3, 2, 1 );
		image.applyOrientation( static_cast<ImageOrientation>( value ) );
		EXPECT_EQ( image.getWidth(), value >= 5 ? 2u : 3u );
		EXPECT_EQ( image.getHeight(), value >= 5 ? 3u : 2u );
		EXPECT_EQ( std::memcmp( image.getPixelsPtr(), expected[value - 1].data(), 6 ), 0 );
	}
}
