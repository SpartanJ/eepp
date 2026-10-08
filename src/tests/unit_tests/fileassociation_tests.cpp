#include "utest.hpp"

#include <algorithm>
#include <eepp/system/fileassociation.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

using namespace EE::System;
using namespace EE::UI::Doc;

UTEST( FileAssociation, normalizesExtensions ) {
	EXPECT_STDSTREQ( "cpp", FileAssociation::normalizeExtension( ".CPP" ) );
	EXPECT_STDSTREQ( "d.ts", FileAssociation::normalizeExtension( "..D.TS" ) );
	EXPECT_STDSTREQ( "c++", FileAssociation::normalizeExtension( "c++" ) );
	EXPECT_TRUE( FileAssociation::normalizeExtension( "bad/ext" ).empty() );
	EXPECT_TRUE( FileAssociation::normalizeExtension( "." ).empty() );
}

UTEST( SyntaxDefinitionManager, listsLiteralFileExtensionsWithoutLoadingDefinitions ) {
	auto* manager = SyntaxDefinitionManager::createSingleton();
	manager->addPreDefinition( { "Unit Test Language",
								 []() -> SyntaxDefinition& {
									 static SyntaxDefinition definition;
									 return definition;
								 },
								 { "%.unitext$", "^unitfile$" } } );
	const auto extensions = manager->getFileExtensions();

	EXPECT_TRUE( std::binary_search( extensions.begin(), extensions.end(), "cpp" ) );
	EXPECT_TRUE( std::binary_search( extensions.begin(), extensions.end(), "json" ) );
	EXPECT_TRUE( std::binary_search( extensions.begin(), extensions.end(), "unitext" ) );
	EXPECT_FALSE( std::binary_search( extensions.begin(), extensions.end(), "unitfile" ) );
}
