#ifndef ECODE_GITBRANCHMODEL_HPP
#define ECODE_GITBRANCHMODEL_HPP

#include "git.hpp"

#include <cstddef>

#include <eepp/ui/models/model.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace ecode {

class GitPlugin;

class GitBranchModel : public Model {
  public:
	static std::shared_ptr<GitBranchModel> asModel( std::vector<Git::Branch>&& branches,
													size_t hash, GitPlugin* gitPlugin ) {
		return std::make_shared<GitBranchModel>( std::move( branches ), hash, gitPlugin );
	}

	static size_t hashBranches( const std::vector<Git::Branch>& branches );

	enum Column { Name, Remote, Type, LastCommit };

	struct BranchData {
		std::string branch;
		std::vector<Git::Branch> data;
	};

	std::string refTypeToString( Git::RefType type );

	GitBranchModel( std::vector<Git::Branch>&& branches, size_t hash, GitPlugin* gitPlugin );

	size_t treeColumn() const { return Column::Name; }

	size_t rowCount( const ModelIndex& index ) const;

	size_t columnCount( const ModelIndex& ) const { return 4; }

	ModelIndex parentIndex( const ModelIndex& index ) const;

	ModelIndex index( int row, int column, const ModelIndex& parent ) const;

	UIIcon* iconFor( const ModelIndex& index ) const;

	Variant data( const ModelIndex& index, ModelRole role ) const;

	virtual bool classModelRoleEnabled() { return true; }

	size_t getHash() const { return mHash; }

	Git::Branch branch( const ModelIndex& index ) const;

  protected:
	std::vector<BranchData> mBranches;
	GitPlugin* mPlugin{ nullptr };
	size_t mHash{ 0 };
};

} // namespace ecode

#endif // ECODE_GITBRANCHMODEL_HPP
