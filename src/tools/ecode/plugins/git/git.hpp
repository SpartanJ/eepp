#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace ecode {

#define git_xy( x, y ) ( ( (uint16_t)x ) << 8 | y )

class Git {
  public:
	struct Blame {
		Blame( const std::string& error );

		Blame( std::string&& author, std::string&& authorEmail, std::string&& date,
			   std::string&& commitHash, std::string&& commitShortHash, std::string&& commitMessage,
			   std::size_t line );

		std::string author;
		std::string authorEmail;
		std::string date;
		std::string commitHash;
		std::string commitShortHash;
		std::string commitMessage;
		std::string error;
		std::size_t line{ 0 };
	};

	enum class GitStatusChar : char {
		Unknown = ' ',
		Modified = 'M',
		Added = 'A',
		Renamed = 'R',
		TypeChanged = 'T',
		UpdatedUnmerged = 'U',
		Deleted = 'D',
		Untracked = '?',
		Ignored = 'I',
		Copied = 'C',
		ModifiedSubmodule = 'm',
	};

	enum class GitStatus {
		Unmerge_BothDeleted,
		Unmerge_AddedByUs,
		Unmerge_DeletedByThem,
		Unmerge_AddedByThem,
		Unmerge_DeletedByUs,
		Unmerge_BothAdded,
		Unmerge_BothModified,

		Index_Modified,
		Index_Added,
		Index_Deleted,
		Index_Renamed,
		Index_Copied,
		Index_ModifiedSubmodule,

		WorkingTree_Modified,
		WorkingTree_Deleted,
		WorkingTree_IntentToAdd,
		WorkingTree_ModifiedSubmodule,

		Untracked,
		Ignored,
		NotSet
	};

	enum class GitStatusType {
		Staged,
		Changed,
		Untracked,
		Unmerged,
		Ignored,
	};

	static constexpr auto ANY = 0;

	enum StatusXY : uint16_t {
		A = git_xy( ' ', 'A' ),	  // modified, added not updated
		M = git_xy( ' ', 'M' ),	  // modified, modified not updated
		D = git_xy( ' ', 'D' ),	  // modified, deleted not updated
		m = git_xy( ' ', 'm' ),	  // modified, modified submodule not updated
		DD = git_xy( 'D', 'D' ),  // unmerged, both deleted
		AU = git_xy( 'A', 'U' ),  // unmerged, added by us
		UD = git_xy( 'U', 'D' ),  // unmerged, deleted by them
		UA = git_xy( 'U', 'A' ),  // unmerged, added by them
		DU = git_xy( 'D', 'U' ),  // unmerged, deleted by us
		AA = git_xy( 'A', 'A' ),  // unmerged, both added
		UU = git_xy( 'U', 'U' ),  // unmerged, both modified
		QQ = git_xy( '?', '?' ),  // untracked
		II = git_xy( '!', '!' ),  // ignored
		SM = git_xy( 'M', ANY ),  // staged modified
		ST = git_xy( 'T', ANY ),  // staged type changed
		SA = git_xy( 'A', ANY ),  // staged added
		SD = git_xy( 'D', ANY ),  // staged deleted
		SR = git_xy( 'R', ANY ),  // staged renamed
		SC = git_xy( 'C', ANY ),  // staged copied
		SMM = git_xy( 'm', ANY ), // staged modified submodule
	};

	struct GitStatusReport {
		GitStatus status = GitStatus::NotSet;
		GitStatusType type = GitStatusType::Untracked;
		GitStatusChar symbol = GitStatusChar::Unknown;

		bool operator==( const GitStatusReport& other ) const {
			return status == other.status && symbol == other.symbol && type == other.type;
		}
	};

	struct DiffFile {
		std::string file;
		int inserts{ 0 };
		int deletes{ 0 };
		GitStatusReport report;

		bool operator==( const DiffFile& other ) const {
			return file == other.file && inserts == other.inserts && deletes == other.deletes &&
				   report == other.report;
		}
	};

	using RepositoryName = std::string;
	using FilesStatus = std::map<RepositoryName, std::vector<DiffFile>>;

	struct Status {
		int totalInserts{ 0 };
		int totalDeletions{ 0 };
		FilesStatus files;

		bool operator==( const Status& other ) const {
			return totalInserts == other.totalInserts && totalDeletions == other.totalDeletions &&
				   files == other.files;
		}
	};

	struct Result {
		std::string result;
		int returnCode = 0;

		bool success() const { return returnCode == EXIT_SUCCESS; }

		bool fail() const { return !success(); }
	};

	struct CheckoutResult : public Result {
		std::string branch;
	};

	struct CountResult : public Result {
		int64_t behind{ 0 };
		int64_t ahead{ 0 };
	};

	enum RefType {
		Head = 0x1,
		Remote = 0x2,
		Tag = 0x4,
		All = 0x7,
	};

	static constexpr const char* HEAD = "head";
	static constexpr const char* REMOTE = "remote";
	static constexpr const char* TAG = "tag";
	static constexpr const char* ALL = "all";

	static const char* refTypeToString( RefType type ) {
		switch ( type ) {
			case Head:
				return HEAD;
			case Remote:
				return REMOTE;
			case Tag:
				return TAG;
			case All:
				return ALL;
		}
		return nullptr;
	}
	struct Branch {
		/** Branch name */
		std::string name;
		/** remote name, will be empty for local branches */
		std::string remote;
		/** Ref type @see RefType */
		RefType type = All;
		/** last commit on this branch, may be empty **/
		std::string lastCommit;
		/** if it's HEAD how much ahead and behind the current local branch is against remote */
		int64_t ahead{ 0 };
		int64_t behind{ 0 };

		const char* typeStr() const { return refTypeToString( type ); }
	};

	Git( const std::string& projectDir = "", const std::string& gitPath = "" );

	int git( const std::string& args, const std::string& projectDir, std::string& buf ) const;

	void gitSubmodules( const std::string& args, const std::string& projectDir, std::string& buf );

	bool isGitRepo( const std::string& projectDir );

	Blame blame( const std::string& filepath, std::size_t line ) const;

	std::string branch( const std::string& projectDir = "" );

	std::unordered_map<std::string, std::string> branches( const std::vector<std::string>& repos );

	Status status( bool recurseSubmodules, const std::string& projectDir = "" );

	Result add( std::vector<std::string> files, const std::string& projectDir = "" );

	Result restore( const std::string& file, const std::string& projectDir = "" );

	Result reset( std::vector<std::string> files, const std::string& projectDir = "" );

	Result createBranch( const std::string& branchName, bool checkout = false,
						 const std::string& projectDir = "" );

	Result renameBranch( const std::string& branch, const std::string& newName,
						 const std::string& projectDir = "" );

	Result deleteBranch( const std::string& branch, const std::string& projectDir = "" );

	Result commit( const std::string& commitMsg, const std::string& projectDir = "" );

	Result fetch( const std::string& projectDir = "" );

	Result fastForwardMerge( const std::string& projectDir = "" );

	Result updateRef( const std::string& headBranch, const std::string& toCommit,
					  const std::string& projectDir = "" );

	CountResult branchHistoryPosition( const std::string& localBranch,
									   const std::string& remoteBranch,
									   const std::string& projectDir = "" );

	CountResult branchHistoryPosition( const Git::Branch& branch,
									   const std::string& projectDir = "" );

	bool setProjectPath( const std::string& projectPath );

	const std::string& getGitPath() const;

	const std::string& getProjectPath() const;

	const std::string& getGitFolder() const;

	std::string setSafeDirectory( const std::string& projectDir ) const;

	Result pull( const std::string& projectDir = "" );

	Result push( const std::string& projectDir = "" );

	CheckoutResult checkout( const std::string& branch, const std::string& projectDir = "" ) const;

	CheckoutResult checkoutAndCreateLocalBranch( const std::string& remoteBranch,
												 const std::string& newBranch = "",
												 const std::string& projectDir = "" ) const;

	CheckoutResult checkoutNewBranch( const std::string& newBranch,
									  const std::string& fromBranch = "",
									  const std::string& projectDir = "" );

	/**
	 * @brief get all local and remote branches
	 */
	std::vector<Branch> getAllBranches( const std::string& projectDir = "" );

	/**
	 * @brief get all local and remote branches + tags
	 */
	std::vector<Branch> getAllBranchesAndTags( RefType ref = RefType::All,
											   std::string_view filterBranch = {},
											   const std::string& projectDir = "" );

	std::vector<std::string> fetchSubModules( const std::string& projectDir );

	std::vector<std::string> getSubModules( const std::string& projectDir = "" );

	std::string repoName( const std::string& file, const std::string& projectDir = "" );

	std::string repoPath( const std::string& file );

	bool hasSubmodules( const std::string& projectDir );

	Result gitSimple( const std::string& cmd, const std::string& projectDir );

	GitStatusReport statusFromShortStatusStr( const std::string_view& statusStr );

  protected:
	std::string mGitPath;
	std::string mProjectPath;
	std::string mGitFolder;
	std::vector<std::string> mSubModules;
	bool mSubModulesUpdated{ false };
};

} // namespace ecode
