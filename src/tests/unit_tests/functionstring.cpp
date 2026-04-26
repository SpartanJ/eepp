#include "utest.hpp"
#include <eepp/system/functionstring.hpp>

using namespace std::literals;

using namespace EE;
using namespace EE::System;

UTEST( FunctionString, functionString ) {
	const auto testCase = [&]( const std::string& in, const std::string& funcName,
							   const std::vector<std::string>& parameters ) {
		UTEST_PRINT_STEP( funcName.c_str() );
		auto ret = FunctionString::parse( in );
		EXPECT_STDSTREQ( funcName, ret.getName() );
		for ( size_t i = 0; i < parameters.size(); i++ ) {
			if ( i < ret.getParameters().size() )
				EXPECT_STDSTREQ( parameters[i], ret.getParameters()[i] );
			else
				EXPECT_STDSTREQ( parameters[i], "" );
		}
		EXPECT_EQ( parameters.size(), ret.getParameters().size() );
	};

	testCase( R"(test1(12, 14))", "test1", { "12", "14" } );
	testCase( R"(test2("string", "string2"))", "test2", { "string", "string2" } );
	testCase( R"(test3 (16, 14, 18))", "test3", { "16", "14", "18" } );
	testCase( R"(test4(2.0, 14))", "test4", { "2.0", "14" } );
	testCase( R"(test5("str ing", "stri ng2"))", "test5", { "str ing", "stri ng2" } );
	testCase( R"(test6("str\"ing", "stri\"ng2"))", "test6", { "str\"ing", "stri\"ng2" } );
	testCase( R"(test7("str,ing", "stri,ng2"))", "test7", { "str,ing", "stri,ng2" } );
	testCase( R"(test8("str\\ing", "stri\\ng2"))", "test8", { "str\\\\ing", "stri\\\\ng2" } );
	testCase( R"(test9("12"   ,  "14"   ))", "test9", { "12", "14" } );
	testCase( R"(test10(12    ,    14   ))", "test10", { "12", "14" } );
	testCase( R"(test11( "12   " ,  "  14  " ))", "test11", { "12   ", "  14  " } );
	testCase( R"(test12( "12 \"  " ,  " \" 14 \" " ))", "test12", { "12 \"  ", " \" 14 \" " } );
	testCase( R"(test13( "\"\"" ,  "\"\"" ))", "test13", { "\"\"", "\"\"" } );
	testCase( R"(test14( ",,," , 1 , ",,," ))", "test14", { ",,,", "1", ",,," } );
	testCase( R"(    test15    (     ",,,"     ,     1     ,     ",,,"    ))", "test15",
			  { ",,,", "1", ",,," } );
	testCase( R"(test16(1,2) )", "test16", { "1", "2" } );
	testCase( R"(test17(func(12,32),2) )", "test17", { "func(12,32)", "2" } );
	testCase( R"(test17(func(12,32),2) )", "test17", { "func(12,32)", "2" } );
	testCase( R"(test18(     func(12,32)     , 2 ) )", "test18", { "func(12,32)", "2" } );
	testCase( R"(test19(func( "test" , "string" ), call(12,42)))", "test19",
			  { "func( \"test\" , \"string\" )", "call(12,42)" } );
	testCase( R"(test20(var(--font), var(--back)))", "test20", { "var(--font)", "var(--back)" } );
	testCase( R"(test21(  str ing  , stri ng2  ))", "test21", { "str ing", "stri ng2" } );
	testCase( R"(test22("   12    "   ,  "    14    "   ))", "test22",
			  { "   12    ", "    14    " } );
	testCase( R"(test23(  s t r i n g  , s t r i n g 2  ))", "test23",
			  { "s t r i n g", "s t r i n g 2" } );
	testCase( R"(test24(       func( "test" , "string" )    , call(12,42)    ))", "test24",
			  { "func( \"test\" , \"string\" )", "call(12,42)" } );
}
