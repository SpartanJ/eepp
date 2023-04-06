#ifndef ECODE_PROJECTBUILD_HPP
#define ECODE_PROJECTBUILD_HPP

#include <eepp/system/threadpool.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace EE::System;

namespace ecode {

/** reference:

{
  "ecode": {
	"build": [
	  {
		"args": "--with-mojoal --with-debug-symbols gmake",
		"command": "premake4",
		"working_dir": "$PROJECT_ROOT"
	  },
	  {
		"args": "-j$(nproc) config=release ecode",
		"command": "make",
		"working_dir": "${build_dir}"
	  }
	],
	"clean": [
	  {
		"args": "config=release clean",
		"command": "make",
		"working_dir": "${build_dir}"
	  }
	],
	"config": {
	  "clear_sys_env": false
	},
	"var": {
	  "build_dir": "$PROJECT_ROOT/make/linux"
	},
	"env": {
	  "SHELL": "fish"
	},
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
  protected:
	friend class ProjectBuildManager;

	bool mRelativeFilePaths{ true };
	std::vector<ProjectBuildOutputParserConfig> mConfig;
};

class ProjectBuild {
  public:
	ProjectBuild( const std::string& name, const std::string& projectRoot ) :
		mName( name ), mProjectRoot( projectRoot ){};

  protected:
	friend class ProjectBuildManager;

	std::string mName;
	std::string mProjectRoot;
	ProjectBuildSteps mBuild;
	ProjectBuildSteps mClean;
	ProjectBuildKeyVal mEnvs;
	ProjectBuildKeyVal mVars;
	ProjectBuildConfig mConfig;
	ProjectBuildOutputParser mOutputParser;

	void replaceVars();
};

using ProjectBuildMap = std::unordered_map<std::string, ProjectBuild>;

class ProjectBuildManager {
  public:
	ProjectBuildManager( const std::string& projectRoot, std::shared_ptr<ThreadPool> pool );

	void run( const std::string& buildName );

	const ProjectBuildMap& getBuilds() const { return mBuilds; }

	const std::string& getProjectRoot() const { return mProjectRoot; }

	const std::string& getProjectFile() const { return mProjectFile; }

	bool loaded() const { return mLoaded; }

	bool loading() const { return mLoading; }

  protected:
	std::string mProjectRoot;
	std::string mProjectFile;
	ProjectBuildMap mBuilds;
	std::shared_ptr<ThreadPool> mThreadPool;
	bool mLoaded{ false };
	bool mLoading{ false };

	bool load();
};

} // namespace ecode

#endif
