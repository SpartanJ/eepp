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
	enum PseudoClasses {
		None = 0,
		Focus = ( 1 << 0 ),
		Selected = ( 1 << 1 ),
		Hover = ( 1 << 2 ),
		Pressed = ( 1 << 3 ),
		Disabled = ( 1 << 4 ),
		FocusWithin = ( 1 << 5 ),
	};

	static constexpr auto PseudoClassesTotal = 6;

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

	static PseudoClasses toPseudoClass( std::string_view cls );

	static std::vector<const char*> fromPseudoClass( Uint32 cls );

	StyleSheetSelectorRule( const std::string& selectorFragment, PatternMatch mPatternMatch );

	void pushSelectorTypeIdentifier( TypeIdentifier selectorTypeIdentifier, std::string name );

	void parseFragment( const std::string& selectorFragment );

	const PatternMatch& getPatternMatch() const { return mPatternMatch; }

	const int& getSpecificity() const { return mSpecificity; }

	bool matches( UIWidget* element, const bool& applyPseudo = true ) const;

	bool hasClass( const std::string& cls ) const;

	bool hasPseudoClasses() const;

	bool hasPseudoClass( const std::string& cls ) const;

	Uint32 getPseudoClasses() const;

	bool hasStructuralPseudoClasses() const;

	const std::vector<std::string>& getStructuralPseudoClasses() const;

	bool hasStructuralPseudoClass( const std::string& cls ) const;

	const std::string& getTagName() const;

	const std::string& getId() const;

  protected:
	int mSpecificity{ 0 };
	PatternMatch mPatternMatch;
	std::string mTagName;
	std::string mId;
	std::vector<std::string> mClasses;
	std::vector<std::string> mStructuralPseudoClasses;
	std::vector<StructuralSelector> mStructuralSelectors;
	Uint32 mPseudoClasses{ 0 };
	Uint32 mRequirementFlags{ 0 };
};

}}} // namespace EE::UI::CSS

#endif
