#pragma once

/**
 * @file retainsymbol.h
 * @brief Provides a cross-platform macro to force a symbol to be retained by the linker.
 *
 * This is useful when building static libraries that are later linked into a dynamic
 * library or executable. The linker may discard unreferenced symbols, but this macro
 * ensures the specified symbol is included in the final binary.
 */

// --- Helper macros to create unique variable names ---
#define _RETAIN_SYMBOL_CONCAT_INNER( a, b ) a##b
#define _RETAIN_SYMBOL_CONCAT( a, b ) _RETAIN_SYMBOL_CONCAT_INNER( a, b )

#ifdef __COUNTER__
#define _RETAIN_SYMBOL_UNIQUE_NAME( base ) _RETAIN_SYMBOL_CONCAT( base, __COUNTER__ )
#else
#define _RETAIN_SYMBOL_UNIQUE_NAME( base ) _RETAIN_SYMBOL_CONCAT( base, __LINE__ )
#endif

/**
 * @def RETAIN_SYMBOL(symbol)
 * @brief Registers a symbol to be retained by the linker.
 *
 * @param symbol The name of the function or variable to retain. Do not pass it as a string.
 *
 * Usage:
 *   RETAIN_SYMBOL(MyFunctionToKeep); // At file scope
 */
#if defined( __GNUC__ ) || defined( __clang__ )
// For GCC and Clang, the 'used' attribute is sufficient to prevent the compiler
// from optimizing away the symbol.
// For Apple platforms (macOS, iOS), the 'retain' attribute is also necessary
// to place the symbol in a special section that prevents the static linker
// from removing it.
#if defined( __APPLE__ )
#define _RETAIN_SYMBOL_ATTRIBUTE __attribute__( ( used, retain ) )
#else
#define _RETAIN_SYMBOL_ATTRIBUTE __attribute__( ( used ) )
#endif

#define RETAIN_SYMBOL( symbol ) \
	_RETAIN_SYMBOL_ATTRIBUTE    \
	static void* const _RETAIN_SYMBOL_UNIQUE_NAME( g_retained_symbol_ptr_ ) = (void*)&( symbol )

#elif defined( _MSC_VER )
// For MSVC, the /include:symbol linker option is the most direct way.
#define RETAIN_SYMBOL( symbol )                                                 \
	_Pragma( "comment(linker, \"/include:\"" #symbol "\")" ) static void* const \
	_RETAIN_SYMBOL_UNIQUE_NAME( g_retained_symbol_ptr_ ) = (void*)&( symbol )

#else
// Fallback for other compilers. 'volatile' may discourage optimization but is not a guarantee.
#define RETAIN_SYMBOL( symbol )                                                        \
	static void* volatile const _RETAIN_SYMBOL_UNIQUE_NAME( g_retained_symbol_ptr_ ) = \
		(void*)&( symbol )

#endif
