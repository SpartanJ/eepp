#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <array>
#include <eepp/audio/inputsoundfile.hpp>
#include <eepp/audio/outputsoundfile.hpp>

using namespace EE;
using namespace EE::Audio;

UTEST( VorbisSecurity, EncoderHandlesLowSampleRatesAndRejectsInvalidSetup ) {
	vfstest::TempDir temp;
	std::array<Int16, 512> samples{};
	for ( unsigned channels : { 1u, 2u } ) {
		for ( unsigned rate : { 1u, 1000u, 4000u, 8000u, 22050u, 44100u } ) {
			const std::string path = temp.path + "encoded.ogg";
			bool opened;
			{
				OutputSoundFile output;
				opened = output.openFromFile( path, rate, channels );
				if ( opened )
					output.write( samples.data(), samples.size() );
			}
			if ( rate >= 8000 )
				EXPECT_TRUE( opened );
			if ( opened ) {
				InputSoundFile input;
				ASSERT_TRUE( input.openFromFile( path ) );
				EXPECT_EQ( input.getSampleRate(), rate );
				EXPECT_EQ( input.getChannelCount(), channels );
				EXPECT_EQ( input.read( samples.data(), samples.size() ), samples.size() );
			}
		}
	}
	OutputSoundFile output;
	EXPECT_FALSE( output.openFromFile( temp.path + "invalid.ogg", 0, 1 ) );
	EXPECT_FALSE( output.openFromFile( temp.path + "invalid.ogg", 44100, 0 ) );
}
