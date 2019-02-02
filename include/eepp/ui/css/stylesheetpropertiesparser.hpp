#ifndef EE_UI_CSS_STYLESHEETPROPERTIESPARSER_HPP
#define EE_UI_CSS_STYLESHEETPROPERTIESPARSER_HPP

#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/core/string.hpp>
#include <map>
#include <vector>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetPropertiesParser {
	public:
		StyleSheetPropertiesParser();

		explicit StyleSheetPropertiesParser( const std::string& propsstr );

		void print();

		const StyleSheetProperties& getProperties() const;

	protected:
		enum ReadState {
			ReadingPropertyName,
			ReadingPropertyValue,
			ReadingValueUrl,
			ReadingComment
		};

		ReadState mPrevRs;

		StyleSheetProperties mProperties;

		void parse( std::string propsstr );

		int readPropertyName( ReadState& rs, std::size_t pos, std::string& buffer, const std::string& str );

		int readPropertyValue( ReadState& rs, std::size_t pos, std::string& buffer, const std::string& str );

		int readComment( ReadState& rs, std::size_t pos, std::string& buffer, const std::string& str );

		int readValueUrl( ReadState& rs, std::size_t pos, std::string& buffer, const std::string& str );

		void addProperty( const std::string& name, std::string value );
};

}}}

#endif
