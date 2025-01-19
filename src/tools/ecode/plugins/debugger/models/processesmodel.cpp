#include "processesmodel.hpp"
#include <eepp/ui/uiscenenode.hpp>

namespace ecode {

ProcessesModel::ProcessesModel( std::vector<std::pair<Uint64, std::string>>&& processes,
								UISceneNode* sceneNode ) :
	mProcesses( std::move( processes ) ), mSceneNode( sceneNode ) {}

size_t ProcessesModel::rowCount( const ModelIndex& ) const {
	return mProcesses.size();
}

size_t ProcessesModel::columnCount( const ModelIndex& ) const {
	return 2;
}

std::string ProcessesModel::columnName( const size_t& colIdx ) const {
	return colIdx == 0 ? mSceneNode->i18n( "process_id", "Process ID" )
					   : mSceneNode->i18n( "command_line", "Command Line" );
}

Variant ProcessesModel::data( const ModelIndex& modelIndex, ModelRole role ) const {
	switch ( modelIndex.column() ) {
		case Columns::Name:
			return Variant( mProcesses[modelIndex.row()].second );
		case Columns::ID:
		default:
			return role == ModelRole::Display
					   ? Variant( String::toString( mProcesses[modelIndex.row()].first ) )
					   : Variant( mProcesses[modelIndex.row()].first );
	}
	return {};
}

} // namespace ecode
