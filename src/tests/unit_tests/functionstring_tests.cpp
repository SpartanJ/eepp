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
	testCase( R"(test25('string', 'string2'))", "test25", { "string", "string2" } );
	testCase( R"(test26('str\'ing', 'stri\'ng2'))", "test26", { "str\'ing", "stri\'ng2" } );
	testCase( R"(test27('str,ing', 'stri,ng2'))", "test27", { "str,ing", "stri,ng2" } );
	testCase( R"(test28( '12   ' ,  '  14  ' ))", "test28", { "12   ", "  14  " } );
	testCase( R"(test29(func( 'test' , 'string' ), call(12,42)))", "test29",
			  { "func( 'test' , 'string' )", "call(12,42)" } );
	testCase( R"(test30("str'ing", 'str"ing2'))", "test30", { "str'ing", "str\"ing2" } );
	testCase( R"(test31('', ''))", "test31", { "", "" } );
}

UTEST( FunctionString, nonOwningViewParser ) {
	constexpr std::string_view input =
		R"(  compose  ( first, nested(1, "two,three"), 'quoted, value', "escaped\"quote", '' ) )";
	auto function = FunctionString::parseView( input );
	EXPECT_FALSE( function.isEmpty() );
	EXPECT_TRUE( "compose"sv == function.getName() );

	SmallVector<std::string_view, 5> parameters;
	SmallVector<bool, 5> strings;
	EXPECT_TRUE( function.forEachParameter( [&]( std::string_view parameter, bool wasString ) {
		parameters.emplace_back( parameter );
		strings.emplace_back( wasString );
		return true;
	} ) );

	EXPECT_EQ( 5u, parameters.size() );
	EXPECT_TRUE( "first"sv == parameters[0] );
	EXPECT_TRUE( R"(nested(1, "two,three"))"sv == parameters[1] );
	EXPECT_TRUE( "quoted, value"sv == parameters[2] );
	EXPECT_TRUE( R"(escaped\"quote)"sv == parameters[3] );
	EXPECT_TRUE( ""sv == parameters[4] );
	EXPECT_FALSE( strings[0] );
	EXPECT_FALSE( strings[1] );
	EXPECT_TRUE( strings[2] );
	EXPECT_TRUE( strings[3] );
	EXPECT_TRUE( strings[4] );
}

UTEST( FunctionString, nonOwningViewParserCanStopEarly ) {
	auto function = FunctionString::parseView( "fn(one, two, three)"sv );
	int calls = 0;
	std::string_view firstParameter;
	EXPECT_FALSE( function.forEachParameter( [&]( std::string_view parameter, bool ) {
		++calls;
		firstParameter = parameter;
		return false;
	} ) );
	EXPECT_EQ( 1, calls );
	EXPECT_TRUE( "one"sv == firstParameter );
}
