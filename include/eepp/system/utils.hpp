#ifndef EE_SYSTEMCUTILS_H
#define EE_SYSTEMCUTILS_H

#include <eepp/declares.hpp>

namespace EE { namespace System {

/** Write a bit into the Key in the position defined.
* @param Key The Key to write
* @param Pos The Position of the bit
* @param BitWrite 0 for write 0, any other to write 1.
*/
inline void Write32BitKey( unsigned int * Key, unsigned int Pos, unsigned int BitWrite ) {
	( BitWrite ) ? ( * Key ) |= ( 1 << Pos ) : ( * Key ) &= ~( 1 << Pos );
}

/** Read a bit from a 32 bit key, in the position defined
* @param Key The Key to read
* @param Pos The Position in the key to read
* @return True if the bit is 1
*/
inline bool Read32BitKey( Uint32 * Key, Uint32 Pos ) {
	return 0 != ( ( * Key ) & ( 1 << Pos ) );
}

/** Write a 32 bit flag value */
inline void SetFlagValue( Uint32 * Key, Uint32 Val, Uint32 BitWrite ) {
	( BitWrite ) ? ( * Key ) |= Val : ( * Key ) &= ~Val;
}

}

}
#endif
