#ifndef EE_UI_CSS_STYLESHEETPROPERTIESPARSER_HPP
#define EE_UI_CSS_STYLESHEETPROPERTIESPARSER_HPP

#include <eepp/core/string.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetvariable.hpp>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetPropertiesParser {
  public:
	StyleSheetPropertiesParser() = default;

	explicit StyleSheetPropertiesParser( std::string_view propsstr );

	void parse( std::string_view propsstr );

	void print();

	const StyleSheetProperties& getProperties() const;

	const StyleSheetVariables& getVariables() const;

  protected:
	enum ReadState { ReadingPropertyName, ReadingPropertyValue, ReadingComment };

	ReadState mPrevRs{ ReadingPropertyName };

	StyleSheetProperties mProperties;
	StyleSheetVariables mVariables;

	int readPropertyName( ReadState& rs, std::size_t pos, std::string& buffer,
						  std::string_view str );

	int readPropertyValue( ReadState& rs, std::size_t pos, std::string& buffer,
						   std::string_view str );

	int readComment( ReadState& rs, std::size_t pos, std::string& buffer, std::string_view str );

	void addProperty( std::string name, std::string value );
};

}}} // namespace EE::UI::CSS

#endif
