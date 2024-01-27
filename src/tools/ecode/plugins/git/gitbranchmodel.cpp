#include "gitbranchmodel.hpp"
#include "gitplugin.hpp"

namespace ecode {

size_t GitBranchModel::hashBranches( const std::vector<Git::Branch>& branches ) {
	size_t hash = 0;
	for ( const auto& branch : branches )
		hash = hashCombine( hash, String::hash( branch.name ), String::hash( branch.remote ),
							String::hash( branch.lastCommit ), branch.type, branch.ahead,
							branch.behind );
	return hash;
}

std::string GitBranchModel::refTypeToString( Git::RefType type ) {
	switch ( type ) {
		case Git::RefType::Head:
			return mPlugin->i18n( "git_local_branches", "Local Branches" ).toUtf8();
		case Git::RefType::Remote:
			return mPlugin->i18n( "git_remote_branches", "Remote Branches" ).toUtf8();
		case Git::RefType::Tag:
			return mPlugin->i18n( "git_tags", "Tags" ).toUtf8();
		default:
			break;
	}
	return "";
}

GitBranchModel::GitBranchModel( std::vector<Git::Branch>&& branches, size_t hash,
								GitPlugin* gitPlugin ) :
	mPlugin( gitPlugin ), mHash( hash ) {
	std::map<std::string, std::vector<Git::Branch>> branchTypes;
	for ( auto& branch : branches ) {
		auto& type = branchTypes[refTypeToString( branch.type )];
		type.emplace_back( std::move( branch ) );
	}
	for ( auto& branch : branchTypes ) {
		mBranches.emplace_back(
			BranchData{ std::move( branch.first ), std::move( branch.second ) } );
	}
}

size_t GitBranchModel::rowCount( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return mBranches.size();
	if ( index.internalId() == -1 )
		return mBranches[index.row()].data.size();
	return 0;
}

ModelIndex GitBranchModel::parentIndex( const ModelIndex& index ) const {
	if ( !index.isValid() || index.internalId() == -1 )
		return {};
	return createIndex( index.internalId(), index.column(), &mBranches[index.internalId()], -1 );
}

ModelIndex GitBranchModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( row < 0 || column < 0 )
		return {};
	if ( !parent.isValid() )
		return createIndex( row, column, &mBranches[row], -1 );
	if ( parent.internalData() )
		return createIndex( row, column, &mBranches[parent.row()].data[row], parent.row() );
	return {};
}

UIIcon* GitBranchModel::iconFor( const ModelIndex& index ) const {
	if ( index.column() == (Int64)treeColumn() ) {
		if ( index.hasParent() ) {
			Git::Branch* branch = static_cast<Git::Branch*>( index.internalData() );
			return mPlugin->findIcon( branch->type == Git::RefType::Tag ? GIT_TAG : GIT_REPO );
		}
	}
	return nullptr;
}

Variant GitBranchModel::data( const ModelIndex& index, ModelRole role ) const {
	switch ( role ) {
		case ModelRole::Display: {
			if ( index.internalId() == -1 ) {
				if ( index.column() == Column::Name )
					return Variant( String::format( "%s (%zu)", mBranches[index.row()].branch,
													mBranches[index.row()].data.size() ) );
				return Variant( GIT_EMPTY );
			}
			const Git::Branch& branch = mBranches[index.internalId()].data[index.row()];
			switch ( index.column() ) {
				case Column::Name: {
					if ( branch.type == Git::Remote &&
						 String::startsWith( branch.name, "origin/" ) ) {
						return Variant( std::string_view{ branch.name }.substr( 7 ).data() );
					} else if ( branch.type == Git::Head && ( branch.ahead || branch.behind ) ) {
						if ( branch.ahead && branch.behind ) {
							return Variant( String::format( "%s (+%ld/-%ld)", branch.name,
															branch.ahead, branch.behind ) );
						} else if ( branch.ahead ) {
							return Variant(
								String::format( "%s (+%ld)", branch.name, branch.ahead ) );
						} else {
							return Variant(
								String::format( "%s (-%ld)", branch.name, branch.behind ) );
						}
					}
					return Variant( branch.name.c_str() );
				}
				case Column::Remote:
					return Variant( branch.remote.c_str() );
				case Column::Type:
					return Variant( branch.typeStr() );
				case Column::LastCommit:
					return Variant( branch.lastCommit.c_str() );
			}
			return Variant( GIT_EMPTY );
		}
		case ModelRole::Class: {
			if ( index.internalId() == -1 )
				return Variant( GIT_BOLD );
			const Git::Branch& branch = mBranches[index.internalId()].data[index.row()];
			if ( branch.name == mPlugin->gitBranch() )
				return Variant( GIT_BOLD );
			return Variant( GIT_NOT_BOLD );
		}
		case ModelRole::Icon: {
			return iconFor( index );
		}
		default:
			break;
	}
	return {};
}

Git::Branch GitBranchModel::branch( const ModelIndex& index ) const {
	return *static_cast<Git::Branch*>( index.internalData() );
}

Git::Branch GitBranchModel::branch( const std::string& name ) const {
	for ( const auto& type : mBranches ) {
		for ( const auto& branch : type.data ) {
			if ( branch.name == name )
				return branch;
		}
	}
	return {};
}

} // namespace ecode
