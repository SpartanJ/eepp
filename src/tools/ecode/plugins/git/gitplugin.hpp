#ifndef ECODE_GITPLUGIN_HPP
#define ECODE_GITPLUGIN_HPP

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "git.hpp"
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <optional>

using namespace EE::UI::Models;
using namespace EE::UI;

namespace ecode {

class Git;

class GitPlugin : public PluginBase {
  public:
	static PluginDefinition Definition() {
		return { "git", "Git", "Git integration", GitPlugin::New, { 0, 0, 1 }, GitPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~GitPlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

	void onFileSystemEvent( const FileEvent& ev, const FileInfo& file ) override;

	void onRegister( UICodeEditor* ) override;

	void onUnregister( UICodeEditor* ) override;

	bool onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu, const Vector2i& position,
							  const Uint32& flags ) override;

	bool onKeyDown( UICodeEditor*, const KeyEvent& ) override;

	bool onMouseLeave( UICodeEditor*, const Vector2i&, const Uint32& ) override;

  protected:
	std::unique_ptr<Git> mGit;
	std::string mGitBranch;
	Git::Status mGitStatus;
	UILinearLayout* mStatusBar{ nullptr };
	UIPushButton* mStatusButton{ nullptr };
	Time mRefreshFreq{ Seconds( 5 ) };

	GitPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	void displayTooltip( UICodeEditor* editor, const Git::Blame& blame, const Vector2f& position );

	void hideTooltip( UICodeEditor* editor );

	void onRegisterListeners( UICodeEditor*, std::vector<Uint32>& listeners ) override;

	void onBeforeUnregister( UICodeEditor* ) override;

	void onUnregisterDocument( TextDocument* ) override;

	bool mGitFound{ false };
	bool mTooltipInfoShowing{ false };
	bool mStatusBarDisplayBranch{ true };
	bool mStatusBarDisplayModifications{ true };
	bool mStatusRecurseSubmodules{ true };
	bool mOldDontAutoHideOnMouseMove{ false };
	bool mOldUsingCustomStyling{ false };
	Uint32 mOldTextStyle{ 0 };
	Uint32 mOldTextAlign{ 0 };
	Color mOldBackgroundColor;

	struct CustomTokenizer {
		SyntaxDefinition def;
		SyntaxColorScheme scheme;
	};
	std::optional<CustomTokenizer> mStatusCustomTokenizer;
	std::optional<SyntaxDefinition> mTooltipCustomSyntaxDef;

	Color getVarColor( const std::string& var );

	void blame( UICodeEditor* editor );

	void updateStatusBar( bool force = false );

	void updateStatusBarSync();

	void updateUI();

	void updateUINow( bool force = false );
};

class GitBranchModel : public Model {
  public:
	static std::shared_ptr<GitBranchModel> asModel( std::vector<Git::Branch>&& branches ) {
		return std::make_shared<GitBranchModel>( std::move( branches ) );
	}

	enum Column { Name, Remote, Type, LastCommit };

	struct BranchData {
		std::string branch;
		std::vector<Git::Branch> data;
	};

	GitBranchModel( std::vector<Git::Branch>&& branches ) {
		std::map<std::string, std::vector<Git::Branch>> branchTypes;
		for ( auto& branch : branches ) {
			auto& type = branchTypes[Git::refTypeToString( branch.type )];
			type.emplace_back( std::move( branch ) );
		}
		for ( auto& branch : branchTypes ) {
			mBranches.emplace_back(
				BranchData{ std::move( branch.first ), std::move( branch.second ) } );
		}
	}

	size_t treeColumn() const { return Column::Name; }

	size_t rowCount( const ModelIndex& index ) const {
		if ( !index.isValid() )
			return mBranches.size();
		return mBranches[index.row()].data.size();
	}

	size_t columnCount( const ModelIndex& ) const { return 4; }

	ModelIndex parentIndex( const ModelIndex& index ) const {
		if ( !index.isValid() || index.internalId() == -1 )
			return {};
		return createIndex( index.internalId(), index.column(), &mBranches[index.internalId()],
							-1 );
	}

	ModelIndex index( int row, int column, const ModelIndex& parent ) const {
		if ( row < 0 || column < 0 )
			return {};
		if ( !parent.isValid() )
			return createIndex( row, column, &mBranches[row], -1 );
		if ( parent.internalData() )
			return createIndex( row, column, &mBranches[parent.row()].data[row], parent.row() );
		return {};
	}

	Variant data( const ModelIndex& index, ModelRole role ) const {
		static const char* EMPTY = "";
		if ( role == ModelRole::Display ) {
			if ( index.internalId() == -1 ) {
				if ( index.column() == Column::Name )
					return mBranches[index.row()].branch.c_str();
				return EMPTY;
			}
			const Git::Branch& branch = mBranches[index.internalId()].data[index.row()];
			switch ( index.column() ) {
				case Column::Name:
					return branch.name.c_str();
				case Column::Remote:
					return branch.remote.c_str();
				case Column::Type:
					return branch.typeStr();
				case Column::LastCommit:
					return branch.lastCommit.c_str();
			}
		}
		return EMPTY;
	}

  protected:
	std::vector<BranchData> mBranches;
};

class GitStatusModel : public Model {
  public:
	static std::shared_ptr<GitStatusModel> asModel( Git::FilesStatus&& status ) {
		return std::make_shared<GitStatusModel>( std::move( status ) );
	}

	struct RepoStatus {
		std::string repo;
		std::vector<Git::DiffFile> files;
	};

	enum Column { File, Inserted, Removed, FileStatus };

	GitStatusModel( Git::FilesStatus&& status ) {
		mStatus.reserve( status.size() );
		for ( auto& s : status )
			mStatus.emplace_back( RepoStatus{ std::move( s.first ), std::move( s.second ) } );
	}

	size_t treeColumn() const { return Column::File; }

	size_t rowCount( const ModelIndex& index ) const {
		if ( !index.isValid() )
			return mStatus.size();
		return mStatus[index.row()].files.size();
	}

	size_t columnCount( const ModelIndex& ) const { return 4; }

	ModelIndex parentIndex( const ModelIndex& index ) const {
		if ( !index.isValid() || index.internalId() == -1 )
			return {};
		return createIndex( index.internalId(), index.column(), &mStatus[index.internalId()], -1 );
	}

	ModelIndex index( int row, int column, const ModelIndex& parent ) const {
		if ( row < 0 || column < 0 )
			return {};
		if ( !parent.isValid() )
			return createIndex( row, column, &mStatus[row], -1 );
		if ( parent.internalData() )
			return createIndex( row, column, &mStatus[parent.row()].files[row], parent.row() );
		return {};
	}

	Variant data( const ModelIndex& index, ModelRole role ) const {
		static const char* EMPTY = "";
		static const char* SUCCESS = "theme-success";
		static const char* ERROR = "theme-error";
		if ( role == ModelRole::Display ) {
			if ( index.internalId() == -1 ) {
				if ( index.column() == Column::File )
					return mStatus[index.row()].repo.c_str();
				return EMPTY;
			}
			const Git::DiffFile& s = mStatus[index.internalId()].files[index.row()];
			switch ( index.column() ) {
				case Column::File:
					return s.file.c_str();
				case Column::Inserted:
					return s.inserts;
				case Column::Removed:
					return s.deletes;
				case Column::FileStatus:
					return Variant( std::string( static_cast<char>( s.status ), 1 ) );
			}
		} else if ( role == ModelRole::Class ) {
			switch ( index.column() ) {
				case Column::Inserted:
					return SUCCESS;
				case Column::Removed:
					return ERROR;
				default:
					break;
			}
		}
		return EMPTY;
	}

	virtual bool classModelRoleEnabled() { return true; }

  protected:
	std::vector<RepoStatus> mStatus;
};

} // namespace ecode

#endif // ECODE_GITPLUGIN_HPP
