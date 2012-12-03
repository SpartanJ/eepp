#ifndef EE_SYSTEMCUTILS_H
#define EE_SYSTEMCUTILS_H

#include <eepp/declares.hpp>

namespace EE { namespace System {

class BitOp {
	public:

	/** Write a bit into the Key in the position defined.
	* @param Key The Key to write
	* @param Pos The Position of the bit
	* @param BitWrite 0 for write 0, any other to write 1.
	*/
	static inline void WriteBitKey( unsigned int * Key, unsigned int Pos, unsigned int BitWrite ) {
		( BitWrite ) ? ( * Key ) |= ( 1 << Pos ) : ( * Key ) &= ~( 1 << Pos );
	}

	/** Read a bit from a 32 bit key, in the position defined
	* @param Key The Key to read
	* @param Pos The Position in the key to read
	* @return True if the bit is 1
	*/
	static inline bool ReadBitKey( Uint32 * Key, Uint32 Pos ) {
		return 0 != ( ( * Key ) & ( 1 << Pos ) );
	}

	/** Write a bit flag value
	* @param Key The Key to write or remove
	* @param Val The Value to write or remove
	* @param BitWrite 0 to remove, any value to write
	*/
	static inline void SetBitFlagValue( Uint32 * Key, Uint32 Val, Uint32 BitWrite ) {
		( BitWrite ) ? ( * Key ) |= Val : ( * Key ) &= ~Val;
	}
};

}

}
#endif
