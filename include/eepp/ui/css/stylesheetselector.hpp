#ifndef EE_UI_CSS_STYLESHEETSELECTOR_HPP
#define EE_UI_CSS_STYLESHEETSELECTOR_HPP

#include <eepp/core.hpp>

namespace EE { namespace UI { namespace CSS {

class StyleSheetElement;

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
			SpecificityPseudoClass = 100,
			SpecificityGlobal = 1
		};

		enum PatternMatch {
			ANY = '*',
			DESCENDANT = ' ',
			CHILD = '>',
			DIRECT_SIBLING = '+',
			SIBLING = '~'
		};

		enum SelectoryTypeIdentifier {
			TAG = 0,
			CLASS = '.',
			ID = '#',
			PSEUDO_CLASS = ':',
			STRUCTURAL_PSEUDO_CLASS = ':'
		};

		class SelectorRule {
			public:
				SelectorRule( const std::string& selectorFragment, PatternMatch patternMatch );
				void pushSelectorTypeIdentifier( SelectoryTypeIdentifier selectorTypeIdentifier, std::string name );
				void parseFragment( const std::string& selectorFragment );
				void match( StyleSheetElement * element );
				const int& getSpecificity() const { return specificity; }

				int specificity;
				PatternMatch patternMatch;
				std::string tagName;
				std::string id;
				std::vector<std::string> classes;
		};

		StyleSheetSelector();

		explicit StyleSheetSelector( const std::string& selectorName );

		Uint32 getRequiredFlags() const;

		const std::string& getName() const;

		const std::string& getTagName() const;

		const std::string getId() const;

		const std::vector<std::string>& getClasses() const;

		const std::string& getPseudoClass() const;

		bool hasTagName() const;

		bool hasId() const;

		bool hasClasses() const;

		bool hasClass( std::string cls ) const;

		bool hasPseudoClass() const;

		bool isGlobal() const;

		const Uint32& getSpecificity() const;
	protected:
		std::string mName;
		std::string mTagName;
		std::string mId;
		std::vector<std::string> mClasses;
		std::string mPseudoClass;
		Uint32 mSpecificity;
		bool mGlobal;
		std::vector<SelectorRule> mSelectorRules;

		void addSelectorRule(std::string& buffer, PatternMatch& curPatternMatch, const PatternMatch & newPatternMatch );

		void parseSelector( const std::string& selector );

		void realParseSelector( const std::string& selector );
};

}}}

#endif
