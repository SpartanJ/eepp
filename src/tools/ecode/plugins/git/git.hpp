#include <string>
#include <vector>

namespace ecode {

class Git {
  public:
	struct BlameData {
		BlameData( const std::string& error );

		BlameData( std::string&& author, std::string&& authorEmail, std::string&& date,
				   std::string&& commitHash, std::string&& commitShortHash,
				   std::string&& commitMessage );

		std::string author;
		std::string authorEmail;
		std::string date;
		std::string commitHash;
		std::string commitShortHash;
		std::string commitMessage;
		std::string error;
	};

	struct DiffFile {
		std::string file;
		int inserts{ 0 };
		int deletes{ 0 };
	};

	struct Status {
		std::vector<DiffFile> modified;
		int totalInserts{ 0 };
		int totalDeletions{ 0 };
	};

	Git( const std::string& projectDir = "", const std::string& gitPath = "" );

	void git( const std::string& args, const std::string& projectDir, std::string& buf ) const;

	BlameData blame( const std::string& filepath, std::size_t line ) const;

	std::string branch( std::string projectDir = "" );

	Status status( std::string projectDir = "" );

	bool setProjectPath( std::string projectPath );

	const std::string& getGitPath() const { return mGitPath; }

	const std::string& getProjectPath() const { return mProjectPath; }

  protected:
	std::string mGitPath;
	std::string mProjectPath;
};

} // namespace ecode
