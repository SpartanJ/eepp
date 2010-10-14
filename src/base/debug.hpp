#ifndef EE_DEBUG_HPP
#define EE_DEBUG_HPP

#include "../base.hpp"

namespace EE {

#ifdef EE_DEBUG

void eeREPORT_ASSERT( const char * File, const int Line, const char * Exp );

#define eeASSERT( expr )		if ( !(expr) ) { eeREPORT_ASSERT( __FILE__, __LINE__, #expr	);	}
#define eeASSERTM( expr, msg )	if ( !(expr) ) { eeREPORT_ASSERT( __FILE__, __LINE__, #msg	);	}

void eePRINT 	( const char * format, ... );
void eePRINTC	( unsigned int cond, const char * format, ... );

#else

#define eeASSERT( expr )
#define eeASSERTM( expr, msg )

#ifdef EE_COMPILER_GCC
#define eePRINT( format, args... ) {}
#define eePRINTC( cond, format, args... ) {}
#else
#define eePRINT
#define eePRINTC
#endif

#endif

}

#endif
