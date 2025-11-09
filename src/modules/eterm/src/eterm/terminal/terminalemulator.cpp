// The MIT License (MIT)

// Copyright (c) 2020 Fredrik A. Kristiansen
// Copyright (c) 2014 - 2020 Hiltjo Posthuma<hiltjo at codemadness dot org>
// Copyright (c) 2018 Devin J.Pohly<djpohly at gmail dot com>
// Copyright (c) 2014 - 2017 Quentin Rameau<quinq at fifth dot space>
// Copyright (c) 2009 - 2012 Aurélien APTEL<aurelien dot aptel at gmail dot com>
// Copyright (c) 2008 - 2017 Anselm R Garbe<garbeam at gmail dot com>
// Copyright (c) 2012 - 2017 Roberto E.Vargas Caballero<k0ga at shike2 dot com>
// Copyright (c) 2012 - 2016 Christoph Lohmann<20h at r - 36 dot net>
// Copyright (c) 2013 Eon S.Jeon<esjeon at hyunmu dot am>
// Copyright (c) 2013 Alexander Sedov<alex0player at gmail dot com>
// Copyright (c) 2013 Mark Edgar<medgar123 at gmail dot com>
// Copyright (c) 2013 - 2014 Eric Pruitt<eric.pruitt at gmail dot com>
// Copyright (c) 2013 Michael Forney<mforney at mforney dot org>
// Copyright (c) 2013 - 2014 Markus Teich<markus dot teich at stusta dot mhn dot de>
// Copyright (c) 2014 - 2015 Laslo Hunhold<dev at frign dot de>

//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
#include <eterm/terminal/boxdrawdata.hpp>
#include <eterm/terminal/terminalemulator.hpp>

#include <eepp/core/memorymanager.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

using namespace EE::Network;
using namespace EE::System;

#include <assert.h>
#include <cmath>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <memory.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if EE_PLATFORM == EE_PLATFORM_LINUX
// For malloc_trim, which is a GNU extension
extern "C" {
#include <malloc.h>
}
#endif

namespace eterm { namespace Terminal {

/* identification sequence returned in DA and DECID */
static const char* vtiden = "\033[?6c";

/*
 * word delimiter string
 *
 * More advanced example: L" `'\"()[]{}"
 */
const static unsigned int worddelimiters[] = { ' ', 0 };

/*
 * spaces per tab
 *
 * When you are changing this value, don't forget to adapt the »it« value in
 * the st.info and appropriately install the st.info in the environment where
 * you use this st version.
 *
 *	it#$tabspaces,
 *
 * Secondly make sure your kernel is not expanding tabs. When running `stty
 * -a` »tab0« should appear. You can tell the terminal to not expand tabs by
 *  running following command:
 *
 *	stty tabs
 */
static const unsigned int tabspaces = 4;

#ifdef _WIN32
#include <windows.h>
#endif

#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) < ( b ) ? ( b ) : ( a ) )
#define LEN( a ) ( sizeof( a ) / sizeof( a )[0] )
#define BETWEEN( x, a, b ) ( ( a ) <= ( x ) && ( x ) <= ( b ) )
#define DIVCEIL( n, d ) ( ( ( n ) + ( ( d ) - 1 ) ) / ( d ) )
#define DEFAULT( a, b ) ( a ) = ( a ) ? ( a ) : ( b )
#define LIMIT( x, a, b ) ( x ) = ( x )<( a ) ? ( a ) : ( x )>( b ) ? ( b ) : ( x )
#define ATTRCMP( a, b ) ( ( a ).mode != ( b ).mode || ( a ).fg != ( b ).fg || ( a ).bg != ( b ).bg )
#define TIMEDIFF( t1, t2 ) ( ( t1.tv_sec - t2.tv_sec ) * 1000 + ( t1.tv_nsec - t2.tv_nsec ) / 1E6 )
#define MODBIT( x, set, bit ) ( ( set ) ? ( ( x ) |= ( bit ) ) : ( ( x ) &= ~( bit ) ) )

#define TRUECOLOR( r, g, b ) ( 1 << 24 | ( r ) << 16 | ( g ) << 8 | ( b ) )
#define IS_TRUECOL( x ) ( 1 << 24 & ( x ) )

/* Arbitrary sizes */
#define UTF_INVALID 0xFFFD
#define UTF_SIZ 4

/* macros */

#define IS_SET( flag ) ( ( mTerm.mode & ( flag ) ) != 0 )
#define ISCONTROLC0( c ) ( BETWEEN( c, 0, 0x1f ) || ( c ) == 0x7f )
#define ISCONTROLC1( c ) ( BETWEEN( c, 0x80, 0x9f ) )
#define ISCONTROL( c ) ( ISCONTROLC0( c ) || ISCONTROLC1( c ) )
#define ISDELIM( u ) ( u && _wcschr( worddelimiters, u ) )
#define TLINE( y )                                                                                \
	( ( y ) < mTerm.scr && mTerm.histsize > 0                                                     \
		  ? mTerm.hist[( ( y ) + mTerm.histi - mTerm.scr + mTerm.histsize + 1 ) % mTerm.histsize] \
		  : mTerm.line[( y ) - mTerm.scr] )

typedef struct emoji_range {
	int32_t min_code;
	int32_t max_code;
	uint8_t bitmap[347];
} emoji_range_t;

static const emoji_range_t bmp_emoji_block = {
	0x231a,
	0x2b55,
	{ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x47, 0x02, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x18, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x03, 0x00,
	  0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x02, 0x80, 0x00, 0x03, 0x00, 0x18, 0x0c, 0x10, 0x04,
	  0x00, 0x00, 0x01, 0x0b, 0x09, 0x08, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x14,
	  0x2e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x40, 0x00, 0x20, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x08 } };

static const emoji_range_t smp_emoji_block = {
	0x1f004,
	0x1fad6,
	{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0xe4, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x40, 0x00, 0x00, 0xc8, 0x77, 0x00, 0x00, 0x30, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0x1f, 0x00, 0xfe, 0xfb, 0xff, 0xff,
	  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff,
	  0x7f, 0xf8, 0x00, 0xf0, 0xff, 0x1f, 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	  0xd7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff,
	  0xff, 0xff, 0x03, 0x80, 0xf7, 0xff, 0xff, 0x0f, 0x00, 0x40, 0x00, 0x00, 0x00, 0x06, 0x00,
	  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff,
	  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff,
	  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x03, 0x71, 0x0e, 0x00, 0x80, 0x01, 0xff, 0x01, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xfb, 0xff, 0xff, 0xff,
	  0xff, 0xff, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
	  0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0xf0, 0x71, 0xf0, 0x07, 0xf0, 0xff, 0xff, 0x1f, 0xf0, 0x07, 0x70, 0x00,
	  0xf0, 0x07 } };

static const emoji_range_t* emoji_ranges[] = { &bmp_emoji_block, &smp_emoji_block, 0 };

static inline bool is_emoji( int32_t code ) {
	const emoji_range_t** curr = emoji_ranges;
	while ( *curr != 0 ) {
		if ( code >= ( *curr )->min_code && code <= ( *curr )->max_code ) {
			int ofs = code - ( *curr )->min_code;
			int byteOffset = ofs / 8;
			int bit = 1 << ( ofs % 8 );
			return ( ( *curr )->bitmap[byteOffset] & bit ) == bit;
		}
		++curr;
	}
	return false;
}

static size_t _wcslen( const Rune* s ) {
	const Rune* a;
	for ( a = s; *s; s++ )
		;
	return s - a;
}

static Rune* _wcschr( const Rune* s, const Rune& c ) {
	if ( !c )
		return (Rune*)s + _wcslen( s );
	for ( ; *s && *s != c; s++ )
		;
	return *s ? (Rune*)s : 0;
}

static const uchar utfbyte[UTF_SIZ + 1] = { 0x80, 0, 0xC0, 0xE0, 0xF0 };
static const uchar utfmask[UTF_SIZ + 1] = { 0xC0, 0x80, 0xE0, 0xF0, 0xF8 };
static const Rune utfmin[UTF_SIZ + 1] = { 0, 0, 0x80, 0x800, 0x10000 };
static const Rune utfmax[UTF_SIZ + 1] = { 0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF };

static int isboxdraw( const Rune& u ) {
	Rune block = u & ~0xff;
	return ( block == 0x2500 && boxdata[u & 0xFF] ) || ( block == 0x2800 );
}

/* the "index" is actually the entire shape data encoded as ushort */
ushort TerminalEmulator::boxdrawindex( const TerminalGlyph* g ) {
	if ( ( g->u & ~0xff ) == 0x2800 )
		return BRL | ( g->u & 0xFF );
	if ( ( g->mode & ATTR_BOLD ) )
		return BDB | boxdata[g->u & 0xFF];
	return boxdata[g->u & 0xFF];
}

#define xfree( p ) eeFree( p )
#define xmalloc( p ) eeMalloc( p )
#define xrealloc( p, len ) eeRealloc( p, len )

static size_t utf8decode( const char*, Rune*, size_t );
static Rune utf8decodebyte( char, size_t* );
static char utf8encodebyte( Rune, size_t );
static size_t utf8validate( Rune*, size_t );
static size_t utf8encode( Rune, char* );

static char* base64dec( const char* );
static char base64dec_getc( const char** );

// static intmax_t xwrite( int, const char*, size_t );

size_t utf8decode( const char* c, Rune* u, size_t clen ) {
	size_t i, j, len, type;
	Rune udecoded;

	*u = UTF_INVALID;
	if ( !clen )
		return 0;
	udecoded = utf8decodebyte( c[0], &len );
	if ( !BETWEEN( len, 1, UTF_SIZ ) )
		return 1;
	for ( i = 1, j = 1; i < clen && j < len; ++i, ++j ) {
		udecoded = ( udecoded << 6 ) | utf8decodebyte( c[i], &type );
		if ( type != 0 )
			return j;
	}
	if ( j < len )
		return 0;
	*u = udecoded;
	utf8validate( u, len );

	return len;
}

Rune utf8decodebyte( char c, size_t* i ) {
	for ( *i = 0; *i < LEN( utfmask ); ++( *i ) )
		if ( ( (uchar)c & utfmask[*i] ) == utfbyte[*i] )
			return (uchar)c & ~utfmask[*i];

	return 0;
}

size_t utf8encode( Rune u, char* c ) {
	size_t len, i;

	len = utf8validate( &u, 0 );
	if ( len > UTF_SIZ )
		return 0;

	for ( i = len - 1; i != 0; --i ) {
		c[i] = utf8encodebyte( u, 0 );
		u >>= 6;
	}
	c[0] = utf8encodebyte( u, len );

	return len;
}

char utf8encodebyte( Rune u, size_t i ) {
	return utfbyte[i] | ( u & ~utfmask[i] );
}

size_t utf8validate( Rune* u, size_t i ) {
	if ( !BETWEEN( *u, utfmin[i], utfmax[i] ) || BETWEEN( *u, 0xD800, 0xDFFF ) )
		*u = UTF_INVALID;
	for ( i = 1; *u > utfmax[i]; ++i )
		;

	return i;
}

static const char base64_digits[] = {
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,		  0,  0,  0,  0,
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,		  0,  0,  0,  62,
	0,	0,	0,	63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,	0,	0,	(char)-1, 0,  0,  0,  0,
	1,	2,	3,	4,	5,	6,	7,	8,	9,	10, 11, 12, 13, 14, 15, 16, 17, 18,		  19, 20, 21, 22,
	23, 24, 25, 0,	0,	0,	0,	0,	0,	26, 27, 28, 29, 30, 31, 32, 33, 34,		  35, 36, 37, 38,
	39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0,	0,	0,	0,	0,		  0,  0,  0,  0,
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,		  0,  0,  0,  0,
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,		  0,  0,  0,  0,
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,		  0,  0,  0,  0,
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,		  0,  0,  0,  0,
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,		  0,  0,  0,  0,
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0 };

char base64dec_getc( const char** src ) {
	while ( **src && !isprint( **src ) )
		( *src )++;
	return **src ? *( ( *src )++ ) : '='; /* emulate padding if string ends */
}

char* base64dec( const char* src ) {
	size_t in_len = strlen( src );
	char *result, *dst;

	if ( in_len % 4 )
		in_len += 4 - ( in_len % 4 );
	result = dst = (char*)xmalloc( in_len / 4 * 3 + 1 );
	while ( *src ) {
		int a = base64_digits[(unsigned char)base64dec_getc( &src )];
		int b = base64_digits[(unsigned char)base64dec_getc( &src )];
		int c = base64_digits[(unsigned char)base64dec_getc( &src )];
		int d = base64_digits[(unsigned char)base64dec_getc( &src )];

		/* invalid input. 'a' can be -1, e.g. if src is "\n" (c-str) */
		if ( a == -1 || b == -1 )
			break;

		*dst++ = ( a << 2 ) | ( ( b & 0x30 ) >> 4 );
		if ( c == -1 )
			break;
		*dst++ = ( ( b & 0x0f ) << 4 ) | ( ( c & 0x3c ) >> 2 );
		if ( d == -1 )
			break;
		*dst++ = ( ( c & 0x03 ) << 6 ) | d;
	}
	*dst = '\0';
	return result;
}

void TerminalEmulator::selinit( void ) {
	mSel.mode = SEL_IDLE;
	mSel.snap = 0;
	mSel.ob.x = -1;
}

int TerminalEmulator::tlinelen( int y ) const {
	int i = mTerm.col;

	if ( TLINE( y )[i - 1].mode & ATTR_WRAP )
		return i;

	while ( i > 0 && TLINE( y )[i - 1].u == ' ' )
		--i;

	return i;
}

int TerminalEmulator::tiswrapped( int y ) {
	int len = tlinelen( y );

	return len > 0 && ( TLINE( y )[len - 1].mode & ATTR_WRAP );
}

TerminalSelectionMode TerminalEmulator::getSelectionMode() const {
	return (TerminalSelectionMode)mSel.mode;
}

void TerminalEmulator::selstart( int col, int row, int snap ) {
	selclear();
	mSel.mode = SEL_EMPTY;
	mSel.type = SEL_REGULAR;
	mSel.alt = IS_SET( MODE_ALTSCREEN );
	mSel.snap = snap;
	mSel.oe.x = mSel.ob.x = col;
	mSel.oe.y = mSel.ob.y = row;
	selnormalize();

	if ( mSel.snap != 0 )
		mSel.mode = SEL_READY;
	tsetdirt( mSel.nb.y, mSel.ne.y );
}

void TerminalEmulator::selextend( int col, int row, int type, int done ) {
	int oldey, oldex, oldsby, oldsey, oldtype;

	if ( mSel.mode == SEL_IDLE )
		return;
	if ( done && mSel.mode == SEL_EMPTY ) {
		selclear();
		return;
	}

	oldey = mSel.oe.y;
	oldex = mSel.oe.x;
	oldsby = mSel.nb.y;
	oldsey = mSel.ne.y;
	oldtype = mSel.type;

	mSel.oe.x = col;
	mSel.oe.y = row;
	selnormalize();
	mSel.type = type;

	if ( oldey != mSel.oe.y || oldex != mSel.oe.x || oldtype != mSel.type ||
		 mSel.mode == SEL_EMPTY )
		tsetdirt( MIN( mSel.nb.y, oldsby ), MAX( mSel.ne.y, oldsey ) );

	mSel.mode = done ? SEL_IDLE : SEL_READY;
}

void TerminalEmulator::selnormalize( void ) {
	int i;

	if ( mSel.type == SEL_REGULAR && mSel.ob.y != mSel.oe.y ) {
		mSel.nb.x = mSel.ob.y < mSel.oe.y ? mSel.ob.x : mSel.oe.x;
		mSel.ne.x = mSel.ob.y < mSel.oe.y ? mSel.oe.x : mSel.ob.x;
	} else {
		mSel.nb.x = MIN( mSel.ob.x, mSel.oe.x );
		mSel.ne.x = MAX( mSel.ob.x, mSel.oe.x );
	}
	mSel.nb.y = MIN( mSel.ob.y, mSel.oe.y );
	mSel.ne.y = MAX( mSel.ob.y, mSel.oe.y );

	selsnap( &mSel.nb.x, &mSel.nb.y, -1 );
	selsnap( &mSel.ne.x, &mSel.ne.y, +1 );

	/* selection is over terminal size? */
	mSel.nb.y = MIN( mTerm.bot, mSel.nb.y );
	mSel.ne.y = MIN( mTerm.bot, mSel.ne.y );

	/* expand selection over line breaks */
	if ( mSel.type == SEL_RECTANGULAR )
		return;
	i = tlinelen( mSel.nb.y );
	if ( i < mSel.nb.x )
		mSel.nb.x = i;
	if ( tlinelen( mSel.ne.y ) <= mSel.ne.x )
		mSel.ne.x = mTerm.col - 1;
}

int TerminalEmulator::selected( int x, int y ) {
	if ( mSel.mode == SEL_EMPTY || mSel.ob.x == -1 || mSel.alt != IS_SET( MODE_ALTSCREEN ) )
		return 0;

	if ( mSel.type == SEL_RECTANGULAR )
		return BETWEEN( y, mSel.nb.y, mSel.ne.y ) && BETWEEN( x, mSel.nb.x, mSel.ne.x );

	return BETWEEN( y, mSel.nb.y, mSel.ne.y ) && ( y != mSel.nb.y || x >= mSel.nb.x ) &&
		   ( y != mSel.ne.y || x <= mSel.ne.x );
}

void TerminalEmulator::selsnap( int* x, int* y, int direction ) {
	int newx, newy, xt, yt;
	int delim, prevdelim;
	TerminalGlyph *gp, *prevgp;

	switch ( mSel.snap ) {
		case SNAP_WORD:
			/*
			 * Snap around if the word wraps around at the end or
			 * beginning of a line.
			 */
			prevgp = &TLINE( *y )[*x];
			prevdelim = ISDELIM( prevgp->u );
			for ( ;; ) {
				newx = *x + direction;
				newy = *y;
				if ( !BETWEEN( newx, 0, mTerm.col - 1 ) ) {
					newy += direction;
					newx = ( newx + mTerm.col ) % mTerm.col;
					if ( !BETWEEN( newy, 0, mTerm.row - 1 ) )
						break;

					if ( direction > 0 )
						yt = *y, xt = *x;
					else
						yt = newy, xt = newx;
					if ( !( TLINE( yt )[xt].mode & ATTR_WRAP ) )
						break;
				}

				if ( newx >= tlinelen( newy ) )
					break;

				gp = &TLINE( newy )[newx];
				delim = ISDELIM( gp->u );
				if ( !( gp->mode & ATTR_WDUMMY ) &&
					 ( delim != prevdelim || ( delim && gp->u != prevgp->u ) ) )
					break;

				*x = newx;
				*y = newy;
				prevgp = gp;
				prevdelim = delim;
			}
			break;
		case SNAP_LINE:
			/*
			 * Snap around if the the previous line or the current one
			 * has set ATTR_WRAP at its end. Then the whole next or
			 * previous line will be selected.
			 */
			*x = ( direction < 0 ) ? 0 : mTerm.col - 1;
			if ( direction < 0 ) {
				for ( ; *y > 0; *y += direction ) {
					if ( !tiswrapped( *y - 1 ) )
						break;
				}
			} else if ( direction > 0 ) {
				for ( ; *y < mTerm.row - 1; *y += direction ) {
					if ( !tiswrapped( *y ) )
						break;
				}
			}
			break;
	}
}

char* TerminalEmulator::getsel( void ) const {
	char *str, *ptr;
	int y, bufsize, lastx, linelen;
	TerminalGlyph *gp, *last;

	if ( mSel.ob.x == -1 )
		return NULL;

	bufsize = ( mTerm.col + 1 ) * ( mSel.ne.y - mSel.nb.y + 1 ) * UTF_SIZ;
	ptr = str = (char*)xmalloc( bufsize );

	/* append every set & selected glyph to the selection */
	for ( y = mSel.nb.y; y <= mSel.ne.y && y < mTerm.row; y++ ) {
		if ( ( linelen = tlinelen( y ) ) == 0 ) {
			*ptr++ = '\n';
			continue;
		}

		if ( mSel.type == SEL_RECTANGULAR ) {
			gp = &TLINE( y )[mSel.nb.x];
			lastx = mSel.ne.x;
		} else {
			gp = &TLINE( y )[mSel.nb.y == y ? mSel.nb.x : 0];
			lastx = ( mSel.ne.y == y ) ? mSel.ne.x : mTerm.col - 1;
		}
		last = &TLINE( y )[MIN( lastx, linelen - 1 )];
		while ( last >= gp && last->u == ' ' )
			--last;

		for ( ; gp <= last; ++gp ) {
			if ( gp->mode & ATTR_WDUMMY )
				continue;

			ptr += utf8encode( gp->u, ptr );
		}

		/*
		 * Copy and pasting of line endings is inconsistent
		 * in the inconsistent terminal and GUI world.
		 * The best solution seems like to produce '\n' when
		 * something is copied from st and convert '\n' to
		 * '\r', when something to be pasted is received by
		 * st.
		 * FIXME: Fix the computer world.
		 */
		if ( ( y < mSel.ne.y || lastx >= linelen ) &&
			 ( !( last->mode & ATTR_WRAP ) || mSel.type == SEL_RECTANGULAR ) )
			*ptr++ = '\n';
	}
	*ptr = 0;
	return str;
}

bool TerminalEmulator::hasSelection() const {
	return mSel.mode == SEL_READY;
}

std::string TerminalEmulator::getSelection() const {
	char* sel = getsel();
	if ( sel ) {
		std::string selection( sel );
		xfree( sel );
		return selection;
	}
	return "";
}

void TerminalEmulator::selclear( void ) {
	if ( mSel.ob.x == -1 )
		return;
	mSel.mode = SEL_IDLE;
	mSel.ob.x = -1;
	tsetdirt( mSel.nb.y, mSel.ne.y );
}

void TerminalEmulator::_die( const char* errstr, ... ) {
	char buf[256];

	memset( buf, 0, 256 );
	va_list ap;
	va_start( ap, errstr );
	vsnprintf( buf, 256, errstr, ap );
	va_end( ap );
	logError( buf );
	terminate();
}

size_t TerminalEmulator::ttyread( void ) {
	int ret, written;

	/* append read bytes to unprocessed bytes */
	ret = mPty->read( mBuf + mBuflen, LEN( mBuf ) - mBuflen );

	switch ( ret ) {
		case 0:
			return 0;
		case -1:
			_die( "couldn't read from shell: %s\n", strerror( errno ) );
			return 0;
		default:
			TerminalArg arg = { (int)mTerm.scr };
			kscrolldown( &arg );

			mBuflen += ret;
			written = twrite( mBuf, mBuflen, 0 );
			mBuflen -= written;
			/* keep any incomplete UTF-8 byte sequence for the next call */
			if ( mBuflen > 0 )
				memmove( mBuf, mBuf + written, mBuflen );
			return ret;
	}

	return 0;
}

void TerminalEmulator::kscrolldown( const TerminalArg* a ) {
	int n = a->i;

	if ( n == INT_MAX )
		n = mTerm.scr;

	if ( n < 0 )
		n = mTerm.row + n;

	if ( n > mTerm.scr )
		n = mTerm.scr;

	if ( mTerm.scr > 0 ) {
		mTerm.scr -= n;
		selmove( -n );
		tfulldirt();
		onScrollPositionChange();
	}
}

void TerminalEmulator::kscrollup( const TerminalArg* a ) {
	int n = a->i;

	if ( n == INT_MAX )
		n = mTerm.histi - mTerm.scr;

	if ( n < 0 )
		n = mTerm.row + n;

	if ( mTerm.scr + n > mTerm.histi )
		n = mTerm.histi - mTerm.scr;

	if ( n == 0 )
		return;

	if ( mTerm.scr <= mTerm.histsize - n && mTerm.scr + n <= mTerm.histi ) {
		mTerm.scr += n;
		selmove( n );
		tfulldirt();
		onScrollPositionChange();
	}
}

void TerminalEmulator::kscrollto( const TerminalArg* a ) {
	int n = a->i;

	if ( 0 <= n && n <= mTerm.histi ) {
		mTerm.scr = n;
		selscroll( 0, n );
		tfulldirt();
		onScrollPositionChange();
	}
}

int TerminalEmulator::tisaltscr() {
	return IS_SET( MODE_ALTSCREEN );
}

int TerminalEmulator::scrollSize() const {
	return mTerm.histi;
}

int TerminalEmulator::rowCount() const {
	return mTerm.row;
}

void TerminalEmulator::trimMemory() {
#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( __GLIBC__ )
	if ( mAllowMemoryTrimnming ) {
		malloc_trim( 0 );
	}
#endif
}

void TerminalEmulator::clearHistory() {
	for ( int i = 0; i < mTerm.histcursize; ++i )
		eeSAFE_FREE( mTerm.hist[i] );
	eeSAFE_FREE( mTerm.hist );
	mTerm.histcursize = 0;
	mTerm.histi = 0;
	trimMemory();
}

int TerminalEmulator::scrollPos() {
	return mTerm.scr;
}

bool TerminalEmulator::getAllowMemoryTrimnming() const {
	return mAllowMemoryTrimnming;
}

void TerminalEmulator::setAllowMemoryTrimnming( bool allowMemoryTrimnming ) {
	mAllowMemoryTrimnming = allowMemoryTrimnming;
}

Vector2i TerminalEmulator::getSize() const {
	return { mTerm.col, mTerm.row };
}

IProcess* TerminalEmulator::getProcess() const {
	return mProcess ? mProcess.get() : nullptr;
}

bool TerminalEmulator::isScrolling() const {
	return mTerm.scr != 0;
}

void TerminalEmulator::ttywrite( const char* s, size_t n, int may_echo ) {
	const char* next;

	TerminalArg arg = { (int)mTerm.scr };
	kscrolldown( &arg );

	if ( may_echo && IS_SET( MODE_ECHO ) )
		twrite( s, (int)n, 1 );

	if ( !IS_SET( MODE_CRLF ) ) {
		ttywriteraw( s, n );
		return;
	}

	/* This is similar to how the kernel handles ONLCR for ttys */
	while ( n > 0 ) {
		if ( *s == '\r' ) {
			next = s + 1;
			ttywriteraw( "\r\n", 2 );
		} else {
			next = (const char*)memchr( s, '\r', n );
			DEFAULT( next, s + n );
			ttywriteraw( s, next - s );
		}
		n -= next - s;
		s = next;
	}
}

void TerminalEmulator::ttywriteraw( const char* s, size_t n ) {
	if ( mPty->write( s, n ) < (int)n ) {
		_die( "Failed to write to TTY" );
	}
}

void TerminalEmulator::ttyhangup() {
	if ( mProcess )
		mProcess->terminate();
}

int TerminalEmulator::tattrset( int attr ) {
	int i, j;

	for ( i = 0; i < mTerm.row - 1; i++ ) {
		for ( j = 0; j < mTerm.col - 1; j++ ) {
			if ( mTerm.line[i][j].mode & attr )
				return 1;
		}
	}

	return 0;
}

void TerminalEmulator::tsetdirt( int top, int bot ) {
	int i;

	LIMIT( top, 0, mTerm.row - 1 );
	LIMIT( bot, 0, mTerm.row - 1 );
	mDirty = true;
	for ( i = top; i <= bot; i++ )
		mTerm.dirty[i] = 1;
}

void TerminalEmulator::tsetdirtattr( int attr ) {
	int i, j;

	for ( i = 0; i < mTerm.row - 1; i++ ) {
		for ( j = 0; j < mTerm.col - 1; j++ ) {
			if ( mTerm.line[i][j].mode & attr ) {
				tsetdirt( i, i );
				break;
			}
		}
	}
}

void TerminalEmulator::tfulldirt( void ) {
	tsetdirt( 0, mTerm.row - 1 );
}

void TerminalEmulator::tcursor( int mode ) {
	static TerminalCursor c[2];
	int alt = IS_SET( MODE_ALTSCREEN );

	if ( mode == CURSOR_SAVE ) {
		c[alt] = mTerm.c;
	} else if ( mode == CURSOR_LOAD ) {
		mTerm.c = c[alt];
		tmoveto( c[alt].x, c[alt].y );
	}
}

void TerminalEmulator::treset( void ) {
	uint i;

	mTerm.c = TerminalCursor{};
	mTerm.c.attr = TerminalGlyph{};
	mTerm.c.attr.mode = ATTR_NULL;
	mTerm.c.attr.fg = mDefaultFg;
	mTerm.c.attr.bg = mDefaultBg;
	mTerm.c.x = 0;
	mTerm.c.y = 0;
	mTerm.c.state = CURSOR_DEFAULT;

	memset( mTerm.tabs, 0, mTerm.col * sizeof( *mTerm.tabs ) );
	for ( i = tabspaces; i < (uint)mTerm.col; i += tabspaces )
		mTerm.tabs[i] = 1;
	mTerm.top = 0;
	mTerm.bot = mTerm.row - 1;
	mTerm.mode = MODE_WRAP | MODE_UTF8;
	memset( mTerm.trantbl, CS_USA, sizeof( mTerm.trantbl ) );
	mTerm.charset = 0;

	for ( i = 0; i < 2; i++ ) {
		tmoveto( 0, 0 );
		tcursor( CURSOR_SAVE );
		tclearregion( 0, 0, mTerm.col - 1, mTerm.row - 1 );
		tswapscreen();
	}
}

void TerminalEmulator::tnew( int col, int row, size_t historySize ) {
	mTerm = Term{};
	mTerm.c = TerminalCursor{};
	mTerm.c.attr = TerminalGlyph{};
	mTerm.c.attr.fg = mDefaultFg;
	mTerm.c.attr.bg = mDefaultBg;
	mTerm.histsize = historySize;

	tresize( col, row );
	treset();
}

void TerminalEmulator::tswapscreen( void ) {
	Line* tmp = mTerm.line;

	mTerm.line = mTerm.alt;
	mTerm.alt = tmp;
	mTerm.mode ^= MODE_ALTSCREEN;
	tfulldirt();
}

void TerminalEmulator::resizeHistory() {
	size_t oriSize = mTerm.histcursize;
	if ( mTerm.histi >= (int)mTerm.histcursize ) {
		int newSize = eemin( mTerm.histi + mTerm.row, mTerm.histsize );
		mTerm.hist = (Line*)xrealloc( mTerm.hist, newSize * sizeof( Line ) );
		mTerm.histcursize = newSize;

		for ( int i = oriSize; i < mTerm.histcursize; i++ ) {
			mTerm.hist[i] = (TerminalGlyph*)xmalloc( mTerm.col * sizeof( TerminalGlyph ) );
			for ( int j = 0; j < mTerm.col; j++ ) {
				mTerm.hist[i][j] = mTerm.c.attr;
				mTerm.hist[i][j].u = ' ';
			}
		}
	}
}

void TerminalEmulator::tscrolldown( int top, int n, int copyhist ) {
	int i;
	Line temp;

	LIMIT( n, 0, mTerm.bot - top + 1 );
	if ( copyhist && mTerm.histsize > 0 ) {
		mTerm.histi = ( mTerm.histi - 1 + mTerm.histsize ) % mTerm.histsize;
		resizeHistory();
		temp = mTerm.hist[mTerm.histi];
		mTerm.hist[mTerm.histi] = mTerm.line[mTerm.bot];
		mTerm.line[mTerm.bot] = temp;
	}

	tsetdirt( top, mTerm.bot - n );
	tclearregion( 0, mTerm.bot - n + 1, mTerm.col - 1, mTerm.bot );

	for ( i = mTerm.bot; i >= top + n; i-- ) {
		temp = mTerm.line[i];
		mTerm.line[i] = mTerm.line[i - n];
		mTerm.line[i - n] = temp;
	}

	if ( mTerm.scr == 0 )
		selscroll( top, n );
}

void TerminalEmulator::tscrollup( int top, int n, int copyhist ) {
	int i;
	Line temp;

	LIMIT( n, 0, mTerm.bot - top + 1 );

	if ( copyhist && mTerm.histsize > 0 ) {
		mTerm.histi = ( mTerm.histi + 1 ) % mTerm.histsize;
		resizeHistory();
		temp = mTerm.hist[mTerm.histi];
		mTerm.hist[mTerm.histi] = mTerm.line[top];
		mTerm.line[top] = temp;
	}

	if ( mTerm.scr > 0 && mTerm.scr < mTerm.histsize )
		mTerm.scr = MIN( mTerm.scr + n, mTerm.histsize - 1 );

	tclearregion( 0, top, mTerm.col - 1, top + n - 1 );
	tsetdirt( top + n, mTerm.bot );

	for ( i = top; i <= mTerm.bot - n; i++ ) {
		temp = mTerm.line[i];
		mTerm.line[i] = mTerm.line[i + n];
		mTerm.line[i + n] = temp;
	}

	if ( mTerm.scr == 0 )
		selscroll( top, -n );

	onScrollPositionChange();
}

void TerminalEmulator::selmove( int n ) {
	mSel.ob.y += n, mSel.nb.y += n;
	mSel.oe.y += n, mSel.ne.y += n;
}

void TerminalEmulator::selscroll( int top, int n ) {
	if ( mSel.ob.x == -1 || mSel.alt != IS_SET( MODE_ALTSCREEN ) )
		return;

	if ( BETWEEN( mSel.nb.y, top, mTerm.bot ) != BETWEEN( mSel.ne.y, top, mTerm.bot ) ) {
		selclear();
	} else if ( BETWEEN( mSel.nb.y, top, mTerm.bot ) ) {
		mSel.ob.y += n;
		mSel.oe.y += n;
		if ( mSel.ob.y < mTerm.top || mSel.ob.y > mTerm.bot || mSel.oe.y < mTerm.top ||
			 mSel.oe.y > mTerm.bot ) {
			selclear();
		} else {
			selnormalize();
		}
	}
}

void TerminalEmulator::tnewline( int first_col ) {
	int y = mTerm.c.y;

	if ( y == mTerm.bot ) {
		tscrollup( mTerm.top, 1, 1 );
	} else {
		y++;
	}
	tmoveto( first_col ? 0 : mTerm.c.x, y );
}

void TerminalEmulator::csiparse( void ) {
	char *p = mCsiescseq.buf, *np;
	long int v;

	mCsiescseq.narg = 0;
	if ( *p == '?' ) {
		mCsiescseq.priv = 1;
		p++;
	}

	mCsiescseq.buf[mCsiescseq.len] = '\0';
	while ( p < mCsiescseq.buf + mCsiescseq.len ) {
		np = NULL;
		v = strtol( p, &np, 10 );
		if ( np == p )
			v = 0;
		if ( v == LONG_MAX || v == LONG_MIN )
			v = -1;
		mCsiescseq.arg[mCsiescseq.narg++] = v;
		p = np;
		if ( *p != ';' || mCsiescseq.narg == ESC_ARG_SIZ )
			break;
		p++;
	}
	mCsiescseq.mode[0] = *p++;
	mCsiescseq.mode[1] = ( p < mCsiescseq.buf + mCsiescseq.len ) ? *p : '\0';
}

/* for absolute user moves, when decom is set */
void TerminalEmulator::tmoveato( int x, int y ) {
	tmoveto( x, y + ( ( mTerm.c.state & CURSOR_ORIGIN ) ? mTerm.top : 0 ) );
}

void TerminalEmulator::tmoveto( int x, int y ) {
	int miny, maxy;

	if ( mTerm.c.state & CURSOR_ORIGIN ) {
		miny = mTerm.top;
		maxy = mTerm.bot;
	} else {
		miny = 0;
		maxy = mTerm.row - 1;
	}
	mTerm.c.state &= ~CURSOR_WRAPNEXT;
	mTerm.c.x = LIMIT( x, 0, mTerm.col - 1 );
	mTerm.c.y = LIMIT( y, miny, maxy );
}

void TerminalEmulator::tsetchar( Rune u, TerminalGlyph* attr, int x, int y ) {
	static const char* vt100_0[62] = {
		/* 0x41 - 0x7e */
		"↑", "↓", "→", "←", "█", "▚", "☃",		/* A - G */
		0,	 0,	  0,   0,	0,	 0,	  0,   0,	/* H - O */
		0,	 0,	  0,   0,	0,	 0,	  0,   0,	/* P - W */
		0,	 0,	  0,   0,	0,	 0,	  0,   " ", /* X - _ */
		"◆", "▒", "␉", "␌", "␍", "␊", "°", "±", /* ` - g */
		"␤", "␋", "┘", "┐", "┌", "└", "┼", "⎺", /* h - o */
		"⎻", "─", "⎼", "⎽", "├", "┤", "┴", "┬", /* p - w */
		"│", "≤", "≥", "π", "≠", "£", "·",		/* x - ~ */
	};

	/*
	 * The table is proudly stolen from rxvt.
	 */
	if ( mTerm.trantbl[mTerm.charset] == CS_GRAPHIC0 && BETWEEN( u, 0x41, 0x7e ) &&
		 vt100_0[u - 0x41] )
		utf8decode( vt100_0[u - 0x41], &u, UTF_SIZ );

	if ( TLINE( y )[x].mode & ATTR_WIDE ) {
		if ( x + 1 < mTerm.col ) {
			TLINE( y )[x + 1].u = ' ';
			TLINE( y )[x + 1].mode &= ~ATTR_WDUMMY;
		}
	} else if ( TLINE( y )[x].mode & ATTR_WDUMMY ) {
		TLINE( y )[x - 1].u = ' ';
		TLINE( y )[x - 1].mode &= ~ATTR_WIDE;
	}

	mTerm.dirty[y] = 1;
	TLINE( y )[x] = *attr;
	TLINE( y )[x].u = u;
	mDirty = true;

	if ( isboxdraw( u ) )
		TLINE( y )[x].mode |= ATTR_BOXDRAW;
}

void TerminalEmulator::tclearregion( int x1, int y1, int x2, int y2 ) {
	int x, y, temp;
	TerminalGlyph* gp;

	if ( x1 > x2 )
		temp = x1, x1 = x2, x2 = temp;
	if ( y1 > y2 )
		temp = y1, y1 = y2, y2 = temp;

	LIMIT( x1, 0, mTerm.col - 1 );
	LIMIT( x2, 0, mTerm.col - 1 );
	LIMIT( y1, 0, mTerm.row - 1 );
	LIMIT( y2, 0, mTerm.row - 1 );

	for ( y = y1; y <= y2; y++ ) {
		mTerm.dirty[y] = 1;
		mDirty = true;
		for ( x = x1; x <= x2; x++ ) {
			gp = &TLINE( y )[x];
			if ( selected( x, y ) )
				selclear();
			gp->fg = mTerm.c.attr.fg;
			gp->bg = mTerm.c.attr.bg;
			gp->mode = 0;
			gp->u = ' ';
		}
	}
}

void TerminalEmulator::tdeletechar( int n ) {
	int dst, src, size;
	TerminalGlyph* line;

	LIMIT( n, 0, mTerm.col - mTerm.c.x );

	dst = mTerm.c.x;
	src = mTerm.c.x + n;
	size = mTerm.col - src;
	line = mTerm.line[mTerm.c.y];

	memmove( &line[dst], &line[src], size * sizeof( TerminalGlyph ) );
	tclearregion( mTerm.col - n, mTerm.c.y, mTerm.col - 1, mTerm.c.y );
}

void TerminalEmulator::tinsertblank( int n ) {
	int dst, src, size;
	TerminalGlyph* line;

	LIMIT( n, 0, mTerm.col - mTerm.c.x );

	dst = mTerm.c.x + n;
	src = mTerm.c.x;
	size = mTerm.col - dst;
	line = mTerm.line[mTerm.c.y];

	memmove( &line[dst], &line[src], size * sizeof( TerminalGlyph ) );
	tclearregion( src, mTerm.c.y, dst - 1, mTerm.c.y );
}

void TerminalEmulator::tinsertblankline( int n ) {
	if ( BETWEEN( mTerm.c.y, mTerm.top, mTerm.bot ) )
		tscrolldown( mTerm.c.y, n, 0 );
}

void TerminalEmulator::tdeleteline( int n ) {
	if ( BETWEEN( mTerm.c.y, mTerm.top, mTerm.bot ) )
		tscrollup( mTerm.c.y, n, 0 );
}

int32_t TerminalEmulator::tdefcolor( int* attr, int* npar, int l ) {
	int32_t idx = -1;
	uint r, g, b;

	switch ( attr[*npar + 1] ) {
		case 2: /* direct color in RGB space */
			if ( *npar + 4 >= l ) {
				fprintf( stderr, "erresc(38): Incorrect number of parameters (%d)\n", *npar );
				break;
			}
			r = attr[*npar + 2];
			g = attr[*npar + 3];
			b = attr[*npar + 4];
			*npar += 4;
			if ( !BETWEEN( r, 0, 255 ) || !BETWEEN( g, 0, 255 ) || !BETWEEN( b, 0, 255 ) )
				fprintf( stderr, "erresc: bad rgb color (%u,%u,%u)\n", r, g, b );
			else
				idx = TRUECOLOR( r, g, b );
			break;
		case 5: /* indexed color */
			if ( *npar + 2 >= l ) {
				fprintf( stderr, "erresc(38): Incorrect number of parameters (%d)\n", *npar );
				break;
			}
			*npar += 2;
			if ( !BETWEEN( attr[*npar], 0, 255 ) )
				fprintf( stderr, "erresc: bad fgcolor %d\n", attr[*npar] );
			else
				idx = attr[*npar];
			break;
		case 0: /* implemented defined (only foreground) */
		case 1: /* transparent */
		case 3: /* direct color in CMY space */
		case 4: /* direct color in CMYK space */
		default:
			fprintf( stderr, "erresc(38): gfx attr %d unknown\n", attr[*npar] );
			break;
	}

	return idx;
}

void TerminalEmulator::tsetattr( int* attr, int l ) {
	int i;
	int32_t idx;

	for ( i = 0; i < l; i++ ) {
		switch ( attr[i] ) {
			case 0:
				mTerm.c.attr.mode &= ~( ATTR_BOLD | ATTR_FAINT | ATTR_ITALIC | ATTR_UNDERLINE |
										ATTR_BLINK | ATTR_REVERSE | ATTR_INVISIBLE | ATTR_STRUCK );
				mTerm.c.attr.fg = mDefaultFg;
				mTerm.c.attr.bg = mDefaultBg;
				break;
			case 1:
				mTerm.c.attr.mode |= ATTR_BOLD;
				break;
			case 2:
				mTerm.c.attr.mode |= ATTR_FAINT;
				break;
			case 3:
				mTerm.c.attr.mode |= ATTR_ITALIC;
				break;
			case 4:
				mTerm.c.attr.mode |= ATTR_UNDERLINE;
				break;
			case 5: /* slow blink */
					/* FALLTHROUGH */
			case 6: /* rapid blink */
				mTerm.c.attr.mode |= ATTR_BLINK;
				break;
			case 7:
				mTerm.c.attr.mode |= ATTR_REVERSE;
				break;
			case 8:
				mTerm.c.attr.mode |= ATTR_INVISIBLE;
				break;
			case 9:
				mTerm.c.attr.mode |= ATTR_STRUCK;
				break;
			case 22:
				mTerm.c.attr.mode &= ~( ATTR_BOLD | ATTR_FAINT );
				break;
			case 23:
				mTerm.c.attr.mode &= ~ATTR_ITALIC;
				break;
			case 24:
				mTerm.c.attr.mode &= ~ATTR_UNDERLINE;
				break;
			case 25:
				mTerm.c.attr.mode &= ~ATTR_BLINK;
				break;
			case 27:
				mTerm.c.attr.mode &= ~ATTR_REVERSE;
				break;
			case 28:
				mTerm.c.attr.mode &= ~ATTR_INVISIBLE;
				break;
			case 29:
				mTerm.c.attr.mode &= ~ATTR_STRUCK;
				break;
			case 38:
				if ( ( idx = tdefcolor( attr, &i, l ) ) >= 0 )
					mTerm.c.attr.fg = idx;
				break;
			case 39:
				mTerm.c.attr.fg = mDefaultFg;
				break;
			case 48:
				if ( ( idx = tdefcolor( attr, &i, l ) ) >= 0 )
					mTerm.c.attr.bg = idx;
				break;
			case 49:
				mTerm.c.attr.bg = mDefaultBg;
				break;
			default:
				if ( BETWEEN( attr[i], 30, 37 ) ) {
					mTerm.c.attr.fg = attr[i] - 30;
				} else if ( BETWEEN( attr[i], 40, 47 ) ) {
					mTerm.c.attr.bg = attr[i] - 40;
				} else if ( BETWEEN( attr[i], 90, 97 ) ) {
					mTerm.c.attr.fg = attr[i] - 90 + 8;
				} else if ( BETWEEN( attr[i], 100, 107 ) ) {
					mTerm.c.attr.bg = attr[i] - 100 + 8;
				} else {
					fprintf( stderr, "erresc(default): gfx attr %d unknown\n", attr[i] );
					csidump();
				}
				break;
		}
	}
}

void TerminalEmulator::tsetscroll( int t, int b ) {
	int temp;

	LIMIT( t, 0, mTerm.row - 1 );
	LIMIT( b, 0, mTerm.row - 1 );
	if ( t > b ) {
		temp = t;
		t = b;
		b = temp;
	}
	mTerm.top = t;
	mTerm.bot = b;
}

void TerminalEmulator::tsetmode( int priv, int set, int* args, int narg ) {
	int alt, *lim;

	for ( lim = args + narg; args < lim; ++args ) {
		if ( priv ) {
			switch ( *args ) {
				case 1: /* DECCKM -- Cursor key */
					xsetmode( set, MODE_APPCURSOR );
					break;
				case 5: /* DECSCNM -- Reverse video */
					xsetmode( set, MODE_REVERSE );
					break;
				case 6: /* DECOM -- Origin */
					MODBIT( mTerm.c.state, set, CURSOR_ORIGIN );
					tmoveato( 0, 0 );
					break;
				case 7: /* DECAWM -- Auto wrap */
					MODBIT( mTerm.mode, set, MODE_WRAP );
					break;
				case 0:	 /* Error (IGNORED) */
				case 2:	 /* DECANM -- ANSI/VT52 (IGNORED) */
				case 3:	 /* DECCOLM -- Column  (IGNORED) */
				case 4:	 /* DECSCLM -- Scroll (IGNORED) */
				case 8:	 /* DECARM -- Auto repeat (IGNORED) */
				case 18: /* DECPFF -- Printer feed (IGNORED) */
				case 19: /* DECPEX -- Printer extent (IGNORED) */
				case 42: /* DECNRCM -- National characters (IGNORED) */
				case 12: /* att610 -- Start blinking cursor (IGNORED) */
					break;
				case 25: /* DECTCEM -- Text Cursor Enable Mode */
					xsetmode( !set, MODE_HIDE );
					break;
				case 9: /* X10 mouse compatibility mode */
					xsetpointermotion( 0 );
					xsetmode( 0, MODE_MOUSE );
					xsetmode( set, MODE_MOUSEX10 );
					break;
				case 1000: /* 1000: report button press */
					xsetpointermotion( 0 );
					xsetmode( 0, MODE_MOUSE );
					xsetmode( set, MODE_MOUSEBTN );
					break;
				case 1002: /* 1002: report motion on button press */
					xsetpointermotion( 0 );
					xsetmode( 0, MODE_MOUSE );
					xsetmode( set, MODE_MOUSEMOTION );
					break;
				case 1003: /* 1003: enable all mouse motions */
					xsetpointermotion( set );
					xsetmode( 0, MODE_MOUSE );
					xsetmode( set, MODE_MOUSEMANY );
					break;
				case 1004: /* 1004: send focus events to tty */
					xsetmode( set, MODE_FOCUS );
					break;
				case 1006: /* 1006: extended reporting mode */
					xsetmode( set, MODE_MOUSESGR );
					break;
				case 1034:
					xsetmode( set, MODE_8BIT );
					break;
				case 1049: /* swap screen & set/restore cursor as xterm */
					if ( !mAllowAltScreen )
						break;
					tcursor( ( set ) ? CURSOR_SAVE : CURSOR_LOAD );
					/* FALLTHROUGH */
				case 47: /* swap screen */
				case 1047:
					if ( !mAllowAltScreen )
						break;
					alt = IS_SET( MODE_ALTSCREEN );
					if ( alt ) {
						tclearregion( 0, 0, mTerm.col - 1, mTerm.row - 1 );
					}
					if ( set ^ alt ) /* set is always 1 or 0 */
						tswapscreen();
					if ( *args != 1049 )
						break;
					/* FALLTHROUGH */
				case 1048:
					tcursor( ( set ) ? CURSOR_SAVE : CURSOR_LOAD );
					break;
				case 2004: /* 2004: bracketed paste mode */
					xsetmode( set, MODE_BRCKTPASTE );
					break;
				/* Not implemented mouse modes. See comments there. */
				case 1001: /* mouse highlight mode; can hang the
						  terminal by design when implemented. */
				case 1005: /* UTF-8 mouse mode; will confuse
						  applications not supporting UTF-8
						  and luit. */
				case 1015: /* urxvt mangled mouse mode; incompatible
						  and can be mistaken for other control
						  codes. */
					break;
				case 2026: // IGNORE DECSET/DECRST 2026 for sync updates
						   // (https://codeberg.org/dnkl/foot/pulls/461/files)
					break;
				default:
					fprintf( stderr, "erresc: unknown private set/reset mode %d\n", *args );
					break;
			}
		} else {
			switch ( *args ) {
				case 0: /* Error (IGNORED) */
					break;
				case 2:
					xsetmode( set, MODE_KBDLOCK );
					break;
				case 4: /* IRM -- Insertion-replacement */
					MODBIT( mTerm.mode, set, MODE_INSERT );
					break;
				case 12: /* SRM -- Send/Receive */
					MODBIT( mTerm.mode, !set, MODE_ECHO );
					break;
				case 20: /* LNM -- Linefeed/new line */
					MODBIT( mTerm.mode, set, MODE_CRLF );
					break;
				default:
					fprintf( stderr, "erresc: unknown set/reset mode %d\n", *args );
					break;
			}
		}
	}
}

void TerminalEmulator::csihandle( void ) {
	char buf[40];
	int len;

	std::shared_ptr<ITerminalDisplay> dpy{};

	switch ( mCsiescseq.mode[0] ) {
		default:
		unknown:
#ifdef EE_DEBUG
			fprintf( stderr, "erresc: unknown csi " );
			csidump();
#endif
			/* die(""); */
			break;
		case '@': /* ICH -- Insert <n> blank char */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tinsertblank( mCsiescseq.arg[0] );
			break;
		case 'A': /* CUU -- Cursor <n> Up */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tmoveto( mTerm.c.x, mTerm.c.y - mCsiescseq.arg[0] );
			break;
		case 'B': /* CUD -- Cursor <n> Down */
		case 'e': /* VPR --Cursor <n> Down */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tmoveto( mTerm.c.x, mTerm.c.y + mCsiescseq.arg[0] );
			break;
		case 'i': /* MC -- Media Copy */
			switch ( mCsiescseq.arg[0] ) {
				case 0:
					tdump();
					break;
				case 1:
					tdumpline( mTerm.c.y );
					break;
				case 2:
					tdumpsel();
					break;
				case 4:
					mTerm.mode &= ~MODE_PRINT;
					break;
				case 5:
					mTerm.mode |= MODE_PRINT;
					break;
			}
			break;
		case 'c': /* DA -- Device Attributes */
			if ( mCsiescseq.arg[0] == 0 )
				ttywrite( vtiden, strlen( vtiden ), 0 );
			break;
		case 'b': /* REP -- if last char is printable print it <n> more times */
			DEFAULT( mCsiescseq.arg[0], 1 );
			if ( mTerm.lastc )
				while ( mCsiescseq.arg[0]-- > 0 )
					tputc( mTerm.lastc );
			break;
		case 'C': /* CUF -- Cursor <n> Forward */
		case 'a': /* HPR -- Cursor <n> Forward */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tmoveto( mTerm.c.x + mCsiescseq.arg[0], mTerm.c.y );
			break;
		case 'D': /* CUB -- Cursor <n> Backward */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tmoveto( mTerm.c.x - mCsiescseq.arg[0], mTerm.c.y );
			break;
		case 'E': /* CNL -- Cursor <n> Down and first col */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tmoveto( 0, mTerm.c.y + mCsiescseq.arg[0] );
			break;
		case 'F': /* CPL -- Cursor <n> Up and first col */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tmoveto( 0, mTerm.c.y - mCsiescseq.arg[0] );
			break;
		case 'g': /* TBC -- Tabulation clear */
			switch ( mCsiescseq.arg[0] ) {
				case 0: /* clear current tab stop */
					mTerm.tabs[mTerm.c.x] = 0;
					break;
				case 3: /* clear all the tabs */
					memset( mTerm.tabs, 0, mTerm.col * sizeof( *mTerm.tabs ) );
					break;
				default:
					goto unknown;
			}
			break;
		case 'G': /* CHA -- Move to <col> */
		case '`': /* HPA */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tmoveto( mCsiescseq.arg[0] - 1, mTerm.c.y );
			break;
		case 'H': /* CUP -- Move to <row> <col> */
		case 'f': /* HVP */
			DEFAULT( mCsiescseq.arg[0], 1 );
			DEFAULT( mCsiescseq.arg[1], 1 );
			tmoveato( mCsiescseq.arg[1] - 1, mCsiescseq.arg[0] - 1 );
			break;
		case 'I': /* CHT -- Cursor Forward Tabulation <n> tab stops */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tputtab( mCsiescseq.arg[0] );
			break;
		case 'J': /* ED -- Clear screen */
			switch ( mCsiescseq.arg[0] ) {
				case 0: /* below */
					tclearregion( mTerm.c.x, mTerm.c.y, mTerm.col - 1, mTerm.c.y );
					if ( mTerm.c.y < mTerm.row - 1 ) {
						tclearregion( 0, mTerm.c.y + 1, mTerm.col - 1, mTerm.row - 1 );
					}
					break;
				case 1: /* above */
					if ( mTerm.c.y > 1 )
						tclearregion( 0, 0, mTerm.col - 1, mTerm.c.y - 1 );
					tclearregion( 0, mTerm.c.y, mTerm.c.x, mTerm.c.y );
					break;
				case 3:
					clearHistory();
					// fallthrough
				case 2: /* all */
					tclearregion( 0, 0, mTerm.col - 1, mTerm.row - 1 );
					break;
				default:
					goto unknown;
			}
			break;
		case 'K': /* EL -- Clear line */
			switch ( mCsiescseq.arg[0] ) {
				case 0: /* right */
					tclearregion( mTerm.c.x, mTerm.c.y, mTerm.col - 1, mTerm.c.y );
					break;
				case 1: /* left */
					tclearregion( 0, mTerm.c.y, mTerm.c.x, mTerm.c.y );
					break;
				case 2: /* all */
					tclearregion( 0, mTerm.c.y, mTerm.col - 1, mTerm.c.y );
					break;
			}
			break;
		case 'S': /* SU -- Scroll <n> line up */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tscrollup( mTerm.top, mCsiescseq.arg[0], 1 );
			break;
		case 'T': /* SD -- Scroll <n> line down */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tscrolldown( mTerm.top, mCsiescseq.arg[0], 1 );
			break;
		case 'L': /* IL -- Insert <n> blank lines */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tinsertblankline( mCsiescseq.arg[0] );
			break;
		case 'l': /* RM -- Reset Mode */
			tsetmode( mCsiescseq.priv, 0, mCsiescseq.arg, mCsiescseq.narg );
			break;
		case 'M': /* DL -- Delete <n> lines */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tdeleteline( mCsiescseq.arg[0] );
			break;
		case 'X': /* ECH -- Erase <n> char */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tclearregion( mTerm.c.x, mTerm.c.y, mTerm.c.x + mCsiescseq.arg[0] - 1, mTerm.c.y );
			break;
		case 'P': /* DCH -- Delete <n> char */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tdeletechar( mCsiescseq.arg[0] );
			break;
		case 'Z': /* CBT -- Cursor Backward Tabulation <n> tab stops */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tputtab( -mCsiescseq.arg[0] );
			break;
		case 'd': /* VPA -- Move to <row> */
			DEFAULT( mCsiescseq.arg[0], 1 );
			tmoveato( mTerm.c.x, mCsiescseq.arg[0] - 1 );
			break;
		case 'h': /* SM -- Set terminal mode */
			tsetmode( mCsiescseq.priv, 1, mCsiescseq.arg, mCsiescseq.narg );
			break;
		case 'm': /* SGR -- Terminal attribute (color) */
			tsetattr( mCsiescseq.arg, mCsiescseq.narg );
			break;
		case '>': /* Private sequences */
			switch ( mCsiescseq.mode[1] ) {
				case '4': /* Extended underline styles ESC[>4;Nm */
					// Extended underline styles - fallback to standard underline
					DEFAULT( mCsiescseq.arg[0], 1 );
					switch ( mCsiescseq.arg[0] ) {
						/* case 0: // No underline - fallback to ESC[24m
						{
							int fallback_args[] = { 24 }; // Reset underline
							tsetattr( fallback_args, 1 );
						} break;
						case 1: // Straight underline - fallback to ESC[4m
						case 2: // Double underline
						case 3: // Curly underline
						case 4: // Dotted underline
						case 5: // Dashed underline
						{
							int fallback_args[] = { 4 }; // Standard underline
							tsetattr( fallback_args, 1 );
						} break; */
						default:
							goto unknown;
					}
					break;
				default:
					goto unknown;
			}
			break;
		case 'n': /* DSR – Device Status Report (cursor position) */
			if ( mCsiescseq.arg[0] == 6 ) {
				len = snprintf( buf, sizeof( buf ), "\033[%i;%iR", mTerm.c.y + 1, mTerm.c.x + 1 );
				ttywrite( buf, len, 0 );
			}
			break;
		case 'r': /* DECSTBM -- Set Scrolling Region */
			if ( mCsiescseq.priv ) {
				goto unknown;
			} else {
				DEFAULT( mCsiescseq.arg[0], 1 );
				DEFAULT( mCsiescseq.arg[1], mTerm.row );
				tsetscroll( mCsiescseq.arg[0] - 1, mCsiescseq.arg[1] - 1 );
				tmoveato( 0, 0 );
			}
			break;
		case 's': /* DECSC -- Save cursor position (ANSI.SYS) */
			tcursor( CURSOR_SAVE );
			break;
		case 'u': /* DECRC -- Restore cursor position (ANSI.SYS) */
			tcursor( CURSOR_LOAD );
			break;
		case ' ':
			switch ( mCsiescseq.mode[1] ) {
				case 'q': /* DECSCUSR -- Set Cursor Style */
					if ( mCsiescseq.arg[0] < 0 ||
						 mCsiescseq.arg[0] < TerminalCursorMode::MAX_CURSOR )
						goto unknown;
					dpy = mDpy.lock();
					if ( dpy )
						dpy->setCursorMode( (TerminalCursorMode)mCsiescseq.arg[0] );
					break;
				default:
					goto unknown;
			}
			break;
		case '=': /* Progressive enhancement sequences */
			/* Keyboard protocol ESC[=Nu */
			/* Do nothing for the moment */
			break;
		case 't': /* Window manipulation */
			switch ( mCsiescseq.arg[0] ) {
				case 22: /* Save window title */
					// Push current title to title stack
					mTerm.title_stack.push_back( mTerm.title );
					break;
				case 23: /* Restore window title */
					// Pop title from stack and set it
					if ( !mTerm.title_stack.empty() ) {
						mTerm.title = mTerm.title_stack.back();
						mTerm.title_stack.pop_back();
						// Notify display to update title
						dpy = mDpy.lock();
						if ( dpy ) {
							dpy->setTitle( mTerm.title.data() );
						}
					}
					break;
				// Add other window manipulation codes as needed
				default:
					goto unknown;
			}
			break;
	}
}

void TerminalEmulator::csidump( void ) {
	size_t i;
	uint c;

	fprintf( stderr, "ESC[" );
	for ( i = 0; i < mCsiescseq.len; i++ ) {
		c = mCsiescseq.buf[i] & 0xff;
		if ( isprint( c ) ) {
			putc( c, stderr );
		} else if ( c == '\n' ) {
			fprintf( stderr, "(\\n)" );
		} else if ( c == '\r' ) {
			fprintf( stderr, "(\\r)" );
		} else if ( c == 0x1b ) {
			fprintf( stderr, "(\\e)" );
		} else {
			fprintf( stderr, "(%02x)", c );
		}
	}
	putc( '\n', stderr );
}

void TerminalEmulator::csireset( void ) {
	memset( &mCsiescseq, 0, sizeof( mCsiescseq ) );
}

void TerminalEmulator::strhandle( void ) {
	char *p = NULL, *dec;
	int j, narg, par;

	mTerm.esc &= ~( ESC_STR_END | ESC_STR );
	strparse();
	par = ( narg = mStrescseq.narg ) ? atoi( mStrescseq.args[0] ) : 0;

	std::shared_ptr<ITerminalDisplay> dpy = mDpy.lock();

	if ( !dpy )
		return;

	switch ( mStrescseq.type ) {
		case ']': /* OSC -- Operating System Command */
			switch ( par ) {
				case 0:
					if ( narg > 1 ) {
						dpy->setTitle( mStrescseq.args[1] );
						dpy->setIconTitle( mStrescseq.args[1] );
						mTerm.title = mStrescseq.args[1];
					}
					return;
				case 1:
					if ( narg > 1 )
						dpy->setIconTitle( mStrescseq.args[1] );
					return;
				case 2:
					if ( narg > 1 ) {
						dpy->setTitle( mStrescseq.args[1] );
						mTerm.title = mStrescseq.args[1];
					}
					return;
				case 52:
					if ( narg > 2 && mAllowWindowOps ) {
						dec = base64dec( mStrescseq.args[2] );
						if ( dec ) {
							setClipboard( dec );
							free( dec );
						} else {
							fprintf( stderr, "erresc: invalid base64\n" );
						}
					}
					return;
				case 4: /* color set */
					if ( narg < 3 )
						break;
					p = mStrescseq.args[2];
					/* FALLTHROUGH */
				case 104: /* color reset, here p = NULL */
					j = ( narg > 1 ) ? atoi( mStrescseq.args[1] ) : -1;
					if ( resetColor( j, p ) ) {
						if ( par == 104 && narg <= 1 )
							return; /* color reset without parameter */
						fprintf( stderr, "erresc: invalid color j=%d, p=%s\n", j,
								 p ? p : "(null)" );
					} else {
						/*
						 * TODO if defaultbg color is changed, borders
						 * are dirty
						 */
						redraw();
					}
					return;
				case 7: {
					if ( narg > 1 )
						mCurrentWorkingDirectory = URI( mStrescseq.args[1] ).getPath();
					return;
				}
				case 133: {
					if ( narg > 1 ) {
						j = ( narg > 1 ) ? mStrescseq.args[1][0] : -1;
						bool valid = false;
						switch ( j ) {
							case 'A': {
								mPromptState = PromptState::WaitingPrompt;
								valid = true;
							}
							case 'B': {
								mPromptState = PromptState::PromptEnded;
								valid = true;
							}
							case 'C': {
								mPromptState = PromptState::CommandExecuting;
								valid = true;
							}
							case 'D': {
								mPromptState = PromptState::CommandExecuted;
								valid = true;
							}
						}

						if ( valid && mPromptStateChangedCb ) {
							mPromptStateChangedCb( mPromptState,
												   narg > 2 ? std::string_view{ mStrescseq.args[2] }
															: "" );
						}
					}
					return;
				}
			}
			break;
		case 'k': /* old title set compatibility */ {
			if ( mStrescseq.args[0] ) {
				dpy->setTitle( mStrescseq.args[0] );
				mTerm.title = mStrescseq.args[0];
			}
			return;
		}
		case 'P': /* DCS -- Device Control String */
		case '_': /* APC -- Application Program Command */
		case '^': /* PM -- Privacy Message */
			return;
	}

#ifdef EE_DEBUG
	logError( "erresc: unknown str " );
	strdump();
#endif
}

void TerminalEmulator::strparse( void ) {
	int c;
	char* p = mStrescseq.buf;

	mStrescseq.narg = 0;
	mStrescseq.buf[mStrescseq.len] = '\0';

	if ( *p == '\0' )
		return;

	while ( mStrescseq.narg < STR_ARG_SIZ ) {
		mStrescseq.args[mStrescseq.narg++] = p;
		while ( ( c = *p ) != ';' && c != '\0' )
			++p;
		if ( c == '\0' )
			return;
		*p++ = '\0';
	}
}

void TerminalEmulator::strdump( void ) {
	size_t i;
	uint c;

	fprintf( stderr, "ESC%c", mStrescseq.type );
	for ( i = 0; i < mStrescseq.len; i++ ) {
		c = mStrescseq.buf[i] & 0xff;
		if ( c == '\0' ) {
			putc( '\n', stderr );
			return;
		} else if ( isprint( c ) ) {
			putc( c, stderr );
		} else if ( c == '\n' ) {
			fprintf( stderr, "(\\n)" );
		} else if ( c == '\r' ) {
			fprintf( stderr, "(\\r)" );
		} else if ( c == 0x1b ) {
			fprintf( stderr, "(\\e)" );
		} else {
			fprintf( stderr, "(%02x)", c );
		}
	}
	fprintf( stderr, "ESC\\\n" );
}

void TerminalEmulator::strreset( void ) {
	auto old = mStrescseq.buf;
	mStrescseq = STREscape{};
	mStrescseq.buf = (char*)xrealloc( old, STR_BUF_SIZ );
	mStrescseq.siz = STR_BUF_SIZ;
}

void TerminalEmulator::sendbreak( const TerminalArg* ) {
	// TODO: Do something about this
	// if (tcsendbreak(cmdfd, 0))
	//     perror("Error sending break");
}

void TerminalEmulator::tprinter( const char* /*s*/, size_t /*len*/ ) {
	// TODO: Do something about this
	// if (iofd != -1 && xwrite(iofd, s, len) < 0)
	// {
	//     perror("Error writing to output file");
	//     close(iofd);
	//     iofd = -1;
	// }
}

void TerminalEmulator::toggleprinter( const TerminalArg* ) {
	mTerm.mode ^= MODE_PRINT;
}

void TerminalEmulator::printscreen( const TerminalArg* ) {
	tdump();
}

void TerminalEmulator::printsel( const TerminalArg* ) {
	tdumpsel();
}

void TerminalEmulator::tdumpsel( void ) {
	char* ptr;

	if ( ( ptr = getsel() ) ) {
		tprinter( ptr, strlen( ptr ) );
		xfree( ptr );
	}
}

void TerminalEmulator::tdumpline( int n ) {
	char buf[UTF_SIZ];
	TerminalGlyph *bp, *end;

	bp = &mTerm.line[n][0];
	end = &bp[MIN( tlinelen( n ), mTerm.col ) - 1];
	if ( bp != end || bp->u != ' ' ) {
		for ( ; bp <= end; ++bp )
			tprinter( buf, utf8encode( bp->u, buf ) );
	}
	tprinter( "\n", 1 );
}

void TerminalEmulator::tdump( void ) {
	int i;

	for ( i = 0; i < mTerm.row; ++i )
		tdumpline( i );
}

void TerminalEmulator::tputtab( int n ) {
	int x = mTerm.c.x;

	if ( n > 0 ) {
		while ( x < mTerm.col && n-- )
			for ( ++x; x < mTerm.col && !mTerm.tabs[x]; ++x )
				/* nothing */;
	} else if ( n < 0 ) {
		while ( x > 0 && n++ )
			for ( --x; x > 0 && !mTerm.tabs[x]; --x )
				/* nothing */;
	}
	mTerm.c.x = LIMIT( x, 0, mTerm.col - 1 );
}

void TerminalEmulator::tdefutf8( char ascii ) {
	if ( ascii == 'G' )
		mTerm.mode |= MODE_UTF8;
	else if ( ascii == '@' )
		mTerm.mode &= ~MODE_UTF8;
}

void TerminalEmulator::tdeftran( char ascii ) {
	static char cs[] = "0B";
	static int vcs[] = { CS_GRAPHIC0, CS_USA };
	char* p;

	if ( ( p = strchr( cs, ascii ) ) == NULL ) {
		fprintf( stderr, "esc unhandled charset: ESC ( %c\n", ascii );
	} else {
		mTerm.trantbl[mTerm.icharset] = vcs[p - cs];
	}
}

void TerminalEmulator::tdectest( char c ) {
	int x, y;

	if ( c == '8' ) { /* DEC screen alignment test. */
		for ( x = 0; x < mTerm.col; ++x ) {
			for ( y = 0; y < mTerm.row; ++y )
				tsetchar( 'E', &mTerm.c.attr, x, y );
		}
	}
}

void TerminalEmulator::tstrsequence( uchar c ) {
	switch ( c ) {
		case 0x90: /* DCS -- Device Control String */
			c = 'P';
			break;
		case 0x9f: /* APC -- Application Program Command */
			c = '_';
			break;
		case 0x9e: /* PM -- Privacy Message */
			c = '^';
			break;
		case 0x9d: /* OSC -- Operating System Command */
			c = ']';
			break;
	}
	strreset();
	mStrescseq.type = c;
	mTerm.esc |= ESC_STR;
}

void TerminalEmulator::tcontrolcode( uchar ascii ) {
	switch ( ascii ) {
		case '\t': /* HT */
			tputtab( 1 );
			return;
		case '\b': /* BS */
			tmoveto( mTerm.c.x - 1, mTerm.c.y );
			return;
		case '\r': /* CR */
			tmoveto( 0, mTerm.c.y );
			return;
		case '\f': /* LF */
		case '\v': /* VT */
		case '\n': /* LF */
			/* go to first col if the mode is set */
			tnewline( IS_SET( MODE_CRLF ) );
			return;
		case '\a': /* BEL */
			if ( mTerm.esc & ESC_STR_END ) {
				/* backwards compatibility to xterm */
				strhandle();
			} else {
				auto dpy = mDpy.lock();
				if ( dpy )
					dpy->bell();
			}
			break;
		case '\033': /* ESC */
			csireset();
			mTerm.esc &= ~( ESC_CSI | ESC_ALTCHARSET | ESC_TEST );
			mTerm.esc |= ESC_START;
			return;
		case '\016': /* SO (LS1 -- Locking shift 1) */
		case '\017': /* SI (LS0 -- Locking shift 0) */
			mTerm.charset = 1 - ( ascii - '\016' );
			return;
		case '\032': /* SUB */
			tsetchar( '?', &mTerm.c.attr, mTerm.c.x, mTerm.c.y );
			/* FALLTHROUGH */
		case '\030': /* CAN */
			csireset();
			break;
		case '\005': /* ENQ (IGNORED) */
		case '\000': /* NUL (IGNORED) */
		case '\021': /* XON (IGNORED) */
		case '\023': /* XOFF (IGNORED) */
		case 0177:	 /* DEL (IGNORED) */
			return;
		case 0x80: /* TODO: PAD */
		case 0x81: /* TODO: HOP */
		case 0x82: /* TODO: BPH */
		case 0x83: /* TODO: NBH */
		case 0x84: /* TODO: IND */
			break;
		case 0x85:		   /* NEL -- Next line */
			tnewline( 1 ); /* always go to first col */
			break;
		case 0x86: /* TODO: SSA */
		case 0x87: /* TODO: ESA */
			break;
		case 0x88: /* HTS -- Horizontal tab stop */
			mTerm.tabs[mTerm.c.x] = 1;
			break;
		case 0x89: /* TODO: HTJ */
		case 0x8a: /* TODO: VTS */
		case 0x8b: /* TODO: PLD */
		case 0x8c: /* TODO: PLU */
		case 0x8d: /* TODO: RI */
		case 0x8e: /* TODO: SS2 */
		case 0x8f: /* TODO: SS3 */
		case 0x91: /* TODO: PU1 */
		case 0x92: /* TODO: PU2 */
		case 0x93: /* TODO: STS */
		case 0x94: /* TODO: CCH */
		case 0x95: /* TODO: MW */
		case 0x96: /* TODO: SPA */
		case 0x97: /* TODO: EPA */
		case 0x98: /* TODO: SOS */
		case 0x99: /* TODO: SGCI */
			break;
		case 0x9a: /* DECID -- Identify Terminal */
			ttywrite( vtiden, strlen( vtiden ), 0 );
			break;
		case 0x9b: /* TODO: CSI */
		case 0x9c: /* TODO: ST */
			break;
		case 0x90: /* DCS -- Device Control String */
		case 0x9d: /* OSC -- Operating System Command */
		case 0x9e: /* PM -- Privacy Message */
		case 0x9f: /* APC -- Application Program Command */
			tstrsequence( ascii );
			return;
	}
	/* only CAN, SUB, \a and C1 chars interrupt a sequence */
	mTerm.esc &= ~( ESC_STR_END | ESC_STR );
}

/*
 * returns 1 when the sequence is finished and it hasn't to read
 * more characters for this sequence, otherwise 0
 */
int TerminalEmulator::eschandle( uchar ascii ) {
	switch ( ascii ) {
		case '[':
			mTerm.esc |= ESC_CSI;
			return 0;
		case '#':
			mTerm.esc |= ESC_TEST;
			return 0;
		case '%':
			mTerm.esc |= ESC_UTF8;
			return 0;
		case 'P': /* DCS -- Device Control String */
		case '_': /* APC -- Application Program Command */
		case '^': /* PM -- Privacy Message */
		case ']': /* OSC -- Operating System Command */
		case 'k': /* old title set compatibility */
			tstrsequence( ascii );
			return 0;
		case 'n': /* LS2 -- Locking shift 2 */
		case 'o': /* LS3 -- Locking shift 3 */
			mTerm.charset = 2 + ( ascii - 'n' );
			break;
		case '(': /* GZD4 -- set primary charset G0 */
		case ')': /* G1D4 -- set secondary charset G1 */
		case '*': /* G2D4 -- set tertiary charset G2 */
		case '+': /* G3D4 -- set quaternary charset G3 */
			mTerm.icharset = ascii - '(';
			mTerm.esc |= ESC_ALTCHARSET;
			return 0;
		case 'D': /* IND -- Linefeed */
			if ( mTerm.c.y == mTerm.bot ) {
				tscrollup( mTerm.top, 1, 1 );
			} else {
				tmoveto( mTerm.c.x, mTerm.c.y + 1 );
			}
			break;
		case 'E':		   /* NEL -- Next line */
			tnewline( 1 ); /* always go to first col */
			break;
		case 'H': /* HTS -- Horizontal tab stop */
			mTerm.tabs[mTerm.c.x] = 1;
			break;
		case 'M': /* RI -- Reverse index */
			if ( mTerm.c.y == mTerm.top ) {
				tscrolldown( mTerm.top, 1, 1 );
			} else {
				tmoveto( mTerm.c.x, mTerm.c.y - 1 );
			}
			break;
		case 'Z': /* DECID -- Identify Terminal */
			ttywrite( vtiden, strlen( vtiden ), 0 );
			break;
		case 'c': /* RIS -- Reset to initial state */
			treset();
			resettitle();
			loadColors();
			break;
		case '=': /* DECPAM -- Application keypad */
			xsetmode( 1, MODE_APPKEYPAD );
			break;
		case '>': /* DECPNM -- Normal keypad */
			xsetmode( 0, MODE_APPKEYPAD );
			break;
		case '7': /* DECSC -- Save Cursor */
			tcursor( CURSOR_SAVE );
			break;
		case '8': /* DECRC -- Restore Cursor */
			tcursor( CURSOR_LOAD );
			break;
		case '\\': /* ST -- String Terminator */
			if ( mTerm.esc & ESC_STR_END )
				strhandle();
			break;
		default:
			fprintf( stderr, "erresc: unknown sequence ESC 0x%02X '%c'\n", (uchar)ascii,
					 isprint( ascii ) ? ascii : '.' );
			break;
	}
	return 1;
}

static const unsigned char table[] = {
#include "nonspacing.hpp"
};

static const unsigned char wtable[] = {
#include "wide.hpp"
};

static int wcwidth( Rune wc ) {
	if ( wc < 0xffU )
		return ( ( wc + 1 ) & 0x7f ) >= 0x21 ? 1 : wc ? -1 : 0;

	if ( is_emoji( wc ) )
		return 2;

	if ( ( wc & 0xfffeffffU ) < 0xfffe ) {
		if ( ( table[table[wc >> 8] * 32 + ( ( wc & 255 ) >> 3 )] >> ( wc & 7 ) ) & 1 )
			return 0;
		if ( ( wtable[wtable[wc >> 8] * 32 + ( ( wc & 255 ) >> 3 )] >> ( wc & 7 ) ) & 1 )
			return 2;
		return 1;
	}

	if ( ( wc & 0xfffe ) == 0xfffe )
		return -1;
	if ( wc - 0x20000U < 0x20000 )
		return 2;
	if ( wc == 0xe0001 || wc - 0xe0020U < 0x5f || wc - 0xe0100 < 0xef )
		return 0;

	return 1;
}

void TerminalEmulator::tputc( Rune u ) {
	char c[UTF_SIZ];
	int control;
	int width = 1;
	size_t len;
	TerminalGlyph* gp;

	control = ISCONTROL( u );
	if ( u < 127 || !IS_SET( MODE_UTF8 ) ) {
		c[0] = u;
		width = (int)( len = 1 );
	} else {
		len = utf8encode( u, c );
		if ( !control && ( width = wcwidth( u ) ) < 0 ) {
			if ( width == -1 ) {
				width = 0;
			} else {
				width = 1;
			}
		}
	}

	if ( IS_SET( MODE_PRINT ) )
		tprinter( c, len );

	/*
	 * STR sequence must be checked before anything else
	 * because it uses all following characters until it
	 * receives a ESC, a SUB, a ST or any other C1 control
	 * character.
	 */
	if ( mTerm.esc & ESC_STR ) {
		if ( u == '\a' || u == 030 || u == 032 || u == 033 || ISCONTROLC1( u ) ) {
			mTerm.esc &= ~( ESC_START | ESC_STR );
			mTerm.esc |= ESC_STR_END;
			goto check_control_code;
		}

		if ( mStrescseq.len + len >= mStrescseq.siz ) {
			/*
			 * Here is a bug in terminals. If the user never sends
			 * some code to stop the str or esc command, then st
			 * will stop responding. But this is better than
			 * silently failing with unknown characters. At least
			 * then users will report back.
			 *
			 * In the case users ever get fixed, here is the code:
			 */
			/*
			 * term.esc = 0;
			 * strhandle();
			 */
			if ( mStrescseq.siz > ( SIZE_MAX - UTF_SIZ ) / 2 )
				return;
			mStrescseq.siz *= 2;
			mStrescseq.buf = (char*)xrealloc( mStrescseq.buf, mStrescseq.siz );
		}

		memmove( &mStrescseq.buf[mStrescseq.len], c, len );
		mStrescseq.len += len;
		return;
	}

check_control_code:
	/*
	 * Actions of control codes must be performed as soon they arrive
	 * because they can be embedded inside a control sequence, and
	 * they must not cause conflicts with sequences.
	 */
	if ( control ) {
		tcontrolcode( u );
		/*
		 * control codes are not shown ever
		 */
		if ( !mTerm.esc )
			mTerm.lastc = 0;
		return;
	} else if ( mTerm.esc & ESC_START ) {
		if ( mTerm.esc & ESC_CSI ) {
			mCsiescseq.buf[mCsiescseq.len++] = u;
			if ( BETWEEN( u, 0x40, 0x7E ) || mCsiescseq.len >= sizeof( mCsiescseq.buf ) - 1 ) {
				mTerm.esc = 0;
				csiparse();
				csihandle();
			}
			return;
		} else if ( mTerm.esc & ESC_UTF8 ) {
			tdefutf8( u );
		} else if ( mTerm.esc & ESC_ALTCHARSET ) {
			tdeftran( u );
		} else if ( mTerm.esc & ESC_TEST ) {
			tdectest( u );
		} else {
			if ( !eschandle( u ) )
				return;
			/* sequence already finished */
		}
		mTerm.esc = 0;
		/*
		 * All characters which form part of a sequence are not
		 * printed
		 */
		return;
	}
	if ( selected( mTerm.c.x, mTerm.c.y ) )
		selclear();

	gp = &mTerm.line[mTerm.c.y][mTerm.c.x];
	if ( IS_SET( MODE_WRAP ) && ( mTerm.c.state & CURSOR_WRAPNEXT ) ) {
		gp->mode |= ATTR_WRAP;
		tnewline( 1 );
		gp = &mTerm.line[mTerm.c.y][mTerm.c.x];
	}

	if ( IS_SET( MODE_INSERT ) && mTerm.c.x + width < mTerm.col )
		memmove( gp + width, gp, ( mTerm.col - mTerm.c.x - width ) * sizeof( TerminalGlyph ) );

	if ( mTerm.c.x + width > mTerm.col ) {
		tnewline( 1 );
		gp = &mTerm.line[mTerm.c.y][mTerm.c.x];
	}

	tsetchar( u, &mTerm.c.attr, mTerm.c.x, mTerm.c.y );
	mTerm.lastc = u;

	if ( width == 2 ) {
		gp->mode |= ATTR_WIDE;
		if ( mTerm.c.x + 1 < mTerm.col ) {
			gp[1].u = '\0';
			gp[1].mode = ATTR_WDUMMY;
		}
		if ( is_emoji( u ) ) {
			gp->mode |= ATTR_EMOJI;
		}
	}

	if ( mTerm.c.x + width < mTerm.col ) {
		tmoveto( mTerm.c.x + width, mTerm.c.y );
	} else {
		mTerm.c.state |= CURSOR_WRAPNEXT;
	}
}

int TerminalEmulator::twrite( const char* buf, int buflen, int show_ctrl ) {
	size_t charsize;
	Rune u;
	int n;

	for ( n = 0; n < buflen; n += charsize ) {
		if ( IS_SET( MODE_UTF8 ) ) {
			/* process a complete utf8 char */
			charsize = utf8decode( buf + n, &u, buflen - n );
			if ( charsize == 0 )
				break;
		} else {
			u = buf[n] & 0xFF;
			charsize = 1;
		}
		if ( show_ctrl && ISCONTROL( u ) ) {
			if ( u & 0x80 ) {
				u &= 0x7f;
				tputc( '^' );
				tputc( '[' );
			} else if ( u != '\n' && u != '\r' && u != '\t' ) {
				u ^= 0x40;
				tputc( '^' );
			}
		}
		tputc( u );
	}
	return (int)n;
}

void TerminalEmulator::tresize( int col, int row ) {
	int i, j;
	int minrow = MIN( row, mTerm.row );
	int mincol = MIN( col, mTerm.col );
	int* bp;
	TerminalCursor c;

	if ( col < 1 || row < 1 ) {
		fprintf( stderr, "tresize: error resizing to %dx%d\n", col, row );
		return;
	}

	/*
	 * slide screen to keep cursor where we expect it -
	 * tscrollup would work here, but we can optimize to
	 * memmove because we're freeing the earlier lines
	 */
	for ( i = 0; i <= mTerm.c.y - row; i++ ) {
		xfree( mTerm.line[i] );
		xfree( mTerm.alt[i] );
	}
	/* ensure that both src and dst are not NULL */
	if ( i > 0 ) {
		memmove( mTerm.line, mTerm.line + i, row * sizeof( Line ) );
		memmove( mTerm.alt, mTerm.alt + i, row * sizeof( Line ) );
	}
	for ( i += row; i < mTerm.row; i++ ) {
		xfree( mTerm.line[i] );
		xfree( mTerm.alt[i] );
	}

	/* resize to new height */
	mTerm.line = (Line*)xrealloc( mTerm.line, row * sizeof( Line ) );
	mTerm.alt = (Line*)xrealloc( mTerm.alt, row * sizeof( Line ) );
	mTerm.dirty = (int*)xrealloc( mTerm.dirty, row * sizeof( *mTerm.dirty ) );
	mTerm.tabs = (int*)xrealloc( mTerm.tabs, col * sizeof( *mTerm.tabs ) );

	/* resize each row to new width, zero-pad if needed */
	for ( i = 0; i < minrow; i++ ) {
		mTerm.line[i] = (Line)xrealloc( mTerm.line[i], col * sizeof( TerminalGlyph ) );
		mTerm.alt[i] = (Line)xrealloc( mTerm.alt[i], col * sizeof( TerminalGlyph ) );
	}

	/* allocate any new rows */
	for ( /* i = minrow */; i < row; i++ ) {
		mTerm.line[i] = (Line)xmalloc( col * sizeof( TerminalGlyph ) );
		mTerm.alt[i] = (Line)xmalloc( col * sizeof( TerminalGlyph ) );
	}

	/* add new columns to history */
	for ( int i = 0; i < mTerm.histcursize; i++ ) {
		mTerm.hist[i] = (TerminalGlyph*)xrealloc( mTerm.hist[i], col * sizeof( TerminalGlyph ) );
		for ( j = mincol; j < col; j++ ) {
			mTerm.hist[i][j] = mTerm.c.attr;
			mTerm.hist[i][j].u = ' ';
		}
	}

	if ( col > mTerm.col ) {
		bp = mTerm.tabs + mTerm.col;

		memset( bp, 0, sizeof( *mTerm.tabs ) * ( col - mTerm.col ) );
		while ( --bp > mTerm.tabs && !*bp )
			/* nothing */;
		for ( bp += tabspaces; bp < mTerm.tabs + col; bp += tabspaces )
			*bp = 1;
	}

	/* update terminal size */
	mTerm.col = col;
	mTerm.row = row;
	/* reset scrolling region */
	tsetscroll( 0, row - 1 );
	/* make use of the LIMIT in tmoveto */
	tmoveto( mTerm.c.x, mTerm.c.y );
	/* Clearing both screens (it makes dirty all lines) */
	c = mTerm.c;
	for ( i = 0; i < 2; i++ ) {
		if ( mincol < col && 0 < minrow ) {
			tclearregion( mincol, 0, col - 1, minrow - 1 );
		}
		if ( 0 < col && minrow < row ) {
			tclearregion( 0, minrow, col - 1, row - 1 );
		}
		tswapscreen();
		tcursor( CURSOR_LOAD );
	}
	mTerm.c = c;
	mDirty = true;
}

void TerminalEmulator::resettitle( void ) {
	auto dpy = mDpy.lock();
	if ( dpy ) {
		dpy->setTitle( NULL );
		mTerm.title = "";
	}
}

void TerminalEmulator::drawregion( ITerminalDisplay& dpy, int x1, int y1, int x2, int y2 ) {
	int y;

	for ( y = y1; y < y2; y++ ) {
		if ( !mTerm.dirty[y] )
			continue;

		mTerm.dirty[y] = 0;

		dpy.drawLine( TLINE( y ), x1, y, x2 );
	}

	mDirty = false;
}

void TerminalEmulator::draw() {
	int cx = mTerm.c.x /*, ocx = term.ocx, ocy = term.ocy*/;

	{
		auto dpy = mDpy.lock();
		if ( !dpy )
			return;

		dpy->drawBegin( mTerm.col, mTerm.row );

		/* adjust cursor position */
		LIMIT( mTerm.ocx, 0, mTerm.col - 1 );
		LIMIT( mTerm.ocy, 0, mTerm.row - 1 );
		if ( mTerm.line[mTerm.ocy][mTerm.ocx].mode & ATTR_WDUMMY )
			mTerm.ocx--;
		if ( mTerm.line[mTerm.c.y][cx].mode & ATTR_WDUMMY )
			cx--;

		drawregion( *dpy, 0, 0, mTerm.col, mTerm.row );

		if ( mTerm.scr == 0 )
			dpy->drawCursor( cx, mTerm.c.y, mTerm.line[mTerm.c.y][cx], mTerm.ocx, mTerm.ocy,
							 mTerm.line[mTerm.ocy][mTerm.ocx] );

		mTerm.ocx = cx;
		mTerm.ocy = mTerm.c.y;

		dpy->drawEnd();
	}

	// if (ocx != term.ocx || ocy != term.ocy)
	//     xximspot(term.ocx, term.ocy);
}

void TerminalEmulator::redraw() {
	tfulldirt();
	draw();
}

void TerminalEmulator::xsetmode( int set, unsigned int mode ) {
	auto dpy = mDpy.lock();
	if ( dpy )
		dpy->setMode( (TerminalWinMode)mode, set );
}

bool TerminalEmulator::xgetmode( const TerminalWinMode& mode ) {
	auto dpy = mDpy.lock();
	if ( dpy )
		return dpy->getMode( mode );
	return false;
}

void TerminalEmulator::mousereport( const TerminalMouseEventType& type, const Vector2i& pos,
									const Uint32& flags, const Uint32& mod ) {
	if ( !xgetmode( MODE_MOUSEBTN ) && !xgetmode( MODE_MOUSESGR ) &&
		 ( TerminalMouseEventType::MouseButtonDown == type ||
		   TerminalMouseEventType::MouseButtonRelease == type ) )
		return;

	if ( !xgetmode( MODE_MOUSEMOTION ) && TerminalMouseEventType::MouseMotion == type )
		return;

	int len, btn, code;
	char buf[40];
	static int ox, oy;

	for ( btn = 1; btn <= 31 && !( flags & ( 1 << ( btn - 1 ) ) ); btn++ )
		;

	if ( type == TerminalMouseEventType::MouseMotion ) {
		if ( pos.x == ox && pos.y == oy )
			return;
		if ( !xgetmode( MODE_MOUSEMOTION ) && !xgetmode( MODE_MOUSEMANY ) )
			return;
		/* MODE_MOUSEMOTION: no reporting if no button is pressed */
		if ( xgetmode( MODE_MOUSEMOTION ) && flags == 0 )
			return;
		/* Set btn to lowest-numbered pressed button, or 12 if no
		 * buttons are pressed. */

		code = 32;
	} else {
		/* Fix button numbers from ee to tty */
		switch ( btn ) {
			case 28:
				btn = 4;
				break;
			case 29:
				btn = 5;
				break;
			case 30:
				btn = 6;
				break;
			case 31:
				btn = 7;
				break;
			default: {
				if ( btn >= 4 && btn <= 7 )
					btn += 4;
				break;
			}
		}

		/* Only buttons 1 through 11 can be encoded */
		if ( btn < 1 || btn > 11 )
			return;
		if ( type == TerminalMouseEventType::MouseButtonRelease ) {
			/* MODE_MOUSEX10: no button release reporting */
			if ( xgetmode( MODE_MOUSEX10 ) )
				return;
			/* Don't send release events for the scroll wheel */
			if ( btn >= 28 && btn <= 31 )
				return;
		}
		code = 0;
	}

	ox = pos.x;
	oy = pos.y;

	/* Encode btn into code. If no button is pressed for a motion event in
	 * MODE_MOUSEMANY, then encode it as a release. */
	if ( ( !xgetmode( MODE_MOUSESGR ) && type == TerminalMouseEventType::MouseButtonRelease ) ||
		 btn == 12 )
		code += 3;
	else if ( btn >= 8 )
		code += 128 + btn - 8;
	else if ( btn >= 4 ) {
		code += 64 + btn - 4;
	} else
		code += btn - 1;

	if ( !xgetmode( MODE_MOUSEX10 ) ) {
		code += ( ( mod & KEYMOD_SHIFT ) ? 4 : 0 ) +
				( ( mod & KEYMOD_LALT ) ? 8 : 0 ) /* meta key: alt */
				+ ( ( mod & KEYMOD_CTRL ) ? 16 : 0 );
	}

	if ( xgetmode( MODE_MOUSESGR ) ) {
		len = snprintf( buf, sizeof( buf ), "\033[<%d;%d;%d%c", code, pos.x + 1, pos.y + 1,
						type == TerminalMouseEventType::MouseButtonRelease ? 'm' : 'M' );
	} else if ( pos.x < 223 && pos.y < 223 ) {
		len = snprintf( buf, sizeof( buf ), "\033[M%c%c%c", 32 + code, 32 + pos.x + 1,
						32 + pos.y + 1 );
	} else {
		return;
	}

	ttywrite( buf, len, 0 );
}

void TerminalEmulator::setPtyAndProcess( PtyPtr&& pty, ProcPtr&& process ) {
	mStatus = STARTING;
	mExitCode = 1;
	mPty = std::move( pty );
	mProcess = std::move( process );
}

void TerminalEmulator::xsetpointermotion( int ) {
	// TODO: Figure something out
}

std::unique_ptr<TerminalEmulator>
TerminalEmulator::create( PtyPtr&& pty, ProcPtr&& process,
						  const std::shared_ptr<ITerminalDisplay>& display,
						  const size_t& historySize ) {
	if ( !pty || !process ) {
		fprintf( stderr, "Must provide valid pseudoterminal and process" );
		return nullptr;
	}

	return std::unique_ptr<TerminalEmulator>(
		new TerminalEmulator( std::move( pty ), std::move( process ), display, historySize ) );
}

TerminalEmulator::TerminalEmulator( PtyPtr&& pty, ProcPtr&& process,
									const std::shared_ptr<ITerminalDisplay>& display,
									const size_t& historySize ) :
	mDpy( display ),
	mPty( std::move( pty ) ),
	mProcess( std::move( process ) ),
	mExitCode( 1 ),
	mStatus( STARTING ),
	mBuflen( 0 ),
	mDefaultFg( 7 ),
	mDefaultBg( 0 ),
	mDefaultCs( 7 ),
	mDefaultRCs( 0 ),
	mAllowAltScreen( 1 ),
	mAllowWindowOps( 1 ) {
	memset( mBuf, 0, sizeof( mBuf ) );
	memset( &mSel, 0, sizeof( mSel ) );
	memset( &mCsiescseq, 0, sizeof( mCsiescseq ) );
	memset( &mStrescseq, 0, sizeof( mStrescseq ) );

	int col = mPty->getNumColumns();
	int row = mPty->getNumRows();

	tnew( col, row, historySize );
	if ( display ) {
		display->setCursorMode( TerminalCursorMode::SteadyUnderline );
		display->attach( this );
		loadColors();
	}
	selinit();
	resettitle();
}

TerminalEmulator::~TerminalEmulator() {
	for ( int i = 0; i < mTerm.row; i++ ) {
		eeSAFE_FREE( mTerm.line[i] );
		eeSAFE_FREE( mTerm.alt[i] );
	}
	eeSAFE_FREE( mTerm.dirty );
	eeSAFE_FREE( mTerm.tabs );
	eeSAFE_FREE( mStrescseq.buf );

	clearHistory();

	{
		auto dpy = mDpy.lock();
		if ( dpy )
			dpy->detach( this );
	}
	mProcess->terminate();
}

void TerminalEmulator::logError( const char* msg ) {
	fprintf( stderr, "%s\n", msg );
}

void TerminalEmulator::terminate() {
	if ( mStatus == STARTING || mStatus == RUNNING ) {
		mProcess->terminate();
		mExitCode = 1;
		mStatus = TERMINATED;
	}
}

void TerminalEmulator::onProcessExit( int exitCode ) {
	auto dpy = mDpy.lock();
	if ( dpy )
		dpy->onProcessExit( exitCode );
}

void TerminalEmulator::onScrollPositionChange() {
	auto dpy = mDpy.lock();
	if ( dpy )
		dpy->onScrollPositionChange();
}

void TerminalEmulator::setClipboard( const char* str ) {
	auto dpy = mDpy.lock();
	if ( dpy )
		dpy->setClipboard( str );
}

void TerminalEmulator::loadColors() {
	auto dpy = mDpy.lock();
	if ( dpy )
		dpy->resetColors();
}

int TerminalEmulator::resetColor( int i, const char* name ) {
	auto dpy = mDpy.lock();
	if ( !dpy )
		return 0;

	return dpy->resetColor( i, name );
}

bool TerminalEmulator::isStarting() const {
	return mStatus == STARTING;
}

bool TerminalEmulator::isRunning() const {
	return mStatus == RUNNING;
}

bool TerminalEmulator::hasExited() const {
	return mStatus == TERMINATED;
}

int TerminalEmulator::getExitCode() const {
	return mExitCode;
}

int TerminalEmulator::getHistorySize() const {
	return mTerm.histi;
}

int TerminalEmulator::write( const char* buf, size_t buflen ) {
	return mPty->write( buf, (int)buflen );
}

void TerminalEmulator::resize( int columns, int rows ) {
	if ( !mPty->resize( columns, rows ) ) {
		_die( "Failed to resize pty!" );
		return;
	}
	tresize( columns, rows );
	redraw();
}

#define MAX_TTY_READS ( 1024 )

bool TerminalEmulator::update() {
	if ( mStatus == TerminalEmulator::STARTING ) {
		mStatus = TerminalEmulator::RUNNING;
	} else if ( mStatus != TerminalEmulator::RUNNING ) {
		if ( mDirty )
			draw();

		return true;
	}

	int read = MAX_TTY_READS;
	while ( ttyread() > 0 && --read )
		;

	if ( read != MAX_TTY_READS || mDirty )
		draw();

	mProcess->checkExitStatus();

	if ( mProcess->hasExited() ) {
		mExitCode = mProcess->getExitCode();
		mStatus = TERMINATED;
		onProcessExit( mExitCode );
	}

	return read != 0;
}

Term::~Term() {
	eeSAFE_FREE( line );
	eeSAFE_FREE( alt );
}

}} // namespace eterm::Terminal
