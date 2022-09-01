#include <algorithm>
#include <array>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <iostream>

namespace EE { namespace UI { namespace CSS {

StyleSheet::StyleSheet() {}

template <class T> inline void HashCombine( std::size_t& seed, const T& v ) {
	std::hash<T> hasher;
	seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

size_t StyleSheet::nodeHash( const std::string& tag, const std::string& id ) {
	size_t seed = 0;
	if ( !tag.empty() )
		seed = std::hash<std::string>()( tag );
	if ( !id.empty() )
		HashCombine( seed, id );
	return seed;
}

void StyleSheet::invalidateCache() {
	mNodeCache.clear();
}

const Uint32& StyleSheet::getMarker() const {
	return mMarker;
}

void StyleSheet::setMarker( const Uint32& marker ) {
	if ( marker == mMarker )
		return;

	mMarker = marker;

	for ( auto& node : mNodes )
		node->setMarker( marker );

	for ( auto& keyframes : mKeyframesMap )
		keyframes.second.setMarker( marker );
}

void StyleSheet::removeAllWithMarker( const Uint32& marker ) {
	std::vector<std::shared_ptr<StyleSheetStyle>> removeNodes;

	for ( auto& node : mNodes )
		if ( node->getMarker() == marker )
			removeNodes.emplace_back( node );

	std::vector<size_t> deprecatedNodeIndex;
	for ( auto& nodeIndex : mNodeIndex ) {
		std::vector<StyleSheetStyle*> removeNodesIndex;
		for ( auto node : nodeIndex.second ) {
			if ( node->getMarker() == marker ) {
				removeNodesIndex.emplace_back( node );
			}
		}
		for ( auto removeNodeIndex : removeNodesIndex ) {
			auto found =
				std::find( nodeIndex.second.begin(), nodeIndex.second.end(), removeNodeIndex );
			if ( found != nodeIndex.second.end() )
				nodeIndex.second.erase( found );
		}
		if ( nodeIndex.second.empty() )
			deprecatedNodeIndex.emplace_back( nodeIndex.first );
	}

	for ( auto removeIndex : deprecatedNodeIndex )
		mNodeIndex.erase( removeIndex );

	std::vector<MediaQueryList::ptr> removeMediaQueries;
	for ( auto& mediaQueryList : mMediaQueryList ) {
		if ( mediaQueryList->getMarker() == marker )
			removeMediaQueries.emplace_back( mediaQueryList );
	}
	if ( !removeMediaQueries.empty() ) {
		for ( auto& removeMediaQuery : removeMediaQueries ) {
			auto found =
				std::find( mMediaQueryList.begin(), mMediaQueryList.end(), removeMediaQuery );
			if ( found != mMediaQueryList.end() )
				mMediaQueryList.erase( found );
		}
	}

	std::vector<std::string> removeKeys;
	for ( auto& keyFrame : mKeyframesMap ) {
		if ( keyFrame.second.getMarker() == marker )
			removeKeys.emplace_back( keyFrame.first );
	}
	for ( auto& removeKey : removeKeys )
		mKeyframesMap.erase( removeKey );

	invalidateCache();
}

bool StyleSheet::addStyleToNodeIndex( StyleSheetStyle* style ) {
	const std::string& id = style->getSelector().getSelectorId();
	const std::string& tag = style->getSelector().getSelectorTagName();
	if ( style->hasProperties() || style->hasVariables() ) {
		size_t nodeHash = this->nodeHash( "*" == tag ? "" : tag, id );
		StyleSheetStyleVector& nodes = mNodeIndex[nodeHash];
		auto it = std::find( nodes.begin(), nodes.end(), style );
		if ( it == nodes.end() ) {
			nodes.push_back( style );
			return true;
		} else {
			Log::debug( "Ignored style %s", style->getSelector().getName().c_str() );
		}
	}
	return false;
}

void StyleSheet::addStyle( std::shared_ptr<StyleSheetStyle> node ) {
	if ( addStyleToNodeIndex( node.get() ) ) {
		mNodes.push_back( node );
	}
	addMediaQueryList( node->getMediaQueryList() );
}

bool StyleSheet::isEmpty() const {
	return mNodes.empty();
}

void StyleSheet::print() {
	for ( auto& style : mNodes ) {
		std::cout << style->build();
	}
}

void StyleSheet::combineStyleSheet( const StyleSheet& styleSheet ) {
	for ( auto& style : styleSheet.getStyles() ) {
		addStyle( style );
	}

	addKeyframes( styleSheet.getKeyframes() );
}

inline static bool StyleSheetNodeSort( const StyleSheetStyle* lhs, const StyleSheetStyle* rhs ) {
	return lhs->getSelector().getSpecificity() < rhs->getSelector().getSpecificity();
}

// This is based on the RmlUi implementation.
std::shared_ptr<ElementDefinition> StyleSheet::getElementStyles( UIWidget* element,
																 const bool& applyPseudo ) const {
	static StyleSheetStyleVector applicableNodes;
	applicableNodes.clear();

	const std::string& tag = element->getElementTag();
	const std::string& id = element->getId();

	std::array<size_t, 4> nodeHash;
	int numHashes = 2;

	nodeHash[0] = 0;
	nodeHash[1] = this->nodeHash( tag, "" );

	if ( !id.empty() ) {
		numHashes = 4;
		nodeHash[2] = this->nodeHash( "", id );
		nodeHash[3] = this->nodeHash( tag, id );
	}

	for ( int i = 0; i < numHashes; i++ ) {
		auto itNodes = mNodeIndex.find( nodeHash[i] );
		if ( itNodes != mNodeIndex.end() ) {
			const StyleSheetStyleVector& nodes = itNodes->second;
			for ( StyleSheetStyle* node : nodes ) {
				if ( node->isMediaValid() && node->getSelector().select( element, applyPseudo ) ) {
					applicableNodes.push_back( node );
				}
			}
		}
	}

	std::sort( applicableNodes.begin(), applicableNodes.end(), StyleSheetNodeSort );

	if ( applicableNodes.empty() )
		return nullptr;

	size_t seed = 0;
	for ( const StyleSheetStyle* node : applicableNodes )
		HashCombine( seed, node );

	auto cacheIterator = mNodeCache.find( seed );
	if ( cacheIterator != mNodeCache.end() ) {
		std::shared_ptr<ElementDefinition>& definition = ( *cacheIterator ).second;
		return definition;
	}

	auto newDefinition = std::make_shared<ElementDefinition>( applicableNodes );
	mNodeCache[seed] = newDefinition;

	return newDefinition;
}

const std::vector<std::shared_ptr<StyleSheetStyle>>& StyleSheet::getStyles() const {
	return mNodes;
}

bool StyleSheet::updateMediaLists( const MediaFeatures& features ) {
	if ( mMediaQueryList.empty() )
		return false;

	bool updateStyles = false;

	for ( auto iter = mMediaQueryList.begin(); iter != mMediaQueryList.end(); iter++ ) {
		if ( ( *iter )->applyMediaFeatures( features ) )
			updateStyles = true;
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
		if ( node->getAtRuleType() == atRuleType ) {
			vector.push_back( node.get() );
		}
	}

	return vector;
}

bool StyleSheet::isKeyframesDefined( const std::string& keyframesName ) {
	return mKeyframesMap.find( keyframesName ) != mKeyframesMap.end();
}

const KeyframesDefinition& StyleSheet::getKeyframesDefinition( const std::string& keyframesName ) {
	static KeyframesDefinition EMPTY;
	const auto& it = mKeyframesMap.find( keyframesName );
	if ( it != mKeyframesMap.end() ) {
		return it->second;
	}
	return EMPTY;
}

void StyleSheet::addKeyframes( const KeyframesDefinition& keyframes ) {
	// "none" is a reserved keyword
	if ( keyframes.getName() != "none" )
		mKeyframesMap[keyframes.getName()] = keyframes;
}

void StyleSheet::addKeyframes( const KeyframesDefinitionMap& keyframesMap ) {
	for ( auto& keyframes : keyframesMap ) {
		addKeyframes( keyframes.second );
	}
}

const KeyframesDefinitionMap& StyleSheet::getKeyframes() const {
	return mKeyframesMap;
}

}}} // namespace EE::UI::CSS
