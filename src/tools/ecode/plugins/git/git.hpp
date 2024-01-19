#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ecode {

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

	static constexpr uint16_t xy( char x, char y ) { return ( (uint16_t)x ) << 8 | y; }

	static constexpr uint16_t x( uint16_t val ) { return ( val >> 8 ) & 0xFF; }

	static constexpr uint16_t y( uint16_t val ) { return val & 0xFF; }

	static constexpr auto ANY = 0;

#define _xy( x, y ) ( ( (uint16_t)x ) << 8 | y )

	enum StatusXY : uint16_t {
		A = _xy( ' ', 'A' ),   // modified, added not updated
		M = _xy( ' ', 'M' ),   // modified, modified not updated
		D = _xy( ' ', 'D' ),   // modified, deleted not updated
		m = _xy( ' ', 'm' ),   // modified, modified submodule not updated
		DD = _xy( 'D', 'D' ),  // unmerged, both deleted
		AU = _xy( 'A', 'U' ),  // unmerged, added by us
		UD = _xy( 'U', 'D' ),  // unmerged, deleted by them
		UA = _xy( 'U', 'A' ),  // unmerged, added by them
		DU = _xy( 'D', 'U' ),  // unmerged, deleted by us
		AA = _xy( 'A', 'A' ),  // unmerged, both added
		UU = _xy( 'U', 'U' ),  // unmerged, both modified
		QQ = _xy( '?', '?' ),  // untracked
		II = _xy( '!', '!' ),  // ignored
		SM = _xy( 'M', ANY ),  // staged modified
		ST = _xy( 'T', ANY ),  // staged type changed
		SA = _xy( 'A', ANY ),  // staged added
		SD = _xy( 'D', ANY ),  // staged deleted
		SR = _xy( 'R', ANY ),  // staged renamed
		SC = _xy( 'C', ANY ),  // staged copied
		SMM = _xy( 'm', ANY ), // staged modified submodule
	};

	struct DiffFile {
		std::string file;
		int inserts{ 0 };
		int deletes{ 0 };
		GitStatus status;
		GitStatusType statusType;
		GitStatusChar statusChar{ GitStatusChar::Unknown };

		bool operator==( const DiffFile& other ) const {
			return file == other.file && inserts == other.inserts && deletes == other.deletes &&
				   status == other.status && statusType == other.statusType &&
				   statusChar == other.statusChar;
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

		const char* typeStr() const { return refTypeToString( type ); }
	};

	Git( const std::string& projectDir = "", const std::string& gitPath = "" );

	int git( const std::string& args, const std::string& projectDir, std::string& buf ) const;

	void gitSubmodules( const std::string& args, const std::string& projectDir, std::string& buf );

	bool isGitRepo( const std::string& projectDir );

	Blame blame( const std::string& filepath, std::size_t line ) const;

	std::string branch( const std::string& projectDir = "" );

	Status status( bool recurseSubmodules, const std::string& projectDir = "" );

	Result add( const std::string& file, const std::string& projectDir = "" );

	Result restore( const std::string& file, const std::string& projectDir = "" );

	Result reset( const std::string& file, const std::string& projectDir = "" );

	bool setProjectPath( const std::string& projectPath );

	const std::string& getGitPath() const;

	const std::string& getProjectPath() const;

	const std::string& getGitFolder() const;

	std::string setSafeDirectory( const std::string& projectDir ) const;

	Result pull( const std::string& projectDir = "" );

	CheckoutResult checkout( const std::string& branch, const std::string& projectDir = "" ) const;

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
											   const std::string& projectDir = "" );

	std::vector<std::string> fetchSubModules( const std::string& projectDir );

	std::vector<std::string> getSubModules( const std::string& projectDir = "" );

  protected:
	std::string mGitPath;
	std::string mProjectPath;
	std::string mGitFolder;
	std::string mLastProjectPath;
	std::vector<std::string> mSubModules;
	bool mSubModulesUpdated{ false };

	bool hasSubmodules( const std::string& projectDir );

	std::string inSubModule( const std::string& file, const std::string& projectDir );
};

} // namespace ecode
