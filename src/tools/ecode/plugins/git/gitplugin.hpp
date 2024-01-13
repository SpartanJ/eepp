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
	enum Column { Name, Remote, Type, LastCommit };

	GitBranchModel( std::vector<Git::Branch>&& branches ) : mBranches( std::move( branches ) ) {}

	size_t rowCount( const ModelIndex& ) const { return mBranches.size(); }

	size_t columnCount( const ModelIndex& ) const { return 4; }

	Variant data( const ModelIndex& index, ModelRole role ) const {
		if ( role == ModelRole::Display && index.row() < mBranches.size() ) {
			const Git::Branch& branch = mBranches[index.row()];
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
		return {};
	}

  protected:
	std::vector<Git::Branch> mBranches;
};

class GitStatusModel : public Model {
  public:
	enum Column { File, Inserted, Removed, FileStatus };

	GitStatusModel( Git::FilesStatus&& status ) {
		mStatus.reserve( status.size() );
		for ( auto& s : status )
			mStatus.emplace_back( std::move( s.second ) );
	}

	size_t rowCount( const ModelIndex& ) const { return mStatus.size(); }

	size_t columnCount( const ModelIndex& ) const { return 4; }

	Variant data( const ModelIndex& index, ModelRole role ) const {
		if ( role == ModelRole::Display && index.row() < mStatus.size() ) {
			const Git::DiffFile& s = mStatus[index.row()];
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
		}
		return {};
	}

  protected:
	std::vector<Git::DiffFile> mStatus;
};

} // namespace ecode

#endif // ECODE_GITPLUGIN_HPP
