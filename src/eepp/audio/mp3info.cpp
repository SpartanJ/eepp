#include <eepp/audio/mp3info.hpp>
#include <eepp/core.hpp>

namespace EE { namespace Audio {

/* Inpired in the mp3info library */

static const int frame_size_index[] = {24000, 72000, 72000};

static const int frequencies[3][4] = {
	{22050,24000,16000,50000},  /* MPEG 2.0 */
	{44100,48000,32000,50000},  /* MPEG 1.0 */
	{11025,12000,8000,50000}    /* MPEG 2.5 */
};

static const int bitrate[2][3][15] = {
	{ /* MPEG 2.0 */
		{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},  /* layer 1 */
		{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},       /* layer 2 */
		{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}        /* layer 3 */
	},

	{ /* MPEG 1.0 */
		{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448}, /* layer 1 */
		{0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},    /* layer 2 */
		{0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}      /* layer 3 */
	}
};

static bool is_same_constant(Mp3Info::Header *h1, Mp3Info::Header *h2) {
	if ((*(uint*)h1) == (*(uint*)h2))
		return true;

	return ((h1->version       == h2->version         ) && (h1->layer         == h2->layer           ) &&
			(h1->crc           == h2->crc             ) && (h1->freq          == h2->freq            ) &&
			(h1->mode          == h2->mode            ) && (h1->copyright     == h2->copyright       ) &&
			(h1->original      == h2->original        ) && (h1->emphasis      == h2->emphasis        ));
}

static int header_frequency(Mp3Info::Header *h) {
	return frequencies[h->version][h->freq];
}

static int header_bitrate(Mp3Info::Header *h) {
	return bitrate[h->version & 1][3-h->layer][h->bitrate];
}

static int frame_length(Mp3Info::Header *header) {
	return header->sync == 0xFFE ? (frame_size_index[3-header->layer]*((header->version&1)+1)*header_bitrate(header)/header_frequency(header))+header->padding : 1;
}

Mp3Info::Mp3Info( IOStream& stream ) :
	mStream( stream ),
	mValidInfo( false )
{
	memset(&mInfo,0,sizeof(Info));

	fetchInfo();
}

Mp3Info::Info Mp3Info::getInfo() {
	return mInfo;
}

int Mp3Info::getFrequency() {
	return header_frequency( &mInfo.header );
}

int Mp3Info::getBitrate() {
	return header_bitrate( &mInfo.header );
}

int Mp3Info::fetchInfo() {
	int had_error = 0;
	int frame_type[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	float seconds=0,total_rate=0;
	int frames=0,frame_types=0,frames_so_far=0;
	int vbr_median=-1;
	int bitrate;
	int counter=0;
	Header header;

	mInfo.datasize = mStream.getSize();

	if( getFirstHeader( 0L ) ) {
		while( ( bitrate = getNextHeader() ) ) {
			frame_type[15-bitrate]++;
			frames++;
		}

		memcpy( &header, &(mInfo.header), sizeof(Header) );

		for(counter=0;counter<15;counter++) {
			if(frame_type[counter]) {
				frame_types++;
				header.bitrate=counter;
				frames_so_far += frame_type[counter];
				seconds += (float)(frame_length(&header)*frame_type[counter])/
						   (float)(header_bitrate(&header)*125);
				total_rate += (float)((header_bitrate(&header))*frame_type[counter]);
				if((vbr_median == -1) && (frames_so_far >= frames/2))
					vbr_median=counter;
			}
		}

		mInfo.seconds=(int)(seconds+0.5);
		mInfo.header.bitrate=vbr_median;
		mInfo.vbr_average=total_rate/(float)frames;
		mInfo.frames=frames;

		if(frame_types > 1) {
			mInfo.vbr=1;
		}
	}

	return had_error;
}

int Mp3Info::getFirstHeader( long startpos ) {
	int k, l=0;
	unsigned char val;
	int c;
	Header h, h2;
	long valid_start=0;

	mStream.seek( startpos );

	while ( 1 ) {
		if ( mStream.tell() < mInfo.datasize ) {
			mStream.read( (char*)&val, 1 ); c = val;
		} else {
			c = EOF;
		}

		while( c != 255 ) {
			if ( mStream.tell() >= mInfo.datasize ) {
				c = EOF;
				break;
			}

			mStream.read( (char*)&val, 1 );
			c = val;
		}

		 if ( c == 255 ) {
			if ( mStream.tell() > 0 && mStream.tell() != mStream.getSize() ) mStream.seek( mStream.tell() - 1 );

			valid_start = mStream.tell();

			if( ( l = getHeader( &h ) ) ) {
				mStream.seek( mStream.tell() + l - FRAME_HEADER_SIZE );

				for( k = 1; ( k < MIN_CONSEC_GOOD_FRAMES ) && (mInfo.datasize - mStream.tell() >= FRAME_HEADER_SIZE ); k++) {
					if ( !( l = getHeader( &h2 ) ) ) break;
					if ( !is_same_constant( &h, &h2 ) ) break;
					mStream.seek( mStream.tell() + l - FRAME_HEADER_SIZE );
				}

				if( k == MIN_CONSEC_GOOD_FRAMES ) {
					mStream.seek( valid_start );
					memcpy( &(mInfo.header), &h2, sizeof(Header) );
					mInfo.header_isvalid=1;
					return 1;
				}
			}
		 } else {
			return 0;
		 }
	}

	return 0;
}

int Mp3Info::getHeader(Header *header) {
	unsigned char buffer[FRAME_HEADER_SIZE];
	int fl;

	if( mStream.read((char*)&buffer,FRAME_HEADER_SIZE) < 1 ) {
		header->sync=0;
		return 0;
	}

	header->sync=(((int)buffer[0]<<4) | ((int)(buffer[1]&0xE0)>>4));

	if( buffer[1] & 0x10 ) {
		header->version=(buffer[1] >> 3) & 1;
	} else {
		header->version=2;
	}

	header->layer=(buffer[1] >> 1) & 3;
	header->bitrate=(buffer[2] >> 4) & 0x0F;

	if((header->sync != 0xFFE) || (header->layer != 1) || (header->bitrate == 0xF)) {
		header->sync=0;
		return 0;
	}

	header->crc=buffer[1] & 1;
	header->freq=(buffer[2] >> 2) & 0x3;
	header->padding=(buffer[2] >>1) & 0x1;
	header->extension=(buffer[2]) & 0x1;
	header->mode=(buffer[3] >> 6) & 0x3;
	header->mode_extension=(buffer[3] >> 4) & 0x3;
	header->copyright=(buffer[3] >> 3) & 0x1;
	header->original=(buffer[3] >> 2) & 0x1;
	header->emphasis=(buffer[3]) & 0x3;

	/* Final sanity checks: bitrate 1111b and frequency 11b are reserved (invalid) */
	if (header->bitrate == 0x0F || header->freq == 0x3) {
		return 0;
	}

	return ((fl=frame_length(header)) >= MIN_FRAME_SIZE ? fl : 0);
}

int Mp3Info::getNextHeader() {
	int l=0,skip_bytes=0;
	unsigned char val;
	int c;
	Header h;

	while( 1 ) {
		if ( mStream.tell() < mInfo.datasize ) {
			mStream.read( (char*)&val, 1 ); c = val;
		} else {
			c = EOF;
		}

		while( c != 255 ) {
			if ( mStream.tell() >= mInfo.datasize ) {
				c = EOF;
				break;
			}

			skip_bytes++;
			mStream.read( (char*)&val, 1 ); c = val;
		}

		if( c == 255 ) {
			if ( mStream.tell() > 0 && mStream.tell() != mStream.getSize() ) mStream.seek( mStream.tell() - 1 );

			if( ( l = getHeader(&h) ) ) {
				if(skip_bytes) mInfo.badframes++;

				mStream.seek( mStream.tell() + l - FRAME_HEADER_SIZE );

				return 15 - h.bitrate;
			} else {
				skip_bytes += FRAME_HEADER_SIZE;
			}
		} else {
			if(skip_bytes) mInfo.badframes++;
			return 0;
		}
	}
}

}}
