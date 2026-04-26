#pragma once
#include "utest.h"

#include <sstream>
#include <vector>

template <typename T> std::string vectorToString( const std::vector<T>& vec ) {
	std::ostringstream oss;
	oss << "[";
	bool first = true;
	for ( const auto& element : vec ) {
		if ( !first )
			oss << ", ";
		oss << element;
		first = false;
	}
	oss << "]";
	return oss.str();
}

#define UTEST_STDSTREQ( x, y, msg, is_assert )                      \
	UTEST_SURPRESS_WARNING_BEGIN do {                               \
		const std::string xEval = ( x );                            \
		const std::string yEval = ( y );                            \
		if ( xEval != yEval ) {                                     \
			UTEST_PRINTF( "%s:%i: Failure\n", __FILE__, __LINE__ ); \
			UTEST_PRINTF( "  Expected : \"%s\"\n", xEval.c_str() ); \
			UTEST_PRINTF( "    Actual : \"%s\"\n", yEval.c_str() ); \
			if ( strlen( msg ) > 0 ) {                              \
				UTEST_PRINTF( "   Message : %s\n", msg );           \
			}                                                       \
			*utest_result = UTEST_TEST_FAILURE;                     \
			if ( is_assert ) {                                      \
				return;                                             \
			}                                                       \
		}                                                           \
	}                                                               \
	while ( 0 )                                                     \
	UTEST_SURPRESS_WARNING_END

#define EXPECT_STDSTREQ( x, y ) UTEST_STDSTREQ( x, y, "", 0 )
#define EXPECT_STDSTREQ_MSG( x, y, msg ) UTEST_STDSTREQ( x, y, msg, 0 )
#define ASSERT_STDSTREQ( x, y ) UTEST_STDSTREQ( x, y, "", 1 )
#define ASSERT_STDSTREQ_MSG( x, y, msg ) UTEST_STDSTREQ( x, y, msg, 1 )

#define UTEST_STRINGEQ( x, y, msg, is_assert )                               \
	UTEST_SURPRESS_WARNING_BEGIN do {                                        \
		const String xEval = ( x );                                          \
		const String yEval = ( y );                                          \
		if ( xEval != yEval ) {                                              \
			UTEST_PRINTF( "%s:%i: Failure\n", __FILE__, __LINE__ );          \
			UTEST_PRINTF( "  Expected : \"%s\"\n", xEval.toUtf8().c_str() ); \
			UTEST_PRINTF( "    Actual : \"%s\"\n", yEval.toUtf8().c_str() ); \
			if ( strlen( msg ) > 0 ) {                                       \
				UTEST_PRINTF( "   Message : %s\n", msg );                    \
			}                                                                \
			*utest_result = UTEST_TEST_FAILURE;                              \
			if ( is_assert ) {                                               \
				return;                                                      \
			}                                                                \
		}                                                                    \
	}                                                                        \
	while ( 0 )                                                              \
	UTEST_SURPRESS_WARNING_END

#define EXPECT_STRINGEQ( x, y ) UTEST_STRINGEQ( x, y, "", 0 )
#define EXPECT_STRINGEQ_MSG( x, y, msg ) UTEST_STRINGEQ( x, y, msg, 0 )
#define ASSERT_STRINGEQ( x, y ) UTEST_STRINGEQ( x, y, "", 1 )
#define ASSERT_STRINGEQ_MSG( x, y, msg ) UTEST_STRINGEQ( x, y, msg, 1 )

#define UTEST_PRINT_INFO( str ) UTEST_PRINTF( "\033[32m[   INFO   ]\033[0m \t%s\n", str )
#define UTEST_PRINT_STEP( str ) UTEST_PRINTF( "\033[32m[ SUB-STEP ]\033[0m \t%s\n", str )

#define UTEST_VECTOREQ( xEval, yEval, msg, is_assert )                                \
	UTEST_SURPRESS_WARNING_BEGIN do {                                                 \
		if ( xEval != yEval ) {                                                       \
			UTEST_PRINTF( "%s:%i: Failure\n", __FILE__, __LINE__ );                   \
			UTEST_PRINTF( "  Expected : \"%s\"\n", vectorToString( xEval ).c_str() ); \
			UTEST_PRINTF( "    Actual : \"%s\"\n", vectorToString( yEval ).c_str() ); \
			if ( strlen( msg ) > 0 ) {                                                \
				UTEST_PRINTF( "   Message : %s\n", msg );                             \
			}                                                                         \
			*utest_result = UTEST_TEST_FAILURE;                                       \
			if ( is_assert ) {                                                        \
				return;                                                               \
			}                                                                         \
		}                                                                             \
	}                                                                                 \
	while ( 0 )                                                                       \
	UTEST_SURPRESS_WARNING_END

#define EXPECT_VECTOREQ( x, y ) UTEST_VECTOREQ( x, y, "", 0 )
#define EXPECT_VECTOREQ_MSG( x, y, msg ) UTEST_VECTOREQ( x, y, msg, 0 )
#define ASSERT_VECTOREQ( x, y ) UTEST_VECTOREQ( x, y, "", 1 )
#define ASSERT_VECTOREQ_MSG( x, y, msg ) UTEST_VECTOREQ( x, y, msg, 1 )
