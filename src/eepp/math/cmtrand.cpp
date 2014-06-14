#include <eepp/math/cmtrand.hpp>

namespace EE { namespace Math {

cMTRand::cMTRand( const Uint32 oneSeed ) {
    Seed( oneSeed );
}

cMTRand::cMTRand() {
    Seed();
}

cMTRand::cMTRand( const cMTRand& o ) {
    register const Uint32 *t	= o.mState;
	register Uint32	* s			= mState;
    register int	i			= N;

    for ( ; i--; *s++ = *t++ )
	{
	}

    mLeft = o.mLeft;

    mNext = &mState[ N - mLeft ];
}

void cMTRand::Initialize( const Uint32 seed ) {
    register Uint32 *s = mState;

    register Uint32 *r = mState;

    register Int32 i = 1;

    *s++ = seed & 0xffffffffUL;

    for ( ; i < N; ++i ) {
        *s++ = ( 1812433253UL * ( *r ^ (*r >> 30) ) + i ) & 0xffffffffUL;
        r++;
    }
}

void cMTRand::Reload() {
    static const int MmN = int(M) - int(N);

    register Uint32 *p = mState;

    register int i;

    for ( i = N - M; i--; ++p )
        *p = Twist( p[M], p[0], p[1] );

    for ( i = M; --i; ++p )
        *p = Twist( p[MmN], p[0], p[1] );

    *p = Twist( p[MmN], p[0], mState[0] );

    mLeft = N, mNext = mState;
}

void cMTRand::Seed( const Uint32 oneSeed ) {
    Initialize( oneSeed );
    Reload();
}

void cMTRand::Seed() {
    Seed( 0xFEDCBA09 );
}

Uint32 cMTRand::hiBit( const Uint32 u ) const {
	return u & 0x80000000UL;
}

Uint32 cMTRand::loBit( const Uint32 u ) const {
	return u & 0x00000001UL;
}

Uint32 cMTRand::loBits( const Uint32 u ) const {
	return u & 0x7fffffffUL;
}

Uint32 cMTRand::MixBits( const Uint32 u, const Uint32 v ) const {
	return hiBit(u) | loBits(v);
}

Uint32 cMTRand::Magic( const Uint32 u ) const {
	return loBit(u) ? 0x9908b0dfUL : 0x0UL;
}

Uint32 cMTRand::Twist( const Uint32 m, const Uint32 s0, const Uint32 s1 ) const {
	return m ^ ( MixBits( s0, s1 )>>1 ) ^ Magic(s1);
}

Uint32 cMTRand::Randi() {
    if ( mLeft == 0 )
		Reload();

    --mLeft;

    register Uint32 s1;
    s1 = *mNext++;
    s1 ^= (s1 >> 11);
    s1 ^= (s1 <<  7) & 0x9d2c5680UL;
    s1 ^= (s1 << 15) & 0xefc60000UL;

    return ( s1 ^ (s1 >> 18) );
}

Uint32 cMTRand::Randi( const Uint32 n ) {
    Uint32 used = n;
    used |= used >> 1;
    used |= used >> 2;
    used |= used >> 4;
    used |= used >> 8;
    used |= used >> 16;

    Uint32 i;

    do {
        i = Randi() & used;
	} while ( i > n );

    return i;
}

double cMTRand::Rand() {
    return double( Randi() ) * ( 1.0 / 4294967295.0 );
}

double cMTRand::Rand( const double n ) {
    return Rand() * n;
}

Float	cMTRand::Randf() {
	return (Float)Rand();
}

Float	cMTRand::Randf( const Float n ) {
	return (Float)Rand(n);
}

int cMTRand::RandRange( int Min, int Max ) {
	return Min + Randi( Max - Min );
}

Float	cMTRand::RandRange( Float Min, Float Max ) {
	return Min + Randf( Max - Min );
}

cMTRand& cMTRand::operator=( const cMTRand& o ) {
	if ( this == &o )
		return (*this);

    register const Uint32 *t	= o.mState;
    register Uint32 *s			= mState;
    register int i			= N;

    for ( ; i--; *s++ = *t++ )
	{
	}

    mLeft		= o.mLeft;
    mNext		= &mState[ N - mLeft ];

    return (*this);
}

void cMTRand::Save( Uint32* saveArray ) const {
	register const Uint32 *s = mState;
	register Uint32 *sa = saveArray;
	register int i = N;
	for( ; i--; *sa++ = *s++ ) {}
	*sa = mLeft;
}

void cMTRand::Load( Uint32 *const loadArray ) {
	register Uint32 *s = mState;
	register Uint32 *la = loadArray;
	register int i = N;
	for( ; i--; *s++ = *la++ ) {}
	mLeft = *la;
	mNext = &mState[N-mLeft];
}

}}
