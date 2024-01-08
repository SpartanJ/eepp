#include <map>
#include <string>

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

	enum class FileStatus {
		Unknown,
		Modified = 'M',
		Added = 'A',
		Renamed = 'R',
		TypeChanged = 'T',
		UpdatedUnmerged = 'U',
		Deleted = 'D',
		Untracked = '?',
		ModifiedSubmodule = 'm',
	};
	struct DiffFile {
		std::string file;
		int inserts{ 0 };
		int deletes{ 0 };
		FileStatus status{ FileStatus::Unknown };

		bool operator==( const DiffFile& other ) const {
			return file == other.file && inserts == other.inserts && deletes == other.deletes;
		}
	};

	struct Status {
		int totalInserts{ 0 };
		int totalDeletions{ 0 };
		std::map<std::string, DiffFile> files;

		bool operator==( const Status& other ) const {
			return totalInserts == other.totalInserts && totalDeletions == other.totalDeletions &&
				   files == other.files;
		}
	};

	struct Result {
		std::string error;
		int returnCode = 0;
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

	struct Branch {
		/** Branch name */
		std::string name;
		/** remote name, will be empty for local branches */
		std::string remote;
		/** Ref type @see RefType */
		RefType type = All;
		/** last commit on this branch, may be empty **/
		std::string lastCommit;
	};

	Git( const std::string& projectDir = "", const std::string& gitPath = "" );

	int git( const std::string& args, const std::string& projectDir, std::string& buf ) const;

	void gitSubmodules( const std::string& args, const std::string& projectDir, std::string& buf );

	bool isGitRepo( const std::string& projectDir );

	Blame blame( const std::string& filepath, std::size_t line ) const;

	std::string branch( const std::string& projectDir = "" );

	Status status( bool recurseSubmodules, const std::string& projectDir = "" );

	bool setProjectPath( const std::string& projectPath );

	const std::string& getGitPath() const;

	const std::string& getProjectPath() const;

	const std::string& getGitFolder() const;

	std::string setSafeDirectory( const std::string& projectDir ) const;

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

  protected:
	std::string mGitPath;
	std::string mProjectPath;
	std::string mGitFolder;

	bool hasSubmodules( const std::string& projectDir );
};

} // namespace ecode
