#ifndef EE_SYSTEMCUTILS_H
#define EE_SYSTEMCUTILS_H

#include <eepp/declares.hpp>
#include <string>

namespace EE { namespace System {

/** @return string hash */
Uint32 EE_API MakeHash( const std::string& str );

/** @return string hash */
Uint32 EE_API MakeHash( const Uint8 * str );

/** Write a bit into the Key in the position defined.
* @param Key The Key to write
* @param Pos The Position of the bit
* @param BitWrite 0 for write 0, any other to write 1.
*/
void EE_API Write32BitKey( Uint32 * Key, Uint32 Pos, Uint32 BitWrite );

/** Read a bit from a 32 bit key, in the position defined
* @param Key The Key to read
* @param Pos The Position in the key to read
* @return True if the bit is 1
*/
bool EE_API Read32BitKey( Uint32 * Key, Uint32 Pos );

/** Write a 32 bit flag value */
void EE_API SetFlagValue( Uint32 * Key, Uint32 Val, Uint32 BitWrite );

}

}
#endif
