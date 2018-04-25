#ifndef EE_AUDIO_MP3INFO_HPP
#define EE_AUDIO_MP3INFO_HPP

#include <eepp/config.hpp>
#include <eepp/system/iostream.hpp>
using namespace EE::System;

namespace EE { namespace Audio {

#define MIN_CONSEC_GOOD_FRAMES 4
#define FRAME_HEADER_SIZE 4
#define MIN_FRAME_SIZE 21

class Mp3Info {
	public:
		class Header {
			public:
				unsigned long	sync;
				unsigned int	version;
				unsigned int	layer;
				unsigned int	crc;
				unsigned int	bitrate;
				unsigned int	freq;
				unsigned int	padding;
				unsigned int	extension;
				unsigned int	mode;
				unsigned int	mode_extension;
				unsigned int	copyright;
				unsigned int	original;
				unsigned int	emphasis;
		};

		class Info {
			public:
				off_t datasize;
				int header_isvalid;
				Header header;
				int id3_isvalid;
				int vbr;
				float vbr_average;
				int seconds;
				int frames;
				int badframes;
		};

		Mp3Info( IOStream& stream );

		Info getInfo();

		int getFrequency();

		int getBitrate();

		bool isValidMp3();
	protected:
		IOStream& mStream;
		Info mInfo;
		bool mValidMp3;
		bool mFetchedInfo;

		bool fetchInfo();
		bool getFirstHeader(long startpos);
		int getHeader(Header * header);
		int getNextHeader();
};

}}

#endif
