#ifndef EE_UI_CSS_STYLESHEETPARSER
#define EE_UI_CSS_STYLESHEETPARSER

#include <eepp/core/string.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetpropertiesparser.hpp>
#include <string>
using namespace EE;
using namespace EE::System;

namespace EE { namespace System {
class Pack;
}} // namespace EE::System

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetParser {
  public:
	StyleSheetParser();

	bool loadFromStream( IOStream& stream );

	bool loadFromFile( const std::string& file );

	bool loadFromMemory( const Uint8* RAWData, const Uint32& size );

	bool loadFromPack( Pack* pack, std::string filePackPath );

	bool loadFromString( const std::string& str );

	bool loadFromString( const std::string_view& str );

	void print();

	StyleSheet& getStyleSheet();

	const bool& isLoaded() const;

  protected:
	enum ReadState { ReadingSelector, ReadingProperty, ReadingComment };

	std::string mCSS;
	StyleSheet mStyleSheet;
	std::vector<std::string> mComments;
	MediaQueryList::ptr mMediaQueryList;
	bool mLoaded;

	bool parse( std::string& css, std::vector<std::string>& importedList );

	int readSelector( const std::string& css, ReadState& rs, std::size_t pos, std::string& buffer );

	int readComment( const std::string& css, ReadState& rs, std::size_t pos, std::string& buffer );

	int readProperty( const std::string& css, ReadState& rs, std::size_t pos, std::string& buffer );

	std::string importCSS( std::string path, std::vector<std::string>& importedList );

	void mediaParse( std::string& css, ReadState& rs, std::size_t& pos, std::string& buffer,
					 std::vector<std::string>& importedList );

	void importParse( std::string& css, std::size_t& pos, std::string& buffer,
					  std::vector<std::string>& importedList );

	void keyframesParse( std::string& css, ReadState& rs, std::size_t& pos, std::string& buffer );
};

}}} // namespace EE::UI::CSS

#endif
