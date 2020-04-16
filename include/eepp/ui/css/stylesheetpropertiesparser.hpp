#ifndef EE_UI_CSS_STYLESHEETPROPERTIESPARSER_HPP
#define EE_UI_CSS_STYLESHEETPROPERTIESPARSER_HPP

#include <eepp/core/string.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetvariable.hpp>
#include <map>
#include <vector>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetPropertiesParser {
  public:
	StyleSheetPropertiesParser();

	explicit StyleSheetPropertiesParser( const std::string& propsstr );

	void parse( const std::string& propsstr );

	void print();

	const StyleSheetProperties& getProperties() const;

	const StyleSheetVariables& getVariables() const;

  protected:
	enum ReadState { ReadingPropertyName, ReadingPropertyValue, ReadingComment };

	ReadState mPrevRs;

	StyleSheetProperties mProperties;
	StyleSheetVariables mVariables;

	int readPropertyName( ReadState& rs, std::size_t pos, std::string& buffer,
						  const std::string& str );

	int readPropertyValue( ReadState& rs, std::size_t pos, std::string& buffer,
						   const std::string& str );

	int readComment( ReadState& rs, std::size_t pos, std::string& buffer, const std::string& str );

	void addProperty( std::string name, std::string value );
};

}}} // namespace EE::UI::CSS

#endif
