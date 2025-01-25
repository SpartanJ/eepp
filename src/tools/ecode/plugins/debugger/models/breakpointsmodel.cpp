#include "breakpointsmodel.hpp"
#include <eepp/ui/uiscenenode.hpp>

namespace ecode {

BreakpointsModel::BreakpointsModel(
	const UnorderedMap<std::string, UnorderedSet<SourceBreakpointStateful>>& breakpoints,
	UISceneNode* sceneNode ) :
	mSceneNode( sceneNode ) {
	for ( const auto& bpf : breakpoints )
		for ( const auto& bp : bpf.second )
			mBreakpoints.emplace_back( bpf.first, bp );
}

size_t BreakpointsModel::rowCount( const ModelIndex& ) const {
	Lock l( mResourceLock );
	return mBreakpoints.size();
}

size_t BreakpointsModel::columnCount( const ModelIndex& ) const {
	return Columns::Count;
}

std::string BreakpointsModel::columnName( const size_t& index ) const {
	Lock l( mResourceLock );
	switch ( index ) {
		case Columns::Enabled:
			return mSceneNode->i18n( "enabled", "Enabled" );
		case Columns::SourcePath:
			return mSceneNode->i18n( "source_path", "Source Path" );
		case Columns::Line:
			return mSceneNode->i18n( "line", "Line" );
		case Columns::Remove:
			return mSceneNode->i18n( "remove", "Remove" );
	}
	return "";
}

Variant BreakpointsModel::data( const ModelIndex& modelIndex, ModelRole role ) const {
	Lock l( mResourceLock );

	switch ( role ) {
		case ModelRole::Display: {
			switch ( modelIndex.column() ) {
				case Columns::Enabled:
					return Variant( "" );
				case Columns::SourcePath:
					return Variant( mBreakpoints[modelIndex.row()].first.c_str() );
				case Columns::Line:
					return Variant(
						String::toString( mBreakpoints[modelIndex.row()].second.line ) );
			}
			break;
		}
		case ModelRole::Data: {
			switch ( modelIndex.column() ) {
				case Columns::Enabled:
					return Variant( mBreakpoints[modelIndex.row()].second.enabled );
				case Columns::SourcePath:
					return Variant( mBreakpoints[modelIndex.row()].first.c_str() );
				case Columns::Line:
					return Variant( mBreakpoints[modelIndex.row()].second.line );
			}
			break;
		}
		case ModelRole::Icon: {
			if ( modelIndex.column() == Columns::Remove ) {
				static UIIcon* eraseIcon = mSceneNode->findIcon( "chrome-close" );
				return Variant( eraseIcon );
			}
			break;
		}
		default:
			break;
	}

	return {};
}

void BreakpointsModel::insert( const std::string& filePath,
							   const SourceBreakpointStateful& breakpoint ) {
	Lock l( mResourceLock );
	mBreakpoints.emplace_back( filePath, breakpoint );
	invalidate( Model::UpdateFlag::DontInvalidateIndexes );
}

void BreakpointsModel::erase( const std::string& filePath,
							  const SourceBreakpointStateful& breakpoint ) {
	Lock l( mResourceLock );
	auto found = std::find( mBreakpoints.begin(), mBreakpoints.end(),
							std::make_pair( filePath, breakpoint ) );
	if ( found != mBreakpoints.end() ) {
		mBreakpoints.erase( found );
		invalidate( Model::UpdateFlag::InvalidateAllIndexes );
	}
}

void BreakpointsModel::enable( const std::string& filePath,
							   const SourceBreakpointStateful& breakpoint, bool enable ) {
	Lock l( mResourceLock );
	auto found = std::find( mBreakpoints.begin(), mBreakpoints.end(),
							std::make_pair( filePath, breakpoint ) );
	if ( found != mBreakpoints.end() ) {
		found->second.enabled = enable;
		invalidate( Model::UpdateFlag::DontInvalidateIndexes );
	}
}

const std::pair<std::string, SourceBreakpointStateful>& BreakpointsModel::get( ModelIndex index ) {
	static std::pair<std::string, SourceBreakpointStateful> EMPTY = {};
	if ( !index.isValid() || index.row() >= static_cast<Int64>( mBreakpoints.size() ) )
		return EMPTY;
	return mBreakpoints[index.row()];
}

} // namespace ecode
