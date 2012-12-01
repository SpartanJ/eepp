#include <eepp/system/utils.hpp>

namespace EE { namespace System {

Uint32 MakeHash( const std::string& str ) {
	return MakeHash( reinterpret_cast<const Uint8*>( &str[0] ) );
}

Uint32 MakeHash( const Uint8 * str ) {
	//! djb2
	if ( NULL != str ) {
		Uint32 hash = 5381;
		Int32 c;

		while ( ( c = *str++ ) )
			hash = ( ( hash << 5 ) + hash ) + c;

		return hash;
	}

	return 0;
}

void Write32BitKey( Uint32 * Key, Uint32 Pos, Uint32 BitWrite ) {
	if ( BitWrite )
		( * Key ) |= ( 1 << Pos );
	else {
		if ( ( * Key ) & ( 1 << Pos ) )
			( * Key ) &= ~( 1 << Pos );
	}
}

bool Read32BitKey( Uint32 * Key, Uint32 Pos ) {
	return 0 != ( ( * Key ) & ( 1 << Pos ) );
}

void SetFlagValue( Uint32 * Key, Uint32 Val, Uint32 BitWrite ) {
	if ( BitWrite )
		( * Key ) |= Val;
	else {
		if ( ( * Key ) & Val )
			( * Key ) &= ~Val;
	}
}

}}
