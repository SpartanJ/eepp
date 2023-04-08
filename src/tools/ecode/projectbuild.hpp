#ifndef ECODE_PROJECTBUILD_HPP
#define ECODE_PROJECTBUILD_HPP

#include <eepp/system/threadpool.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace EE;
using namespace EE::System;

namespace ecode {

/** reference:
{
  "ecode": {
	"build": [
	  {
		"args": "--with-mojoal --with-debug-symbols gmake",
		"command": "premake4",
		"working_dir": "${project_root}"
	  },
	  {
		"args": "-j${nproc} config=${build_type} ecode",
		"command": "make",
		"working_dir": "${build_dir}"
	  }
	],
	"build_types": [
	  "debug",
	  "release"
	],
	"clean": [
	  {
		"args": "config=${build_type} clean",
		"command": "make",
		"working_dir": "${build_dir}"
	  }
	],
	"config": {
	  "clear_sys_env": false,
	  "enabled": true
	},
	"env": {
	  "SHELL": "fish"
	},
	"os": [
	  "linux"
	],
	"output_parser": {
	  "config": {
		"relative_file_paths": true
	  },
	  "error": [
		{
		  "pattern": "([^:]*):(%d+):(%d+):%s?[%w%s]*error:%s?([^\n]*)",
		  "pattern_order": {
			"col": 3,
			"file": 1,
			"line": 2,
			"message": 4
		  }
		}
	  ]
	},
	"var": {
	  "build_dir": "${project_root}/make/${os}"
	}
  }
}
*/

struct ProjectBuildStep {
	std::string cmd;
	std::string args;
	std::string workingDir;
};

using ProjectBuildSteps = std::vector<ProjectBuildStep>;
using ProjectBuildKeyVal = std::unordered_map<std::string, std::string>;

struct ProjectBuildConfig {
	bool enabled{ true };
	bool clearSysEnv{ false };
};

enum class ProjectOutputParserTypes { Error, Warning, Notice };

struct ProjectBuildOutputParserConfig {
	ProjectOutputParserTypes type;
	std::string pattern;
	struct {
		int file{ 1 };
		int line{ 2 };
		int col{ 3 };
		int message{ 4 };
	} patternOrder;
};

class ProjectBuildOutputParser {
  public:
	const std::vector<ProjectBuildOutputParserConfig>& getConfig() const { return mConfig; }

	bool useRelativeFilePaths() const { return mRelativeFilePaths; }

  protected:
	friend class ProjectBuildManager;

	bool mRelativeFilePaths{ true };
	std::vector<ProjectBuildOutputParserConfig> mConfig;
};

class ProjectBuild {
  public:
	ProjectBuild( const std::string& name, const std::string& projectRoot ) :
		mName( name ), mProjectRoot( projectRoot ){};

	const ProjectBuildConfig& getConfig() const { return mConfig; }

	bool isOSSupported( const std::string& os ) const;

  protected:
	friend class ProjectBuildManager;

	std::string mName;
	std::string mProjectRoot;
	std::unordered_set<std::string> mOS;
	std::unordered_set<std::string> mBuildTypes;
	ProjectBuildSteps mBuild;
	ProjectBuildSteps mClean;
	ProjectBuildKeyVal mEnvs;
	ProjectBuildKeyVal mVars;
	ProjectBuildConfig mConfig;
	ProjectBuildOutputParser mOutputParser;

	void replaceVars();
};

using ProjectBuildMap = std::unordered_map<std::string, ProjectBuild>;

struct ProjectBuildCommand : public ProjectBuildStep {
	ProjectBuildKeyVal envs;
	ProjectBuildConfig config;

	ProjectBuildCommand( const ProjectBuildStep& step, const ProjectBuildKeyVal& envs ) :
		ProjectBuildStep( step ), envs( envs ) {}
};

using ProjectBuildCommands = std::vector<ProjectBuildCommand>;

struct ProjectBuildCommandsRes {
	String errorMsg;
	ProjectBuildCommands cmds;

	ProjectBuildCommandsRes() {}

	ProjectBuildCommandsRes( const String& errMsg ) : errorMsg( errMsg ) {}

	ProjectBuildCommandsRes( ProjectBuildCommands&& cmds ) : cmds( cmds ) {}

	bool isValid() { return errorMsg.empty(); }
};

using ProjectBuildProgressFn = std::function<void( int curProgress, std::string buffer )>;
using ProjectBuildDoneFn = std::function<void( int exitCode )>;
using ProjectBuildi18nFn =
	std::function<String( const std::string& /*key*/, const String& /*defaultvalue*/ )>;

class ProjectBuildManager {
  public:
	ProjectBuildManager( const std::string& projectRoot, std::shared_ptr<ThreadPool> pool );

	~ProjectBuildManager();

	ProjectBuildCommandsRes run( const std::string& buildName, const ProjectBuildi18nFn& i18n,
								 const std::string& buildType = "",
								 const ProjectBuildProgressFn& progressFn = {},
								 const ProjectBuildDoneFn& doneFn = {} );

	ProjectBuildCommandsRes generateBuildCommands( const std::string& buildName,
												   const ProjectBuildi18nFn& i18n,
												   const std::string& buildType = "" );

	ProjectBuildOutputParser getOutputParser( const std::string& buildName );

	const ProjectBuildMap& getBuilds() const { return mBuilds; }

	const std::string& getProjectRoot() const { return mProjectRoot; }

	const std::string& getProjectFile() const { return mProjectFile; }

	bool loaded() const { return mLoaded; }

	bool loading() const { return mLoading; }

	bool isBuilding() const { return mBuilding; }

	void cancelBuild();

  protected:
	std::string mProjectRoot;
	std::string mProjectFile;
	ProjectBuildMap mBuilds;
	std::shared_ptr<ThreadPool> mThreadPool;
	bool mLoaded{ false };
	bool mLoading{ false };
	bool mBuilding{ false };
	bool mShuttingDown{ false };
	bool mCancelBuild{ false };

	void runBuild( const std::string& buildName, const std::string& buildType,
				   const ProjectBuildi18nFn& i18n, const ProjectBuildCommandsRes& res,
				   const ProjectBuildProgressFn& progressFn = {},
				   const ProjectBuildDoneFn& doneFn = {} );

	bool load();
};

} // namespace ecode

#endif
