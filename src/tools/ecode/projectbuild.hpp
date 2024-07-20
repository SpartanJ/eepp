#ifndef ECODE_PROJECTBUILD_HPP
#define ECODE_PROJECTBUILD_HPP

#include "appconfig.hpp"
#include <eepp/system/process.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;

namespace ecode {

class App;
class StatusBuildOutputController;
class StatusAppOutputController;

/** reference:
{
  "ecode": {
	"build": [
	  {
		"args": "--with-debug-symbols gmake",
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
	  "clear_sys_env": false
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
	"run": [
	  {
		"args": "-x",
		"command": "${project_root}/bin/ecode-debug",
		"name": "ecode-debug",
		"working_dir": "${project_root}/bin/"
	  },
	  {
		"args": "-x",
		"command": "${project_root}/bin/ecode",
		"name": "ecode-release",
		"working_dir": "${project_root}/bin/"
	  }
	],
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
	std::string name;
	bool enabled{ true };
	bool runInTerminal{ false };
};

using ProjectBuildSteps = std::vector<ProjectBuildStep>;
using ProjectBuildKeyVal = std::vector<std::pair<std::string, std::string>>;

struct ProjectBuildConfig {
	bool clearSysEnv{ false };
};

enum class ProjectOutputParserTypes { Error = 0, Warning = 1, Notice = 2 };

struct PatternOrder {
	int file{ 1 };
	int line{ 2 };
	int col{ 3 };
	int message{ 4 };
};

struct ProjectBuildOutputParserConfig {
	static std::string typeToString( ProjectOutputParserTypes type ) {
		switch ( type ) {
			case ProjectOutputParserTypes::Notice:
				return "notice";
			case ProjectOutputParserTypes::Warning:
				return "warning";
			case ProjectOutputParserTypes::Error:
			default:
				return "error";
		}
	}

	ProjectOutputParserTypes type;
	std::string pattern;
	PatternOrder patternOrder;
};

class ProjectBuildOutputParser {
  public:
	static std::map<std::string, ProjectBuildOutputParser> getPresets();

	static bool existsPreset( const std::string& name );

	static ProjectBuildOutputParser getGeneric();

	const std::vector<ProjectBuildOutputParserConfig>& getPresetConfig() const {
		return mPresetConfig;
	}

	const std::vector<ProjectBuildOutputParserConfig>& getConfig() const { return mConfig; }

	bool useRelativeFilePaths() const { return mRelativeFilePaths; }

	const std::string& getPreset() const { return mPreset; }

  protected:
	friend class ProjectBuildManager;
	friend class ProjectBuild;
	friend class UIBuildSettings;

	bool mRelativeFilePaths{ true };
	std::string mPreset;
	std::vector<ProjectBuildOutputParserConfig> mPresetConfig;
	std::vector<ProjectBuildOutputParserConfig> mConfig;
};

class ProjectBuild {
  public:
	using Map = std::unordered_map<std::string, ProjectBuild>;

	ProjectBuild( const std::string& name, const std::string& projectRoot ) :
		mName( name ), mProjectRoot( projectRoot ){};

	const ProjectBuildConfig& getConfig() const { return mConfig; }

	bool isOSSupported( const std::string& os ) const;

	const std::string& getName() const { return mName; }

	const std::set<std::string>& buildTypes() const { return mBuildTypes; }

	const std::set<std::string>& os() const { return mOS; }

	const ProjectBuildOutputParser& getOutputParser() const { return mOutputParser; }

	const ProjectBuildSteps& buildSteps() const { return mBuild; }

	const ProjectBuildSteps& cleanSteps() const { return mClean; }

	const ProjectBuildSteps& runConfigs() const { return mRun; }

	const ProjectBuildKeyVal& envs() const { return mEnvs; }

	const ProjectBuildKeyVal& vars() const { return mVars; }

	bool hasBuild() const { return !mBuild.empty(); }

	bool hasClean() const { return !mClean.empty(); }

	bool hasRun() const {
		return !mRun.empty() && ( !mRun.front().cmd.empty() || !mRun.front().args.empty() ||
								  !mRun.front().workingDir.empty() );
	}

	ProjectBuildStep replaceVars( const ProjectBuildStep& step ) const;

	ProjectBuildSteps replaceVars( const ProjectBuildSteps& steps ) const;

	static json serialize( const ProjectBuild::Map& builds );

	static ProjectBuild::Map deserialize( const json& j, const std::string& projectRoot );

  protected:
	friend class ProjectBuildManager;
	friend class UIBuildSettings;

	std::string mName;
	std::string mProjectRoot;
	std::set<std::string> mOS;
	std::set<std::string> mBuildTypes;
	ProjectBuildSteps mBuild;
	ProjectBuildSteps mClean;
	ProjectBuildSteps mRun;
	ProjectBuildKeyVal mEnvs;
	ProjectBuildKeyVal mVars;
	ProjectBuildConfig mConfig;
	ProjectBuildOutputParser mOutputParser;
};

struct ProjectBuildCommand : public ProjectBuildStep {
	ProjectBuildConfig config;

	ProjectBuildCommand( const ProjectBuildStep& step ) : ProjectBuildStep( step ) {}
};

using ProjectBuildCommands = std::vector<ProjectBuildCommand>;

struct ProjectBuildCommandsRes {
	String errorMsg;
	ProjectBuildCommands cmds;
	ProjectBuildKeyVal envs;

	ProjectBuildCommandsRes() {}

	ProjectBuildCommandsRes( const String& errMsg ) : errorMsg( errMsg ) {}

	ProjectBuildCommandsRes( ProjectBuildCommands&& cmds ) : cmds( cmds ) {}

	bool isValid() { return errorMsg.empty(); }
};

using ProjectBuildProgressFn =
	std::function<void( int curProgress, std::string buffer, const ProjectBuildCommand* cmd )>;
using ProjectBuildDoneFn = std::function<void( int exitCode, const ProjectBuildCommand* cmd )>;
using ProjectBuildi18nFn =
	std::function<String( const std::string& /*key*/, const String& /*defaultvalue*/ )>;

class ProjectBuildManager {
  public:
	ProjectBuildManager( const std::string& projectRoot, std::shared_ptr<ThreadPool> pool,
						 UITabWidget* sidePanel, App* app );

	~ProjectBuildManager();

	ProjectBuildCommandsRes build( const std::string& buildName, const ProjectBuildi18nFn& i18n,
								   const std::string& buildType = "",
								   const ProjectBuildProgressFn& progressFn = {},
								   const ProjectBuildDoneFn& doneFn = {}, bool isClean = false );

	ProjectBuildCommandsRes run( const ProjectBuildCommand& runData, const ProjectBuildi18nFn& i18n,
								 const ProjectBuildProgressFn& progressFn = {},
								 const ProjectBuildDoneFn& doneFn = {} );

	ProjectBuildCommandsRes generateBuildCommands( const std::string& buildName,
												   const ProjectBuildi18nFn& i18n,
												   const std::string& buildType = "",
												   bool isClean = false );

	void replaceDynamicVars( ProjectBuildCommand& cmd );

	ProjectBuildOutputParser getOutputParser( const std::string& buildName );

	const ProjectBuild::Map& getBuilds() const { return mBuilds; }

	const std::string& getProjectRoot() const { return mProjectRoot; }

	const std::string& getProjectFile() const { return mProjectFile; }

	std::string getCurrentDocument();

	ProjectBuild* getBuild( const std::string& buildName );

	bool hasBuild( const std::string& name ) { return mBuilds.find( name ) != mBuilds.end(); }

	bool hasBuildCommands( const std::string& name );

	bool hasCleanCommands( const std::string& name );

	bool loaded() const { return mLoadedWithBuilds; }

	bool loading() const { return mLoading; }

	bool isBuilding() const { return mBuilding; }

	bool isRunningApp() const { return mRunning; }

	void cancelBuild();

	void cancelRun();

	ProjectBuildConfiguration getConfig() const;

	void setConfig( const ProjectBuildConfiguration& config );

	void buildCurrentConfig( StatusBuildOutputController* sboc,
							 std::function<void( int exitStatus )> doneFn = {} );

	void cleanCurrentConfig( StatusBuildOutputController* sboc );

	void runCurrentConfig( StatusAppOutputController* saoc, bool build,
						   StatusBuildOutputController* sboc = nullptr );

	void editCurrentBuild();

	bool hasRunConfig();

	bool hasBuildConfig();

	void selectTab();

  protected:
	std::string mProjectRoot;
	std::string mProjectFile;
	ProjectBuild::Map mBuilds;
	ProjectBuildConfiguration mConfig;
	std::shared_ptr<ThreadPool> mThreadPool;
	UITabWidget* mSidePanel{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	UITab* mTab{ nullptr };
	App* mApp{ nullptr };
	std::unique_ptr<Process> mProcess;
	ProjectBuild mNewBuild;
	bool mLoadedWithBuilds{ false };
	bool mLoading{ false };
	bool mBuilding{ false };
	bool mShuttingDown{ false };
	bool mCancelBuild{ false };
	bool mCancelRun{ false };
	bool mRunning{ false };
	std::unordered_map<Node*, std::set<Uint32>> mCbs;

	void runBuild( const std::string& buildName, const std::string& buildType,
				   const ProjectBuildi18nFn& i18n, const ProjectBuildCommandsRes& res,
				   const ProjectBuildProgressFn& progressFn = {},
				   const ProjectBuildDoneFn& doneFn = {} );

	void runApp( const ProjectBuildCommand& runStep, const ProjectBuildi18nFn& i18n,
				 const ProjectBuildCommandsRes& res, const ProjectBuildProgressFn& progressFn = {},
				 const ProjectBuildDoneFn& doneFn = {} );

	bool load();

	bool save();

	bool saveAsync();

	void buildSidePanelTab();

	void updateSidePanelTab();

	void updateBuildType();

	void updateRunConfig();

	void addNewBuild();

	bool cloneBuild( const std::string& build, std::string newBuildName );

	void addBuild( UIWidget* buildTab );

	void editBuild( std::string buildName, UIWidget* buildTab );

	void runConfig( StatusAppOutputController* saoc );
};

} // namespace ecode

#endif
