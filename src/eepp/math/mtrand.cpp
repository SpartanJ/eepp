#include <eepp/math/mtrand.hpp>

namespace EE { namespace Math {

MTRand::MTRand( const Uint32 oneSeed ) {
	seed( oneSeed );
}

MTRand::MTRand() {
	seed();
}

MTRand::MTRand( const MTRand& o ) {
	register const Uint32 *t	= o.mState;
	register Uint32	* s			= mState;
	register int	i			= N;

	for ( ; i--; *s++ = *t++ )
	{
	}

	mLeft = o.mLeft;

	mNext = &mState[ N - mLeft ];
}

void MTRand::initialize( const Uint32 seed ) {
	register Uint32 *s = mState;

	register Uint32 *r = mState;

	register Int32 i = 1;

	*s++ = seed & 0xffffffffUL;

	for ( ; i < N; ++i ) {
		*s++ = ( 1812433253UL * ( *r ^ (*r >> 30) ) + i ) & 0xffffffffUL;
		r++;
	}
}

void MTRand::reload() {
	static const int MmN = int(M) - int(N);

	register Uint32 *p = mState;

	register int i;

	for ( i = N - M; i--; ++p )
		*p = twist( p[M], p[0], p[1] );

	for ( i = M; --i; ++p )
		*p = twist( p[MmN], p[0], p[1] );

	*p = twist( p[MmN], p[0], mState[0] );

	mLeft = N, mNext = mState;
}

void MTRand::seed( const Uint32 oneSeed ) {
	initialize( oneSeed );
	reload();
}

void MTRand::seed() {
	seed( 0xFEDCBA09 );
}

Uint32 MTRand::hiBit( const Uint32 u ) const {
	return u & 0x80000000UL;
}

Uint32 MTRand::loBit( const Uint32 u ) const {
	return u & 0x00000001UL;
}

Uint32 MTRand::loBits( const Uint32 u ) const {
	return u & 0x7fffffffUL;
}

Uint32 MTRand::mixBits( const Uint32 u, const Uint32 v ) const {
	return hiBit(u) | loBits(v);
}

Uint32 MTRand::magic( const Uint32 u ) const {
	return loBit(u) ? 0x9908b0dfUL : 0x0UL;
}

Uint32 MTRand::twist( const Uint32 m, const Uint32 s0, const Uint32 s1 ) const {
	return m ^ ( mixBits( s0, s1 )>>1 ) ^ magic(s1);
}

Uint32 MTRand::randi() {
	if ( mLeft == 0 )
		reload();

	--mLeft;

	register Uint32 s1;
	s1 = *mNext++;
	s1 ^= (s1 >> 11);
	s1 ^= (s1 <<  7) & 0x9d2c5680UL;
	s1 ^= (s1 << 15) & 0xefc60000UL;

	return ( s1 ^ (s1 >> 18) );
}

Uint32 MTRand::randi( const Uint32 n ) {
	Uint32 used = n;
	used |= used >> 1;
	used |= used >> 2;
	used |= used >> 4;
	used |= used >> 8;
	used |= used >> 16;

	Uint32 i;

	do {
		i = randi() & used;
	} while ( i > n );

	return i;
}

double MTRand::rand() {
	return double( randi() ) * ( 1.0 / 4294967295.0 );
}

double MTRand::rand( const double n ) {
	return rand() * n;
}

Float	MTRand::randf() {
	return (Float)rand();
}

Float	MTRand::randf( const Float n ) {
	return (Float)rand(n);
}

int MTRand::randRange( int Min, int Max ) {
	return Min + randi( Max - Min );
}

Float	MTRand::randRange( Float Min, Float Max ) {
	return Min + randf( Max - Min );
}

MTRand& MTRand::operator=( const MTRand& o ) {
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

void MTRand::save( Uint32* saveArray ) const {
	register const Uint32 *s = mState;
	register Uint32 *sa = saveArray;
	register int i = N;
	for( ; i--; *sa++ = *s++ ) {}
	*sa = mLeft;
}

void MTRand::load( Uint32 *const loadArray ) {
	register Uint32 *s = mState;
	register Uint32 *la = loadArray;
	register int i = N;
	for( ; i--; *s++ = *la++ ) {}
	mLeft = *la;
	mNext = &mState[N-mLeft];
}

}}
