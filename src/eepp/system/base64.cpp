#include <eepp/system/base64.hpp>

namespace EE { namespace System {

/* base64.c : base-64 / MIME encode/decode */
/* PUBLIC DOMAIN - Jon Mayo - November 13, 2003 */
/* $Id: base64.c 156 2007-07-12 23:29:10Z orange $ */

/* decode a base64 string in one shot */
int Base64::Decode(size_t in_len, const char *in, size_t out_len, unsigned char *out) {
	static const Uint8 base64dec_tab[256] = {
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
		 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
		255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
		255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	};

	unsigned ii, io;
	Uint32 v;
	unsigned rem;

	for(io=0,ii=0,v=0,rem=0;ii<in_len;ii++) {
		unsigned char ch;
		if(isspace(in[ii])) continue;
		if(in[ii]=='=') break; /* stop at = */
		ch=base64dec_tab[(unsigned)in[ii]];
		if(ch==255) break; /* stop at a parse error */
		v=(v<<6)|ch;
		rem+=6;
		if(rem>=8) {
			rem-=8;
			if(io>=out_len) return -1; /* truncation is failure */
			out[io++]=(v>>rem)&255;
		}
	}
	if(rem>=8) {
		rem-=8;
		if(io>=out_len) return -1; /* truncation is failure */
		out[io++]=(v>>rem)&255;
	}
	return io;
}

int Base64::Encode(size_t in_len, const unsigned char *in, size_t out_len, char *out) {
	static const Uint8 base64enc_tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned ii, io;
	Uint32 v;
	unsigned rem;

	for(io=0,ii=0,v=0,rem=0;ii<in_len;ii++) {
		unsigned char ch;
		ch=in[ii];
		v=(v<<8)|ch;
		rem+=8;
		while(rem>=6) {
			rem-=6;
			if(io>=out_len) return -1; /* truncation is failure */
			out[io++]=base64enc_tab[(v>>rem)&63];
		}
	}
	if(rem) {
		v<<=(6-rem);
		if(io>=out_len) return -1; /* truncation is failure */
		out[io++]=base64enc_tab[v&63];
	}
	while(io&3) {
		if(io>=out_len) return -1; /* truncation is failure */
		out[io++]='=';
	}
	if(io>=out_len) return -1; /* no room for null terminator */
	out[io]=0;
	return io;
}

bool Base64::Encode(const std::string & in, std::string & out) {
	size_t b64len = EncodeSafeOutLen( in.size() );

	if ( out.size() < b64len ) {
		out.resize( b64len );
	}

	int len = Encode( in.size(), (const unsigned char*)in.c_str(), out.size(), (char*)&out[0] );

	if ( -1 != len && (size_t)len != out.size() ) {
		out.resize( len );
	}

	return -1 != len;
}

bool Base64::Decode(const std::string & in, std::string & out) {
	size_t d64len = DecodeSafeOutLen( in.size() );

	if ( out.size() < d64len ) {
		out.resize( d64len );
	}

	int len = Decode( in.size(), in.c_str(), out.size(), (unsigned char*)&out[0] );

	if ( -1 != len && (size_t)len != out.size() ) {
		out.resize( len );
	}

	return -1 != len;
}

}}
