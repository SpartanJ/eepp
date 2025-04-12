#include <algorithm>
#include <eepp/ui/css/stylesheetselectorrule.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI { namespace CSS {

static int numberOfSetBits( Uint32 i ) {
	i = i - ( ( i >> 1 ) & 0x55555555 );				  // add pairs of bits
	i = ( i & 0x33333333 ) + ( ( i >> 2 ) & 0x33333333 ); // quads
	i = ( i + ( i >> 4 ) ) & 0x0F0F0F0F;				  // groups of 8
	i *= 0x01010101;									  // horizontal sum of bytes
	return i >> 24; // return just that top byte (after truncating to 32-bit even when int is wider
					// than uint32_t)
}

static const char* StatePseudoClasses[] = { "focus",	"selected",		"hover", "pressed",
											"disabled", "focus-within", "active" };

static bool isPseudoClassState( const std::string& pseudoClass ) {
	for ( Uint32 i = 0; i < eeARRAY_SIZE( StatePseudoClasses ); i++ ) {
		if ( pseudoClass == StatePseudoClasses[i] )
			return true;
	}

	return false;
}

static const char* StructuralPseudoClasses[] = {
	"checked",		  "disabled",		  "empty",		  "enabled",	  "first-child",
	"first-of-type",  "last-child",		  "last-of-type", "not",		  "nth-child",
	"nth-last-child", "nth-last-of-type", "nth-of-type",  "only-of-type", "only-child" };

static bool isStructuralPseudoClass( const std::string& pseudoClass ) {
	for ( Uint32 i = 0; i < eeARRAY_SIZE( StructuralPseudoClasses ); i++ ) {
		if ( String::startsWith( pseudoClass, StructuralPseudoClasses[i] ) )
			return true;
	}

	return false;
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
	eeASSERT( false );
	return StyleSheetSelectorRule::PseudoClasses::None;
}

static void splitSelectorPseudoClass( const std::string& selector, std::string& realSelector,
									  std::string& realPseudoClass ) {
	if ( !selector.empty() ) {
		bool lastWasColon = false;
		bool inFunction = false;

		for ( int i = (Int32)selector.size() - 1; i >= 0; i-- ) {
			char curChar = selector[i];

			if ( inFunction && curChar == '(' )
				inFunction = false;

			if ( inFunction )
				continue;

			if ( lastWasColon ) {
				if ( StyleSheetSelectorRule::PSEUDO_CLASS == curChar ) {
					// no pseudo class
					realSelector = selector;
				} else {
					if ( i + 2 <= (int)selector.size() ) {
						realSelector = selector.substr( 0, i + 1 );
						realPseudoClass = selector.substr( i + 2 );
					} else {
						realSelector = selector;
					}
				}

				return;
			} else if ( StyleSheetSelectorRule::PSEUDO_CLASS == curChar ) {
				lastWasColon = true;
			}

			if ( curChar == ')' ) {
				inFunction = true;
				lastWasColon = false;
			}
		}

		if ( lastWasColon ) {
			if ( selector.size() > 1 )
				realPseudoClass = selector.substr( 1 );
		} else {
			realSelector = selector;
		}
	}
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
			mTagName = name;
			mSpecificity += SpecificityGlobal;
			break;
		case TAG:
			mTagName = name;
			mSpecificity += SpecificityTag;
			break;
		case CLASS:
			mClasses.push_back( name );
			mSpecificity += SpecificityClass;
			break;
		case ID:
			mId = name;
			mSpecificity += SpecificityId;
			break;
		default:
			break;
	}
}

void StyleSheetSelectorRule::parseFragment( const std::string& selectorFragment ) {
	std::string selector = selectorFragment;
	std::string realSelector = "";
	std::string pseudoClass = "";

	do {
		pseudoClass.clear();
		realSelector.clear();

		if ( !selectorFragment.empty() && selectorFragment[0] != ':' ) {
			splitSelectorPseudoClass( selector, realSelector, pseudoClass );

			if ( !pseudoClass.empty() ) {
				if ( isPseudoClassState( pseudoClass ) ) {
					mPseudoClasses |= toPseudoClass( pseudoClass );
				} else if ( isStructuralPseudoClass( pseudoClass ) ) {
					mStructuralPseudoClasses.push_back( pseudoClass );

					StructuralSelector structuralSelector =
						StyleSheetSpecification::instance()->getStructuralSelector( pseudoClass );

					if ( structuralSelector.selector ) {
						mStructuralSelectors.push_back( structuralSelector );
					}
				}

				selector = realSelector;
			}
		}
	} while ( !pseudoClass.empty() );

	TypeIdentifier curSelectorType = TAG;
	std::string buffer;

	for ( auto charIt = selector.begin(); charIt != selector.end(); ++charIt ) {
		char curChar = *charIt;

		switch ( curChar ) {
			case CLASS: {
				if ( !buffer.empty() ) {
					pushSelectorTypeIdentifier( curSelectorType, buffer );
					buffer.clear();
				}

				curSelectorType = CLASS;

				break;
			}
			case ID: {
				if ( !buffer.empty() ) {
					pushSelectorTypeIdentifier( curSelectorType, buffer );
					buffer.clear();
				}

				curSelectorType = ID;

				break;
			}
			default: {
				buffer += curChar;
				break;
			}
		}
	}

	if ( !buffer.empty() ) {
		if ( buffer.size() == 1 && buffer[0] == GLOBAL )
			curSelectorType = GLOBAL;

		pushSelectorTypeIdentifier( curSelectorType, buffer );
	}

	if ( !mTagName.empty() )
		mRequirementFlags |= TagName;

	if ( !mId.empty() )
		mRequirementFlags |= Id;

	if ( !mClasses.empty() )
		mRequirementFlags |= Class;

	if ( mPseudoClasses ) {
		mRequirementFlags |= PseudoClass;
		mSpecificity += SpecificityPseudoClass * numberOfSetBits( mPseudoClasses );
	}

	if ( !mStructuralPseudoClasses.empty() ) {
		mRequirementFlags |= StructuralPseudoClass;
		mSpecificity += SpecificityStructuralPseudoClass * mStructuralPseudoClasses.size();
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
			if ( mTagName != element->getElementTag() ) {
				return false;
			} else {
				flags |= TagName;
			}
		} else {
			if ( !applyPseudo ) {
				return true;
			} else {
				flags |= TagName;
			}
		}
	}

	if ( !mId.empty() ) {
		if ( mId != element->getId() ) {
			return false;
		} else {
			flags |= Id;
		}
	}

	const std::vector<std::string>& elClasses = element->getStyleSheetClasses();
	if ( !mClasses.empty() && !elClasses.empty() ) {
		bool hasClasses = true;

		for ( const auto& cls : mClasses ) {
			if ( std::find( elClasses.begin(), elClasses.end(), cls ) == elClasses.end() ) {
				hasClasses = false;
				break;
			}
		}

		if ( hasClasses ) {
			flags |= Class;
		}
	}

	if ( applyPseudo ) {
		if ( mPseudoClasses && element->getStyleSheetPseudoClasses() ) {
			bool hasPseudoClasses = true;

			for ( Uint32 i = 0; i < PseudoClassesTotal; i++ ) {
				Uint32 pcls = ( 1 << i );
				if ( ( mPseudoClasses & pcls ) &&
					 !( element->getStyleSheetPseudoClasses() & pcls ) ) {
					hasPseudoClasses = false;
					break;
				}
			}

			if ( hasPseudoClasses ) {
				flags |= PseudoClass;
			}
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
