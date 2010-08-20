// MersenneTwister.h
// Mersenne Twister random number generator -- a C++ class MTRand
// Based on code by Makoto Matsumoto, Takuji Nishimura, and Shawn Cokus
// Richard J. Wagner  v1.1  28 September 2009  wagnerr@umich.edu

// The Mersenne Twister is an algorithm for generating random numbers.  It
// was designed with consideration of the flaws in various other generators.
// The period, 2^19937-1, and the order of equidistribution, 623 dimensions,
// are far greater.  The generator is also fast; it avoids multiplication and
// division, and it benefits from caches and pipelines.  For more information
// see the inventors' web page at
// http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html

// Reference
// M. Matsumoto and T. Nishimura, "Mersenne Twister: A 623-Dimensionally
// Equidistributed Uniform Pseudo-Random Number Generator", ACM Transactions on
// Modeling and Computer Simulation, Vol. 8, No. 1, January 1998, pp 3-30.

// Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
// Copyright (C) 2000 - 2009, Richard J. Wagner
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//   3. The names of its contributors may not be used to endorse or promote
//      products derived from this software without specific prior written
//      permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// The original code included the following notice:
//
//     When you use this, send an email to: m-mat@math.sci.hiroshima-u.ac.jp
//     with an appropriate reference to your work.
//
// It would be nice to CC: wagnerr@umich.edu and Cokus@math.washington.edu
// when you write.

// Adapted to EE++ ( this is not the original file ).
// Added RandRange.

#ifndef EE_UTILSCMTRAND_HPP
#define EE_UTILSCMTRAND_HPP

#include "base.hpp"

namespace EE { namespace Math {

class EE_API MTRand {
	public:
		static const Uint32 M		= 397;
		static const Int32 N		= 624;
		static const Uint32 SAVE	= N + 1;

		MTRand( const Uint32 oneSeed );

		/** Initialize with a predefined seed */
		MTRand();

		MTRand( const MTRand& o );

		MTRand& operator=( const MTRand& o );

		/** @return integer in [0,2^32-1] */
		Uint32				Randi();

		/** @return integer in [0,n] for n < 2^32 */
		Uint32				Randi( const Uint32 n );

		/** @return real number in [0,1] */
		eeDouble			Rand();

		/** @return real number in [0,n] */
		eeDouble			Rand( const eeDouble n );

		/** Set a new seed */
		void				Seed( const Uint32 oneSeed );

		/** Set the default seed */
		void				Seed();

		/** @return float number in [0,1] */
		eeFloat				Randf();

		/** @return float number in [0,n] */
		eeFloat				Randf( const eeFloat n );

		/** @return int number in [Min,Max] */
		eeInt				RandRange( eeInt Min, eeInt Max );

		/** @return float number in [Min,Max] */
		eeFloat				RandRange( eeFloat Min, eeFloat Max );

		/** Save the state to an allocated array */
		void 				Save( Uint32* saveArray ) const;

		/** Load state from an array */
		void				Load( Uint32 *const loadArray );
	protected:
		Uint32				mState[N];
		Uint32 *			mNext;
		eeInt				mLeft;

		void				Initialize( const Uint32 oneSeed );

		void				Reload();

		Uint32				hiBit( const Uint32 u ) const;

		Uint32				loBit( const Uint32 u ) const;

		Uint32				loBits( const Uint32 u ) const;

		Uint32				MixBits( const Uint32 u, const Uint32 v ) const;

		Uint32				Magic( const Uint32 u ) const;

		Uint32				Twist( const Uint32 m, const Uint32 s0, const Uint32 s1 ) const;
};

}}

#endif
