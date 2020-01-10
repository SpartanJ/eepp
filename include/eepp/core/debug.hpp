#ifndef EE_DEBUG_HPP
#define EE_DEBUG_HPP

#include <eepp/config.hpp>

namespace EE {

extern EE_API bool PrintDebugInLog;

#ifndef EE_SILENT
void EE_API eePRINT( const char* format, ... );
void EE_API eePRINTL( const char* format, ... );
void EE_API eePRINTC( unsigned int cond, const char* format, ... );
#else
#ifdef EE_COMPILER_GCC
#define eePRINT( format, args... ) \
	{}
#define eePRINTL( format, args... ) \
	{}
#define eePRINTC( cond, format, args... ) \
	{}
#else
#define eePRINT
#define eePRINTL
#define eePRINTC
#endif
#endif

#ifdef EE_DEBUG
void EE_API eeREPORT_ASSERT( const char* File, const int Line, const char* Exp );
#define eeASSERT( expr )                              \
	if ( !( expr ) ) {                                \
		eeREPORT_ASSERT( __FILE__, __LINE__, #expr ); \
	}
#define eeASSERTM( expr, msg )                       \
	if ( !( expr ) ) {                               \
		eeREPORT_ASSERT( __FILE__, __LINE__, #msg ); \
	}
#else
#define eeASSERT( expr )
#define eeASSERTM( expr, msg )
#endif

} // namespace EE

#endif
