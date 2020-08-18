#include "lua-str.hpp"
#include <cctype>
#include <cstdarg>
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Taken from rx-cpp (https://github.com/stevedonovan/rx-cpp/) that took this code from the
// Lua source code.

static LuaFailFun s_fail_fun;

void lua_str_fail_func( LuaFailFun f ) {
	s_fail_fun = f;
}

/* macro to `unsign' a character */
#define uchar( c ) ( (unsigned char)( c ) )

/*
** {======================================================
** PATTERN MATCHING
** =======================================================
*/

#define LUA_MAXCAPTURES 32

/* maximum recursion depth for 'match' */
#define MAXCCALLS 200

#define CAP_UNFINISHED ( -1 )
#define CAP_POSITION ( -2 )

typedef struct MatchState {
	int matchdepth;		  /* control for recursive depth (to avoid C stack overflow) */
	const char* src_init; /* init of source string */
	const char* src_end;  /* end ('\0') of source string */
	const char* p_end;	  /* end ('\0') of pattern */
	int level;			  /* total number of captures (finished or unfinished) */
	struct {
		const char* init;
		ptrdiff_t len;
	} capture[LUA_MAXCAPTURES];
} MatchState;

/* recursive function */
static const char* match( MatchState* ms, const char* s, const char* p );

#define L_ESC '%'
#define SPECIALS "^$*+?.([%-"

// error handling, hm?? NB

static int throw_error( const char* fmt, ... ) {
	char buff[1024];
	va_list ap;
	va_start( ap, fmt );
	vsnprintf( buff, sizeof( buff ), fmt, ap );
	va_end( ap );
	if ( !s_fail_fun ) {
		fprintf( stderr, "%s\n", buff );
		exit( 1 );
	} else {
		s_fail_fun( buff );
	}
	return 0;
}

static int check_capture( MatchState* ms, int l ) {
	l -= '1';
	if ( l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED )
		return throw_error( "invalid capture index %%%d", l + 1 );
	return l;
}

static int capture_to_close( MatchState* ms ) {
	int level = ms->level;
	for ( level--; level >= 0; level-- )
		if ( ms->capture[level].len == CAP_UNFINISHED )
			return level;
	return throw_error( "invalid pattern capture" );
}

static const char* classend( MatchState* ms, const char* p ) {
	switch ( *p++ ) {
		case L_ESC: {
			if ( p == ms->p_end )
				throw_error( "malformed pattern (ends with '%')" );
			return p + 1;
		}
		case '[': {
			if ( *p == '^' )
				p++;
			do { /* look for a `]' */
				if ( p == ms->p_end )
					throw_error( "malformed pattern (missing ']')" );
				if ( *( p++ ) == L_ESC && p < ms->p_end )
					p++; /* skip escapes (e.g. `%]') */
			} while ( *p != ']' );
			return p + 1;
		}
		default: {
			return p;
		}
	}
}

static int match_class( int c, int cl ) {
	int res;
	switch ( tolower( cl ) ) {
		case 'a':
			res = isalpha( c );
			break;
		case 'c':
			res = iscntrl( c );
			break;
		case 'd':
			res = isdigit( c );
			break;
		case 'g':
			res = isgraph( c );
			break;
		case 'l':
			res = islower( c );
			break;
		case 'p':
			res = ispunct( c );
			break;
		case 's':
			res = isspace( c );
			break;
		case 'u':
			res = isupper( c );
			break;
		case 'w':
			res = isalnum( c );
			break;
		case 'x':
			res = isxdigit( c );
			break;
		case 'z':
			res = ( c == 0 );
			break; /* deprecated option */
		default:
			return ( cl == c );
	}
	return ( islower( cl ) ? res : !res );
}

static int matchbracketclass( int c, const char* p, const char* ec ) {
	int sig = 1;
	if ( *( p + 1 ) == '^' ) {
		sig = 0;
		p++; /* skip the `^' */
	}
	while ( ++p < ec ) {
		if ( *p == L_ESC ) {
			p++;
			if ( match_class( c, uchar( *p ) ) )
				return sig;
		} else if ( ( *( p + 1 ) == '-' ) && ( p + 2 < ec ) ) {
			p += 2;
			if ( uchar( *( p - 2 ) ) <= c && c <= uchar( *p ) )
				return sig;
		} else if ( uchar( *p ) == c )
			return sig;
	}
	return !sig;
}

static int singlematch( MatchState* ms, const char* s, const char* p, const char* ep ) {
	if ( s >= ms->src_end )
		return 0;
	else {
		int c = uchar( *s );
		switch ( *p ) {
			case '.':
				return 1; /* matches any char */
			case L_ESC:
				return match_class( c, uchar( *( p + 1 ) ) );
			case '[':
				return matchbracketclass( c, p, ep - 1 );
			default:
				return ( uchar( *p ) == c );
		}
	}
}

static const char* matchbalance( MatchState* ms, const char* s, const char* p ) {
	if ( p >= ms->p_end - 1 )
		throw_error( "malformed pattern "
					 "(missing arguments to  '%b')" );
	if ( *s != *p )
		return NULL;
	else {
		int b = *p;
		int e = *( p + 1 );
		int cont = 1;
		while ( ++s < ms->src_end ) {
			if ( *s == e ) {
				if ( --cont == 0 )
					return s + 1;
			} else if ( *s == b )
				cont++;
		}
	}
	return NULL; /* string ends out of balance */
}

static const char* max_expand( MatchState* ms, const char* s, const char* p, const char* ep ) {
	ptrdiff_t i = 0; /* counts maximum expand for item */
	while ( singlematch( ms, s + i, p, ep ) )
		i++;
	/* keeps trying to match with the maximum repetitions */
	while ( i >= 0 ) {
		const char* res = match( ms, ( s + i ), ep + 1 );
		if ( res )
			return res;
		i--; /* else didn't match; reduce 1 repetition to try again */
	}
	return NULL;
}

static const char* min_expand( MatchState* ms, const char* s, const char* p, const char* ep ) {
	for ( ;; ) {
		const char* res = match( ms, s, ep + 1 );
		if ( res != NULL )
			return res;
		else if ( singlematch( ms, s, p, ep ) )
			s++; /* try with one more repetition */
		else
			return NULL;
	}
}

static const char* start_capture( MatchState* ms, const char* s, const char* p, int what ) {
	const char* res;
	int level = ms->level;
	if ( level >= LUA_MAXCAPTURES )
		throw_error( "too many captures" );
	ms->capture[level].init = s;
	ms->capture[level].len = what;
	ms->level = level + 1;
	if ( ( res = match( ms, s, p ) ) == NULL ) /* match failed? */
		ms->level--;						   /* undo capture */
	return res;
}

static const char* end_capture( MatchState* ms, const char* s, const char* p ) {
	int l = capture_to_close( ms );
	const char* res;
	ms->capture[l].len = s - ms->capture[l].init; /* close capture */
	if ( ( res = match( ms, s, p ) ) == NULL )	  /* match failed? */
		ms->capture[l].len = CAP_UNFINISHED;	  /* undo capture */
	return res;
}

static const char* match_capture( MatchState* ms, const char* s, int l ) {
	size_t len;
	l = check_capture( ms, l );
	len = ms->capture[l].len;
	if ( ( size_t )( ms->src_end - s ) >= len && memcmp( ms->capture[l].init, s, len ) == 0 )
		return s + len;
	else
		return NULL;
}

static const char* match( MatchState* ms, const char* s, const char* p ) {
	if ( ms->matchdepth-- == 0 )
		throw_error( "pattern too complex" );
init:						/* using goto's to optimize tail recursion */
	if ( p != ms->p_end ) { /* end of pattern? */
		switch ( *p ) {
			case '(': {					 /* start capture */
				if ( *( p + 1 ) == ')' ) /* position capture? */
					s = start_capture( ms, s, p + 2, CAP_POSITION );
				else
					s = start_capture( ms, s, p + 1, CAP_UNFINISHED );
				break;
			}
			case ')': { /* end capture */
				s = end_capture( ms, s, p + 1 );
				break;
			}
			case '$': {
				if ( ( p + 1 ) != ms->p_end )		 /* is the `$' the last char in pattern? */
					goto dflt;						 /* no; go to default */
				s = ( s == ms->src_end ) ? s : NULL; /* check end of string */
				break;
			}
			case L_ESC: { /* escaped sequences not in the format class[*+?-]? */
				switch ( *( p + 1 ) ) {
					case 'b': { /* balanced string? */
						s = matchbalance( ms, s, p + 2 );
						if ( s != NULL ) {
							p += 4;
							goto init; /* return match(ms, s, p + 4); */
						}			   /* else fail (s == NULL) */
						break;
					}
					case 'f': { /* frontier? */
						const char* ep;
						char previous;
						p += 2;
						if ( *p != '[' )
							throw_error( "missing '[' after '%f' in pattern" );
						ep = classend( ms, p ); /* points to what is next */
						previous = ( s == ms->src_init ) ? '\0' : *( s - 1 );
						if ( !matchbracketclass( uchar( previous ), p, ep - 1 ) &&
							 matchbracketclass( uchar( *s ), p, ep - 1 ) ) {
							p = ep;
							goto init; /* return match(ms, s, ep); */
						}
						s = NULL; /* match failed */
						break;
					}
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9': { /* capture results (%0-%9)? */
						s = match_capture( ms, s, uchar( *( p + 1 ) ) );
						if ( s != NULL ) {
							p += 2;
							goto init; /* return match(ms, s, p + 2) */
						}
						break;
					}
					default:
						goto dflt;
				}
				break;
			}
			default:
			dflt : {								/* pattern class plus optional suffix */
				const char* ep = classend( ms, p ); /* points to optional suffix */
				/* does not match at least once? */
				if ( !singlematch( ms, s, p, ep ) ) {
					if ( *ep == '*' || *ep == '?' || *ep == '-' ) { /* accept empty? */
						p = ep + 1;
						goto init;	 /* return match(ms, s, ep + 1); */
					} else			 /* '+' or no suffix */
						s = NULL;	 /* fail */
				} else {			 /* matched once */
					switch ( *ep ) { /* handle optional suffix */
						case '?': {	 /* optional */
							const char* res;
							if ( ( res = match( ms, s + 1, ep + 1 ) ) != NULL )
								s = res;
							else {
								p = ep + 1;
								goto init; /* else return match(ms, s, ep + 1); */
							}
							break;
						}
						case '+': /* 1 or more repetitions */
							s++;  /* 1 match already done */
								  /* go through */
						case '*': /* 0 or more repetitions */
							s = max_expand( ms, s, p, ep );
							break;
						case '-': /* 0 or more repetitions (minimum) */
							s = min_expand( ms, s, p, ep );
							break;
						default: /* no suffix */
							s++;
							p = ep;
							goto init; /* return match(ms, s + 1, ep); */
					}
				}
				break;
			}
		}
	}
	ms->matchdepth++;
	return s;
}

static void push_onecapture( MatchState* ms, int i, const char* s, const char* e, LuaMatch* mm ) {
	if ( i >= ms->level ) {
		if ( i == 0 ) { /* ms->level == 0, too */
			mm->start = 0;
			mm->end = e - s;
			// lua_pushlstring(ms->L, s, e - s);  /* add whole match */
		} else
			throw_error( "invalid capture index" );
	} else {
		ptrdiff_t l = ms->capture[i].len;
		if ( l == CAP_UNFINISHED )
			throw_error( "unfinished capture" );
		if ( l == CAP_POSITION ) {
			mm[i].start = ms->capture[i].init - ms->src_init + 1;
			mm[i].end = mm[i].start;
		} else {
			mm[i].start = ms->capture[i].init - ms->src_init;
			mm[i].end = mm[i].start + l;
		}
	}
}

static int push_captures( MatchState* ms, const char* s, const char* e, LuaMatch* mm ) {
	int i;
	int nlevels = ( ms->level == 0 && s ) ? 1 : ms->level;
	for ( i = 0; i < nlevels; i++ )
		push_onecapture( ms, i, s, e, mm );
	return nlevels; /* number of strings pushed */
}

int lua_str_match( const char* s, int offset, size_t ls, const char* p, LuaMatch* mm ) {
	size_t lp = strlen( p );
	const char* s1 = s + offset;
	MatchState ms;
	int anchor = ( *p == '^' );
	if ( anchor ) {
		p++;
		lp--; /* skip anchor character */
	}
	ms.matchdepth = MAXCCALLS;
	ms.src_init = s;
	ms.src_end = s + ls;
	ms.p_end = p + lp;
	do {
		const char* res;
		ms.level = 0;
		if ( ( res = match( &ms, s1, p ) ) != NULL ) {
			mm[0].start = s1 - s; /* start */
			mm[0].end = res - s;  /* end */
			return push_captures( &ms, NULL, 0, mm + 1 ) + 1;
		}
	} while ( s1++ < ms.src_end && !anchor );
	return 0;
}
