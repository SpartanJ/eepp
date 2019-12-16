#include <algorithm>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetelement.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <iostream>

namespace EE { namespace UI { namespace CSS {

StyleSheet::StyleSheet() {}

void StyleSheet::addStyle( const StyleSheetStyle& node ) {
	mNodes.push_back( node );

	addMediaQueryList( node.getMediaQueryList() );
}

bool StyleSheet::isEmpty() const {
	return mNodes.empty();
}

void StyleSheet::print() {
	for ( auto& style : mNodes ) {
		std::cout << style.build();
	}
}

void StyleSheet::combineStyleSheet( const StyleSheet& styleSheet ) {
	for ( auto& style : styleSheet.getStyles() ) {
		addStyle( style );
	}
}

StyleSheetStyleVector StyleSheet::getElementStyles( StyleSheetElement* element,
													const bool& applyPseudo ) const {
	StyleSheetStyleVector styles;

	for ( const auto& node : mNodes ) {
		const StyleSheetSelector& selector = node.getSelector();

		if ( selector.select( element, applyPseudo ) ) {
			styles.push_back( node );
		}
	}

	return styles;
}

const StyleSheetStyleVector& StyleSheet::getStyles() const {
	return mNodes;
}

bool StyleSheet::updateMediaLists( const MediaFeatures& features ) {
	if ( mMediaQueryList.empty() )
		return false;

	bool updateStyles = false;

	for ( auto iter = mMediaQueryList.begin(); iter != mMediaQueryList.end(); iter++ ) {
		if ( ( *iter )->applyMediaFeatures( features ) ) {
			updateStyles = true;
			break;
		}
	}

	return updateStyles;
}

bool StyleSheet::isMediaQueryListEmpty() const {
	return mMediaQueryList.empty();
}

void StyleSheet::addMediaQueryList( MediaQueryList::ptr list ) {
	if ( list ) {
		if ( std::find( mMediaQueryList.begin(), mMediaQueryList.end(), list ) ==
			 mMediaQueryList.end() ) {
			mMediaQueryList.push_back( list );
		}
	}
}

StyleSheetStyleVector StyleSheet::getStyleSheetStyleByAtRule( const AtRuleType& atRuleType ) const {
	StyleSheetStyleVector vector;

	for ( auto& node : mNodes ) {
		if ( node.getAtRuleType() == atRuleType ) {
			vector.push_back( node );
		}
	}

	return vector;
}

}}} // namespace EE::UI::CSS
