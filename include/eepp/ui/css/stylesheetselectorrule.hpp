#ifndef STYLESHEETSELECTORRULE_HPP
#define STYLESHEETSELECTORRULE_HPP

#include <eepp/core.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <stdint.h>

namespace EE { namespace UI {
class UIWidget;
}} // namespace EE::UI

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetSelectorRule {
  public:
	enum TypeIdentifier {
		TAG = 0,
		GLOBAL = '*',
		CLASS = '.',
		ID = '#',
		PSEUDO_CLASS = ':',
		STRUCTURAL_PSEUDO_CLASS = ':'
	};

	enum SelectorType {
		TagName = 1 << 0,
		Id = 1 << 1,
		Class = 1 << 2,
		PseudoClass = 1 << 3,
		StructuralPseudoClass = 1 << 4
	};

	enum SpecificityVal {
		SpecificityImportant = UINT32_MAX,
		SpecificityInline = UINT32_MAX - 1,
		SpecificityId = 1000000,
		SpecificityClass = 100000,
		SpecificityTag = 10000,
		SpecificityPseudoClass = 100,
		SpecificityStructuralPseudoClass = 50,
		SpecificityGlobal = 1
	};

	enum PatternMatch {
		ANY = '*',
		DESCENDANT = ' ',
		CHILD = '>',
		DIRECT_SIBLING = '+',
		SIBLING = '~',
		PREVIOUS_SIBLING = '|',
	};

	StyleSheetSelectorRule( const std::string& selectorFragment, PatternMatch mPatternMatch );

	void pushSelectorTypeIdentifier( TypeIdentifier selectorTypeIdentifier, std::string name );

	void parseFragment( const std::string& selectorFragment );

	const PatternMatch& getPatternMatch() const { return mPatternMatch; }

	const int& getSpecificity() const { return mSpecificity; }

	bool matches( UIWidget* element, const bool& applyPseudo = true ) const;

	bool hasClass( const std::string& cls ) const;

	bool hasPseudoClasses() const;

	bool hasPseudoClass( const std::string& cls ) const;

	const std::vector<std::string>& getPseudoClasses() const;

	bool hasStructuralPseudoClasses() const;

	const std::vector<std::string>& getStructuralPseudoClasses() const;

	bool hasStructuralPseudoClass( const std::string& cls ) const;

	const std::string& getTagName() const;

	const std::string& getId() const;

  protected:
	int mSpecificity;
	PatternMatch mPatternMatch;
	std::string mTagName;
	std::string mId;
	std::vector<std::string> mClasses;
	std::vector<std::string> mPseudoClasses;
	std::vector<std::string> mStructuralPseudoClasses;
	std::vector<StructuralSelector> mStructuralSelectors;
	Uint32 mRequirementFlags;
};

}}} // namespace EE::UI::CSS

#endif
