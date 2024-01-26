#ifndef ECODE_GITSTATUSMODEL_HPP
#define ECODE_GITSTATUSMODEL_HPP

#include "git.hpp"
#include <eepp/ui/models/model.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace ecode {

class GitPlugin;

class GitStatusModel : public Model {
  public:
	static std::shared_ptr<GitStatusModel> asModel( Git::FilesStatus status,
													GitPlugin* gitPlugin ) {
		return std::make_shared<GitStatusModel>( std::move( status ), gitPlugin );
	}

	struct RepoStatusType;
	struct RepoStatus;

	struct DiffFile : Git::DiffFile {
		DiffFile( Git::DiffFile&& df, RepoStatusType* parent ) :
			Git::DiffFile( df ), parent( parent ){};
		RepoStatusType* parent;
	};

	struct RepoStatusType {
		std::string typeStr;
		Git::GitStatusType type;
		std::vector<DiffFile> files;
		RepoStatus* parent{ nullptr };
	};

	struct RepoStatus {
		std::string repo;
		std::vector<RepoStatusType> type;
	};

	enum Column { File, State, Inserted, Removed, RelativeDirectory };

	GitStatusModel( Git::FilesStatus&& status, GitPlugin* gitPlugin );

	size_t treeColumn() const { return Column::File; }

	size_t rowCount( const ModelIndex& index ) const;

	size_t columnCount( const ModelIndex& ) const { return 5; }

	ModelIndex parentIndex( const ModelIndex& index ) const;

	enum ModelCategory { Repo, Status, GitFile };

	ModelIndex index( int row, int column, const ModelIndex& parent ) const;

	Variant data( const ModelIndex& index, ModelRole role ) const;

	virtual bool classModelRoleEnabled() { return true; }

	const RepoStatus* repo( const ModelIndex& index ) const;

	const RepoStatusType* statusType( const ModelIndex& index ) const;

	const DiffFile* file( const ModelIndex& index ) const;

	std::vector<std::string> getFiles( const std::string& repo, uint32_t statusType ) const;

  protected:
	std::vector<RepoStatus> mStatus;
	GitPlugin* mPlugin{ nullptr };
};

} // namespace ecode

#endif // ECODE_GITSTATUSMODEL_HPP
