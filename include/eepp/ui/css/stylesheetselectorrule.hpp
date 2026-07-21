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
		Link = ( 1 << 6 ),
		Visited = ( 1 << 7 ),
	};

	static constexpr auto PseudoClassesTotal = 8;

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
		StructuralPseudoClass = 1 << 4,
		Attribute = 1 << 5,
	};

	// Packed CSS specificity tuple. This matches lexicographic specificity comparison while
	// selector buckets remain below SpecificityClass.
	enum SpecificityVal : Int64 {
		SpecificityImportant = 1000000000000000LL,
		SpecificityInline = 1000000000000LL,
		SpecificityId = 1048576LL,
		SpecificityClass = 1024LL,
		SpecificityTag = 1LL,
		SpecificityPseudoClass = SpecificityClass,
		SpecificityStructuralPseudoClass = SpecificityClass,
		SpecificityGlobal = 0LL
	};

	enum PatternMatch {
		ANY = '*',
		DESCENDANT = ' ',
		CHILD = '>',
		DIRECT_SIBLING = '+',
		SIBLING = '~',
		PREVIOUS_SIBLING = '|',
	};

	enum class AttributeOperator {
		None,
		Exact,			// =
		ContainsWord,	// ~=
		StartsWithDash, // |=
		StartsWith,		// ^=
		EndsWith,		// $=
		Contains		// *=
	};

	static AttributeOperator attributeOperatorFromString( const std::string& op ) {
		if ( op == "=" )
			return AttributeOperator::Exact;
		if ( op == "~=" )
			return AttributeOperator::ContainsWord;
		if ( op == "|=" )
			return AttributeOperator::StartsWithDash;
		if ( op == "^=" )
			return AttributeOperator::StartsWith;
		if ( op == "$=" )
			return AttributeOperator::EndsWith;
		if ( op == "*=" )
			return AttributeOperator::Contains;
		return AttributeOperator::None;
	}

	static std::string attributeOperatorToString( AttributeOperator op ) {
		switch ( op ) {
			case AttributeOperator::Exact:
				return "=";
			case AttributeOperator::ContainsWord:
				return "~=";
			case AttributeOperator::StartsWithDash:
				return "|=";
			case AttributeOperator::StartsWith:
				return "^=";
			case AttributeOperator::EndsWith:
				return "$=";
			case AttributeOperator::Contains:
				return "*=";
			default:
				return "";
		}
	}

	struct AttributeSelector {
		std::string name;
		std::string value;
		const PropertyDefinition* propertyDefinition{ nullptr };
		AttributeOperator op{ AttributeOperator::None };
		bool isDataAttribute{ false };
	};

	static PseudoClasses toPseudoClass( std::string_view cls );

	static std::vector<const char*> fromPseudoClass( Uint32 cls );

	StyleSheetSelectorRule( const std::string& selectorFragment, PatternMatch mPatternMatch );

	void pushSelectorTypeIdentifier( TypeIdentifier selectorTypeIdentifier, std::string name );

	void parseFragment( const std::string& selectorFragment );

	const PatternMatch& getPatternMatch() const { return mPatternMatch; }

	const Int64& getSpecificity() const { return mSpecificity; }

	bool matches( UIWidget* element, const bool& applyPseudo = true ) const;

	bool hasClass( const std::string& cls ) const;

	/** @return True when this rule requires at least one CSS class. */
	bool hasClasses() const { return !mClassHashes.empty(); }

	/** @return Sorted unique hashes of every CSS class required by this rule. */
	const std::vector<String::HashType>& getClassHashes() const { return mClassHashes; }

	/**
	 * @return The mandatory class used to place this rule in the stylesheet candidate index.
	 * Every matching element must have this class, but the full rule is still validated by
	 * matches(). The first sorted hash is used for now; a future implementation may choose the
	 * rarest class to reduce the candidate bucket further.
	 */
	String::HashType getBestClassHash() const {
		return mClassHashes.empty() ? 0 : mClassHashes.front();
	}

	bool hasPseudoClasses() const;

	bool hasPseudoClass( const std::string& cls ) const;

	Uint32 getPseudoClasses() const;

	bool hasStructuralPseudoClasses() const;

	const std::vector<std::string>& getStructuralPseudoClasses() const;

	bool hasStructuralPseudoClass( const std::string& cls ) const;

	const std::string& getTagName() const;

	const std::string& getId() const;

  protected:
	Int64 mSpecificity{ 0 };
	PatternMatch mPatternMatch;
	std::string mTagName;
	std::string mId;
	std::vector<std::string> mClasses;
	String::HashType mTagHash{ 0 };
	String::HashType mIdHash{ 0 };
	std::vector<String::HashType> mClassHashes;
	std::vector<std::string> mStructuralPseudoClasses;
	std::vector<StructuralSelector> mStructuralSelectors;
	std::vector<AttributeSelector> mAttributeSelectors;
	Uint32 mPseudoClasses{ 0 };
	Uint32 mRequirementFlags{ 0 };
};

}}} // namespace EE::UI::CSS

#endif
