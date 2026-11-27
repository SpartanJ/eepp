#include "utest.hpp"
#include <eepp/system/pluralrules.hpp>
#include <limits>

using namespace EE;
using namespace EE::System;
using Category = PluralCategory;

UTEST( PluralRules, SelectsAllCardinalCategoriesAndRegionalExceptions ) {
	const LocaleId arabic( "ar-EG" );
	EXPECT_TRUE( PluralRules::select( arabic, 0 ) == Category::Zero );
	EXPECT_TRUE( PluralRules::select( arabic, -1 ) == Category::One );
	EXPECT_TRUE( PluralRules::select( arabic, 2u ) == Category::Two );
	EXPECT_TRUE( PluralRules::select( arabic, 103 ) == Category::Few );
	EXPECT_TRUE( PluralRules::select( arabic, 111 ) == Category::Many );
	EXPECT_TRUE( PluralRules::select( arabic, 102 ) == Category::Other );
	EXPECT_TRUE( PluralRules::select( LocaleId( "pt-BR" ), 0 ) == Category::One );
	EXPECT_TRUE( PluralRules::select( LocaleId( "pt-PT" ), 0 ) == Category::Other );
	EXPECT_TRUE( PluralRules::select( LocaleId( "pt-AO" ), 0 ) == Category::Other );
	EXPECT_TRUE( PluralRules::select( LocaleId( "zh-Hant-TW" ), 1 ) == Category::Other );
	EXPECT_TRUE( PluralRules::select( LocaleId( "xxx" ), 1 ) == Category::Other );
}

UTEST( PluralRules, RespectsVisibleFractionsAndIntegerRanges ) {
	const LocaleId english( "en" ), russian( "ru" ), slovenian( "sl" ), latvian( "lv" );
	EXPECT_TRUE( PluralRules::select( english, "1" ) == Category::One );
	EXPECT_TRUE( PluralRules::select( english, "1.0" ) == Category::Other );
	EXPECT_TRUE( PluralRules::select( russian, "2" ) == Category::Few );
	EXPECT_TRUE( PluralRules::select( russian, "2.0" ) == Category::Other );
	EXPECT_TRUE( PluralRules::select( slovenian, "2.0" ) == Category::Few );
	EXPECT_TRUE( PluralRules::select( latvian, "0.11" ) == Category::Zero );
	EXPECT_TRUE( PluralRules::select( latvian, "0.110" ) == Category::Other );
	EXPECT_TRUE( PluralRules::select( LocaleId( "is" ), "2.10" ) == Category::One );
	EXPECT_TRUE( PluralRules::select( LocaleId( "ar" ), "103.01" ) == Category::Other );
	EXPECT_TRUE( PluralRules::select( LocaleId( "ar" ), "103.000" ) == Category::Few );
}

UTEST( PluralRules, HandlesWideValuesAndRejectsUnsupportedNotation ) {
	const LocaleId russian( "ru" ), french( "fr" ), arabic( "ar" );
	EXPECT_TRUE( PluralRules::select( russian, std::numeric_limits<Int64>::min() ) ==
				 Category::Many );
	EXPECT_TRUE( PluralRules::select( russian, std::numeric_limits<Uint64>::max() ) ==
				 Category::Many );
	EXPECT_TRUE( PluralRules::select( russian, "184467440737095516160000000000001" ) ==
				 Category::One );
	EXPECT_TRUE( PluralRules::select( russian, "-184467440737095516160000000000022" ) ==
				 Category::Few );
	EXPECT_TRUE( PluralRules::select( french, "99999999999999999999999000000" ) == Category::Many );
	EXPECT_TRUE( PluralRules::select( arabic, "1.00000000000000000000000000000000001" ) ==
				 Category::Other );
	EXPECT_TRUE( PluralRules::select( LocaleId( "is" ), "2.1234567890123456789010000" ) ==
				 Category::One );
	for ( auto invalid : { "", "1.", " 1", "1e6", "1c6", "1,0", "1\t", "1..0", "NaN" } )
		EXPECT_TRUE( PluralRules::select( french, invalid ) == Category::Other );
}
