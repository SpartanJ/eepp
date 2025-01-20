#include "processesmodel.hpp"
#include <eepp/ui/uiscenenode.hpp>

namespace ecode {

ProcessesModel::ProcessesModel( std::vector<std::pair<Uint64, std::string>>&& processes,
								UISceneNode* sceneNode ) :
	mProcesses( std::move( processes ) ), mSceneNode( sceneNode ) {}

size_t ProcessesModel::rowCount( const ModelIndex& ) const {
	Lock l( mResourceLock );
	return mFilter.empty() ? mProcesses.size() : mProcessesFiltered.size();
}

size_t ProcessesModel::columnCount( const ModelIndex& ) const {
	return 2;
}

std::string ProcessesModel::columnName( const size_t& colIdx ) const {
	return colIdx == 0 ? mSceneNode->i18n( "process_id", "Process ID" )
					   : mSceneNode->i18n( "command_line", "Command Line" );
}

Variant ProcessesModel::data( const ModelIndex& modelIndex, ModelRole role ) const {
	Lock l( mResourceLock );
	switch ( modelIndex.column() ) {
		case Columns::Name:
			return Variant( mFilter.empty() ? mProcesses[modelIndex.row()].second.c_str()
											: mProcessesFiltered[modelIndex.row()].second.data() );
		case Columns::ID:
		default:
			return role == ModelRole::Display
					   ? Variant( String::toString(
							 mFilter.empty() ? mProcesses[modelIndex.row()].first
											 : mProcessesFiltered[modelIndex.row()].first ) )
					   : Variant( mFilter.empty() ? mProcesses[modelIndex.row()].first
												  : mProcessesFiltered[modelIndex.row()].first );
	}
	return {};
}

bool containsIgnoreCase( std::string_view str, std::string_view substr ) {
	return std::search( str.begin(), str.end(), substr.begin(), substr.end(),
						[]( char ch1, char ch2 ) {
							return std::tolower( ch1 ) == std::tolower( ch2 );
						} ) != str.end();
}

void ProcessesModel::setFilter( const std::string& filter ) {
	if ( filter == mFilter )
		return;
	Lock l( mResourceLock );
	mFilter = filter;
	mProcessesFiltered.clear();
	for ( const auto& process : mProcesses ) {
		if ( containsIgnoreCase( process.second, filter ) )
			mProcessesFiltered.emplace_back( process.first, process.second );
	}
	invalidate();
}

void ProcessesModel::setProcesses( std::vector<std::pair<Uint64, std::string>>&& processes ) {
	{
		Lock l( mResourceLock );
		mProcesses = std::move( processes );
	}
	if ( !mFilter.empty() ) {
		auto filter = mFilter;
		mFilter.clear();
		setFilter( filter );
	} else {
		invalidate();
	}
}

std::pair<Uint64, std::string> ProcessesModel::getProcess( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return {};
	Lock l( mResourceLock );
	if ( mFilter.empty() ) {
		return mProcesses[index.row()];
	}
	return { mProcessesFiltered[index.row()].first,
			 std::string{ mProcessesFiltered[index.row()].second } };
}

} // namespace ecode
