#include <eepp/ui/doc/foldrangeservice.hpp>
#include <eepp/ui/doc/textdocument.hpp>

namespace EE { namespace UI { namespace Doc {

FoldRangeServive::FoldRangeServive( TextDocument* doc ) : mDoc( doc ) {}

void FoldRangeServive::findRegions() {
	if ( mDoc == nullptr )
		return;

	if ( mProvider && mProvider( mDoc ) )
		return;

	switch ( mDoc->getSyntaxDefinition().getFoldRangeType() ) {
		case FoldRangeType::Braces:
			break;
		case FoldRangeType::Indentation:
		case FoldRangeType::Tag:
		case FoldRangeType::Undefined:
			break;
	}
}

void FoldRangeServive::clear() {
	Lock l( mMutex );
	mFoldingRegions.clear();
}

bool FoldRangeServive::empty() {
	Lock l( mMutex );
	return mFoldingRegions.empty();
}

std::optional<TextRange> FoldRangeServive::find( Int64 docIdx ) {
	Lock l( mMutex );
	auto foldRegionIt = mFoldingRegions.find( docIdx );
	if ( foldRegionIt == mFoldingRegions.end() )
		return {};
	return foldRegionIt->second;
}

void FoldRangeServive::addFoldRegion( TextRange region ) {
	Lock l( mMutex );
	region.normalize();
	mFoldingRegions[region.start().line()] = std::move( region );
}

bool FoldRangeServive::isFoldingRegionInLine( Int64 docIdx ) {
	Lock l( mMutex );
	auto foldRegionIt = mFoldingRegions.find( docIdx );
	return foldRegionIt != mFoldingRegions.end();
}

void FoldRangeServive::shiftFoldingRegions( Int64 fromLine, Int64 numLines ) {
	// TODO: Optimize this
	Lock l( mMutex );
	std::unordered_map<Int64, TextRange> foldingRegions;
	for ( auto& foldingRegion : mFoldingRegions ) {
		if ( foldingRegion.second.start().line() > fromLine ) {
			foldingRegion.second.start().setLine( foldingRegion.second.start().line() + numLines );
			foldingRegion.second.end().setLine( foldingRegion.second.end().line() + numLines );
			foldingRegions[foldingRegion.second.start().line()] = foldingRegion.second;
		}
		foldingRegions[foldingRegion.second.start().line()] = foldingRegion.second;
	}
	mFoldingRegions = foldingRegions;
}

void FoldRangeServive::setFoldingRegions( std::vector<TextRange> regions ) {
	Lock l( mMutex );
	mFoldingRegions.clear();
	std::sort( regions.begin(), regions.end() );
	for ( auto& range : regions ) {
		auto line = range.start().line();
		mFoldingRegions[line] = std::move( range );
	}
}

const FoldRangeServive::FoldRangeProvider& FoldRangeServive::getProvider() const {
	return mProvider;
}

void FoldRangeServive::setProvider( const FoldRangeProvider& provider ) {
	mProvider = provider;
}

}}} // namespace EE::UI::Doc
