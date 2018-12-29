#ifndef EE_UI_CSS_STYLESHEETSELECTOR_HPP
#define EE_UI_CSS_STYLESHEETSELECTOR_HPP

#include <eepp/core.hpp>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetSelector {
	public:
		enum SelectorType {
			TagName = 1 << 0,
			Id = 1 << 1,
			Class = 1 << 2,
			PseudoClass = 1 << 3
		};

		enum SpecificityVal {
			SpecificityId = 1000000,
			SpecificityClass = 100000,
			SpecificityTag = 10000,
			SpecificityPseudoClass = 100
		};

		explicit StyleSheetSelector( const std::string& selectorName );

		Uint32 getRequiredFlags();

		const std::string& getName() const;

		const std::string& getTagName() const;

		const std::string getId() const;

		const std::vector<std::string> getClasses() const;

		const std::string& getPseudoClass() const;

		bool hasTagName();

		bool hasId();

		bool hasClasses();

		bool hasClass( std::string cls );

		bool hasPseudoClass();

		Uint32 getSpecificity();
	protected:
		std::string name;
		std::string tagName;
		std::string id;
		std::vector<std::string> classes;
		std::string pseudoClass;
		Uint32 specificity;

		void parseSelector( const std::string& selector );
};

}}}

#endif
