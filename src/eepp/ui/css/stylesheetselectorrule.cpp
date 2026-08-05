#include <algorithm>
#include <eepp/ui/css/stylesheetselectorrule.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI { namespace CSS {

// This will be optimized as popcnt
static int numberOfSetBits( Uint32 i ) {
	i = i - ( ( i >> 1 ) & 0x55555555 );				  // add pairs of bits
	i = ( i & 0x33333333 ) + ( ( i >> 2 ) & 0x33333333 ); // quads
	i = ( i + ( i >> 4 ) ) & 0x0F0F0F0F;				  // groups of 8
	i *= 0x01010101;									  // horizontal sum of bytes
	return i >> 24; // return just that top byte (after truncating to 32-bit even when int is wider
					// than uint32_t)
}

static const char* StatePseudoClasses[] = { "focus",   "selected", "hover",
											"pressed", "disabled", "focus-within",
											"active",  "link",	   "visited" };

static bool isPseudoClassState( const std::string& pseudoClass ) {
	for ( Uint32 i = 0; i < eeARRAY_SIZE( StatePseudoClasses ); i++ ) {
		if ( pseudoClass == StatePseudoClasses[i] )
			return true;
	}

	return false;
}

static const char* StructuralPseudoClasses[] = {
	"checked",	   "disabled",		"empty",		  "enabled",
	"first-child", "first-of-type", "last-child",	  "last-of-type",
	"not",		   "nth-child",		"nth-last-child", "nth-last-of-type",
	"nth-of-type", "only-of-type",	"only-child",	  "where",
	"is" };

static bool isStructuralPseudoClass( const std::string& pseudoClass ) {
	for ( Uint32 i = 0; i < eeARRAY_SIZE( StructuralPseudoClasses ); i++ ) {
		if ( String::startsWith( pseudoClass, StructuralPseudoClasses[i] ) )
			return true;
	}

	return false;
}

static bool isDataAttributeName( std::string_view name ) {
	return String::istartsWith( name, "data-" );
}

static bool containsWord( const std::string& string, const std::string& word ) {
	size_t pos = 0;
	while ( pos < string.size() ) {
		while ( pos < string.size() && string[pos] == ' ' )
			pos++;

		size_t end = string.find( ' ', pos );
		if ( end == std::string::npos )
			end = string.size();

		if ( word.size() == end - pos && string.compare( pos, word.size(), word ) == 0 )
			return true;

		pos = end + 1;
	}
	return false;
}

static bool startsWithDashMatch( const std::string& string, const std::string& prefix ) {
	return string == prefix || ( string.size() > prefix.size() && string[prefix.size()] == '-' &&
								 string.compare( 0, prefix.size(), prefix ) == 0 );
}

StyleSheetSelectorRule::PseudoClasses
StyleSheetSelectorRule::toPseudoClass( std::string_view cls ) {
	if ( "focus" == cls )
		return StyleSheetSelectorRule::PseudoClasses::Focus;
	if ( "selected" == cls )
		return StyleSheetSelectorRule::PseudoClasses::Selected;
	if ( "hover" == cls )
		return StyleSheetSelectorRule::PseudoClasses::Hover;
	if ( "pressed" == cls || "active" == cls )
		return StyleSheetSelectorRule::PseudoClasses::Pressed;
	if ( "disabled" == cls )
		return StyleSheetSelectorRule::PseudoClasses::Disabled;
	if ( "focus-within" == cls )
		return StyleSheetSelectorRule::PseudoClasses::FocusWithin;
	if ( "link" == cls )
		return StyleSheetSelectorRule::PseudoClasses::Link;
	if ( "visited" == cls )
		return StyleSheetSelectorRule::PseudoClasses::Visited;
	// eeASSERT( false );
	return StyleSheetSelectorRule::PseudoClasses::None;
}

std::vector<const char*> StyleSheetSelectorRule::fromPseudoClass( Uint32 cls ) {
	std::vector<const char*> ret;
	ret.reserve( numberOfSetBits( cls ) );
	for ( Uint32 i = 0; i < PseudoClassesTotal; i++ )
		if ( cls & ( 1 << i ) )
			ret.push_back( StatePseudoClasses[i] );
	return ret;
}

StyleSheetSelectorRule::StyleSheetSelectorRule( const std::string& selectorFragment,
												PatternMatch patternMatch ) :
	mSpecificity( 0 ), mPatternMatch( patternMatch ), mRequirementFlags( 0 ) {
	parseFragment( selectorFragment );
}

void StyleSheetSelectorRule::pushSelectorTypeIdentifier( TypeIdentifier selectorTypeIdentifier,
														 std::string name ) {
	switch ( selectorTypeIdentifier ) {
		case GLOBAL:
			String::toLowerInPlace( name );
			mTagHash = String::hash( name );
			mTagName = std::move( name );
			mSpecificity += SpecificityGlobal;
			break;
		case TAG:
			String::toLowerInPlace( name );
			mTagHash = String::hash( name );
			mTagName = std::move( name );
			mSpecificity += SpecificityTag;
			break;
		case CLASS:
			mClassHashes.push_back( String::hash( name ) );
			mClasses.push_back( std::move( name ) );
			mSpecificity += SpecificityClass;
			break;
		case ID:
			mIdHash = String::hash( name );
			mId = std::move( name );
			mSpecificity += SpecificityId;
			break;
		default:
			break;
	}
}

void StyleSheetSelectorRule::parseFragment( const std::string& selectorFragment ) {
	TypeIdentifier curSelectorType = TAG;
	std::string buffer;
	int functionDepth = 0;
	bool inAttribute = false;

	auto processBuffer = [&]() {
		if ( buffer.empty() )
			return;

		if ( inAttribute ) {
			AttributeSelector attr;

			size_t opPos = buffer.find_first_of( "=~|^$*" );
			if ( opPos != std::string::npos ) {
				size_t eqPos = buffer.find( '=', opPos );
				attr.name = String::trim( buffer.substr( 0, opPos ) );

				// Extract the string operator and convert to Enum
				std::string opStr = buffer.substr( opPos, ( eqPos - opPos ) + 1 );
				attr.op = attributeOperatorFromString( opStr );

				std::string val = String::trim( buffer.substr( eqPos + 1 ) );
				String::trimInPlace( val, '"' );
				String::trimInPlace( val, '\'' );
				attr.value = val;
			} else {
				attr.name = String::trim( buffer );
				attr.op = AttributeOperator::None;
			}
			attr.isDataAttribute = isDataAttributeName( attr.name );
			if ( !attr.isDataAttribute )
				attr.propertyDefinition =
					StyleSheetSpecification::instance()->getProperty( attr.name );

			mAttributeSelectors.emplace_back( std::move( attr ) );
			mSpecificity += SpecificityClass;
			buffer.clear();
			return;
		}

		if ( curSelectorType == TAG || curSelectorType == CLASS || curSelectorType == ID ) {
			if ( buffer.size() == 1 && buffer[0] == GLOBAL )
				curSelectorType = GLOBAL;

			pushSelectorTypeIdentifier( curSelectorType, buffer );
		} else if ( curSelectorType == PSEUDO_CLASS ||
					curSelectorType == STRUCTURAL_PSEUDO_CLASS ) {
			if ( isPseudoClassState( buffer ) ) {
				mPseudoClasses |= toPseudoClass( buffer );
			} else if ( isStructuralPseudoClass( buffer ) ) {
				mStructuralPseudoClasses.push_back( buffer );

				StructuralSelector structuralSelector =
					StyleSheetSpecification::instance()->getStructuralSelector( buffer );

				if ( structuralSelector.selector ) {
					mStructuralSelectors.push_back( structuralSelector );
				}
			} else if ( buffer == "root" ) {
				// :root defines global variables. Map it to the GLOBAL wildcard
				// so variables are correctly attached to the root node hierarchy.
				pushSelectorTypeIdentifier( GLOBAL, "*" );
			} else {
				// Poison the rule for genuinely unsupported pseudo-classes
				pushSelectorTypeIdentifier( TAG, ":" + buffer );
			}
		}
		buffer.clear();
	};

	// Single forward pass
	for ( size_t i = 0; i < selectorFragment.size(); i++ ) {
		char curChar = selectorFragment[i];

		if ( curChar == '(' )
			functionDepth++;
		else if ( curChar == ')' )
			functionDepth--;

		// Catch attribute brackets
		if ( curChar == '[' && functionDepth == 0 ) {
			processBuffer(); // Flush previous tag/class
			inAttribute = true;
			continue;
		} else if ( curChar == ']' && inAttribute ) {
			processBuffer(); // Parse the attribute we just accumulated
			inAttribute = false;
			continue;
		}

		// Protect `eepp` sub-widgets. If we see `::`, it is part of the
		// tag name (e.g., tableview::cell). Append it and skip the split.
		if ( curChar == ':' && i + 1 < selectorFragment.size() && selectorFragment[i + 1] == ':' ) {
			buffer += "::";
			i++; // Skip the second colon
			continue;
		}

		// Only trigger a split if we aren't inside a function like :not()
		if ( functionDepth == 0 &&
			 ( curChar == CLASS || curChar == ID || curChar == PSEUDO_CLASS ) ) {
			processBuffer();
			curSelectorType = (TypeIdentifier)curChar;
		} else {
			buffer += curChar;
		}
	}

	processBuffer();

	// Calculate Requirement Flags and Specificity
	if ( !mTagName.empty() )
		mRequirementFlags |= TagName;

	if ( !mId.empty() )
		mRequirementFlags |= Id;

	if ( !mClasses.empty() )
		mRequirementFlags |= Class;

	std::sort( mClassHashes.begin(), mClassHashes.end() );
	mClassHashes.erase( std::unique( mClassHashes.begin(), mClassHashes.end() ),
						mClassHashes.end() );

	if ( !mAttributeSelectors.empty() )
		mRequirementFlags |= Attribute;

	if ( mPseudoClasses ) {
		mRequirementFlags |= PseudoClass;
		mSpecificity += SpecificityPseudoClass * numberOfSetBits( mPseudoClasses );
	}

	if ( !mStructuralPseudoClasses.empty() ) {
		mRequirementFlags |= StructuralPseudoClass;
		size_t count = 0;
		for ( const auto& spc : mStructuralPseudoClasses ) {
			if ( spc != "where" )
				count++;
		}
		mSpecificity += SpecificityStructuralPseudoClass * count;
	}
}

bool StyleSheetSelectorRule::hasClass( const std::string& cls ) const {
	return std::find( mClasses.begin(), mClasses.end(), cls ) != mClasses.end();
}

bool StyleSheetSelectorRule::hasPseudoClasses() const {
	return mPseudoClasses != 0;
}

bool StyleSheetSelectorRule::hasPseudoClass( const std::string& cls ) const {
	return mPseudoClasses & toPseudoClass( cls );
}

Uint32 StyleSheetSelectorRule::getPseudoClasses() const {
	return mPseudoClasses;
}

bool StyleSheetSelectorRule::hasStructuralPseudoClasses() const {
	return !mStructuralPseudoClasses.empty();
}

const std::vector<std::string>& StyleSheetSelectorRule::getStructuralPseudoClasses() const {
	return mStructuralPseudoClasses;
}

bool StyleSheetSelectorRule::hasStructuralPseudoClass( const std::string& cls ) const {
	return std::find( mStructuralPseudoClasses.begin(), mStructuralPseudoClasses.end(), cls ) !=
		   mStructuralPseudoClasses.end();
}

const std::string& StyleSheetSelectorRule::getTagName() const {
	return mTagName;
}

const std::string& StyleSheetSelectorRule::getId() const {
	return mId;
}

bool StyleSheetSelectorRule::matches( UIWidget* element, const bool& applyPseudo ) const {
	Uint32 flags = 0;

	if ( !mTagName.empty() ) {
		if ( mTagName != "*" ) {
			if ( mTagHash != element->getElementTagHash() ) {
				return false;
			} else {
				flags |= TagName;
			}
		} else {
			flags |= TagName;

			const Uint32 nonPseudoRequirements =
				mRequirementFlags & ~( PseudoClass | StructuralPseudoClass );
			if ( !applyPseudo && nonPseudoRequirements == TagName ) {
				return true;
			}
		}
	}

	if ( !mId.empty() ) {
		if ( mIdHash != element->getIdHash() ) {
			return false;
		} else {
			flags |= Id;
		}
	}

	if ( !mClassHashes.empty() ) {
		if ( element->getClassHashCount() < mClassHashes.size() )
			return false;

		if ( !element->hasClassHashes( mClassHashes ) )
			return false;

		flags |= Class;
	}

	if ( !mAttributeSelectors.empty() ) {
		for ( const auto& attr : mAttributeSelectors ) {
			const std::string* elVal;
			std::string elValStorage;

			if ( attr.isDataAttribute && element->isType( UI_TYPE_HTML_WIDGET ) ) {
				auto* htmlElement = element->asType<UIHTMLWidget>();
				const auto* property = htmlElement->getDataProperty( attr.name );
				if ( property == nullptr )
					return false;
				if ( attr.op == AttributeOperator::None )
					continue;
				elVal = &property->value();
			} else {
				elValStorage = element->getPropertyString( attr.propertyDefinition );
				if ( elValStorage.empty() )
					return false;
				if ( attr.op == AttributeOperator::None )
					continue;
				elVal = &elValStorage;
			}

			switch ( attr.op ) {
				case AttributeOperator::Exact: // =
					if ( *elVal != attr.value )
						return false;
					break;
				case AttributeOperator::StartsWith: // ^=
					if ( !String::startsWith( *elVal, attr.value ) )
						return false;
					break;
				case AttributeOperator::EndsWith: // $=
					if ( !String::endsWith( *elVal, attr.value ) )
						return false;
					break;
				case AttributeOperator::Contains: // *=
					if ( elVal->find( attr.value ) == std::string::npos )
						return false;
					break;
				case AttributeOperator::ContainsWord: { // ~= (Space-separated word check)
					if ( !containsWord( *elVal, attr.value ) ) {
						return false;
					}
					break;
				}
				case AttributeOperator::StartsWithDash: // |= (Exact match or starts with value
														// + "-")
					if ( !startsWithDashMatch( *elVal, attr.value ) ) {
						return false;
					}
					break;
				default:
					break;
			}
		}

		flags |= Attribute;
	}

	if ( applyPseudo ) {
		if ( mPseudoClasses ) {
			const Uint32 elementPseudoClasses = element->getStyleSheetPseudoClasses();
			if ( ( elementPseudoClasses & mPseudoClasses ) == mPseudoClasses )
				flags |= PseudoClass;
		}

		if ( !mStructuralSelectors.empty() ) {
			bool matchesStructural = true;

			for ( const auto& spc : mStructuralSelectors ) {
				if ( !spc.selector( element, spc.a, spc.b, spc.data ) ) {
					matchesStructural = false;
					break;
				}
			}

			if ( matchesStructural ) {
				flags |= StructuralPseudoClass;
			}
		}

		return mRequirementFlags == flags;
	}

	return ( mRequirementFlags & ~PseudoClass & ~StructuralPseudoClass ) == flags;
}

}}} // namespace EE::UI::CSS
