#include "gitstatusmodel.hpp"
#include "gitplugin.hpp"

#include <map>
#include <set>

#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiiconthememanager.hpp>

namespace ecode {

GitStatusModel::GitStatusModel( Git::FilesStatus&& status, GitPlugin* gitPlugin ) :
	mPlugin( gitPlugin ) {
	std::map<std::string, std::set<Git::GitStatusType>> typesFound;
	std::unordered_map<std::string, size_t> repoPos;
	std::unordered_map<size_t, std::unordered_map<Git::GitStatusType, size_t>> repoTypePos;

	for ( auto& s : status )
		for ( auto& f : s.second )
			typesFound[s.first].insert( f.report.type );

	for ( const auto& tf : typesFound ) {
		RepoStatus rs;
		rs.repo = tf.first;
		size_t pos = mStatus.size();
		repoPos[rs.repo] = pos;
		for ( const auto& s : tf.second ) {
			RepoStatusType rt;
			rt.typeStr = mPlugin->statusTypeToString( s );
			rt.type = s;
			repoTypePos[pos][s] = rs.type.size();
			rs.type.emplace_back( std::move( rt ) );
		}
		mStatus.emplace_back( std::move( rs ) );
	}

	for ( auto& s : status ) {
		for ( auto& fv : s.second ) {
			auto pos = repoPos[s.first];
			auto typePos = repoTypePos[pos][fv.report.type];
			DiffFile df( std::move( fv ), &mStatus[pos].type[typePos] );
			mStatus[pos].type[typePos].files.emplace_back( std::move( df ) );
		}
	}

	// Set the parents after the addreses are stable
	for ( auto& status : mStatus ) {
		for ( auto& type : status.type ) {
			for ( auto& f : type.files )
				f.parent = &type;
			type.parent = &status;
		}
	}
}

size_t GitStatusModel::rowCount( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return mStatus.size();

	if ( index.internalId() == Repo )
		return mStatus[index.row()].type.size();

	if ( index.internalId() == Status )
		return mStatus[index.parent().row()].type[index.row()].files.size();

	return 0;
}

ModelIndex GitStatusModel::parentIndex( const ModelIndex& index ) const {
	if ( !index.isValid() || index.internalId() == Repo )
		return {};

	if ( index.internalId() == Status ) {
		RepoStatusType* status = reinterpret_cast<RepoStatusType*>( index.internalData() );
		size_t f = 0;
		size_t statusSize = mStatus.size();
		for ( size_t i = 0; i < statusSize; i++ ) {
			if ( &mStatus[i] == status->parent ) {
				f = i;
				break;
			}
		}
		return createIndex( f, index.column(), status->parent, Repo );
	}

	if ( index.internalId() == GitFile ) {
		DiffFile* file = reinterpret_cast<DiffFile*>( index.internalData() );
		RepoStatusType* status = file->parent;
		RepoStatus* repoStatus = status->parent;
		size_t typeSize = repoStatus->type.size();
		size_t f = 0;
		for ( size_t i = 0; i < typeSize; i++ ) {
			if ( &status->parent->type[i] == status ) {
				f = i;
				break;
			}
		}
		return createIndex( f, index.column(), status, Status );
	}

	return {};
}

ModelIndex GitStatusModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( row < 0 || column < 0 )
		return {};

	if ( !parent.isValid() )
		return createIndex( row, column, &mStatus[row], Repo );

	if ( parent.internalId() == Repo )
		return createIndex( row, column, &mStatus[parent.row()].type[row], Status );

	if ( parent.internalId() == Status ) {
		size_t pprow = parent.parent().row();
		size_t prow = parent.row();
		return createIndex( row, column, &mStatus[pprow].type[prow].files[row], GitFile );
	}

	return {};
}

Variant GitStatusModel::data( const ModelIndex& index, ModelRole role ) const {
	switch ( role ) {
		case ModelRole::Display: {
			if ( index.internalId() == Repo ) {
				if ( index.column() == Column::File )
					return Variant( mStatus[index.row()].repo.c_str() );
				return Variant( GIT_EMPTY );
			} else if ( index.internalId() == Status ) {
				if ( index.column() == Column::File ) {
					return Variant(
						mStatus[index.parent().row()].type[index.row()].typeStr.c_str() );
				}
				return Variant( GIT_EMPTY );
			}
			const Git::DiffFile& s = mStatus[index.parent().parent().row()]
										 .type[index.parent().row()]
										 .files[index.row()];
			switch ( index.column() ) {
				case Column::File:
					return Variant( FileSystem::fileNameFromPath( s.file ) );
				case Column::Inserted:
					return Variant( String::format( "+%d ", s.inserts ) );
				case Column::Removed:
					return Variant( String::format( "-%d ", s.deletes ) );
				case Column::State:
					return Variant( String::format( "%c", s.report.symbol ) );
				case Column::RelativeDirectory:
					return Variant( FileSystem::fileRemoveFileName( s.file ) );
			}
			break;
		}
		case ModelRole::Class: {
			if ( index.internalId() == GitFile ) {
				switch ( index.column() ) {
					case Column::Inserted:
						return Variant( GIT_SUCCESS );
					case Column::Removed:
						return Variant( GIT_ERROR );
					default:
						break;
				}
			}
			break;
		}
		case ModelRole::Icon: {
			if ( (Int64)treeColumn() == index.column() ) {
				if ( index.internalId() == Repo ) {
					return Variant( mPlugin->findIcon( "repo" ) );
				} else if ( index.internalId() == GitFile ) {
					const Git::DiffFile& s = mStatus[index.parent().parent().row()]
												 .type[index.parent().row()]
												 .files[index.row()];
					std::string iconName = UIIconThemeManager::getIconNameFromFileName( s.file );
					auto* scene = mPlugin->getUISceneNode();
					auto* d = scene->findIcon( iconName );
					if ( !d )
						return scene->findIcon( "file" );
					return d;
				}
			}
			break;
		}
		default:
			break;
	}
	return {};
}

const GitStatusModel::RepoStatus* GitStatusModel::repo( const ModelIndex& index ) const {
	if ( index.internalId() != Repo )
		return nullptr;
	return &mStatus[index.row()];
}

const GitStatusModel::RepoStatusType* GitStatusModel::statusType( const ModelIndex& index ) const {
	if ( index.internalId() != Status )
		return nullptr;
	return &mStatus[index.parent().row()].type[index.row()];
}

const GitStatusModel::DiffFile* GitStatusModel::file( const ModelIndex& index ) const {
	if ( index.internalId() != GitFile )
		return nullptr;
	return &mStatus[index.parent().parent().row()].type[index.parent().row()].files[index.row()];
}

std::vector<std::string> GitStatusModel::getFiles( const std::string& repo,
												   uint32_t statusType ) const {
	std::vector<std::string> files;
	for ( const auto& status : mStatus ) {
		if ( status.repo == repo ) {
			for ( const auto& type : status.type ) {
				if ( static_cast<uint32_t>( type.type ) & statusType ) {
					for ( const auto& file : type.files )
						files.push_back( file.file );
				}
			}
			break;
		}
	}
	return files;
}

} // namespace ecode
