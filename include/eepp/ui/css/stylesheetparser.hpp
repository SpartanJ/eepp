#ifndef EE_UI_CSS_STYLESHEETPARSER
#define EE_UI_CSS_STYLESHEETPARSER

#include <eepp/core/string.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetnode.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <string>
#include <map>
#include <iostream>
using namespace EE;
using namespace EE::System;

namespace EE { namespace System {
class Pack;
}}

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetParser {
	public:
		StyleSheetParser();

		bool loadFromStream( IOStream& stream );

		bool loadFromFile( const std::string& file );

		bool loadFromMemory( const Uint8* RAWData, const Uint32& size );

		bool loadFromPack( Pack * pack, std::string filePackPath );

		void print();

		StyleSheet& getStyleSheet();
	protected:
		enum ReadState {
			ReadingStyle,
			ReadingProperty,
			ReadingComment
		};

		std::string mCSS;

		StyleSheet mStyleSheet;

		std::vector<std::string> mComments;

		bool parse();

		int readStyle( ReadState& rs, std::size_t pos, std::string& buffer );

		int readComment( ReadState& rs, std::size_t pos, std::string& buffer );

		int readProperty( ReadState& rs, std::size_t pos, std::string& buffer );
};

}}}

#endif
