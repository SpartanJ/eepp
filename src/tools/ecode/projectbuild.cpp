#include "projectbuild.hpp"
#include "ecode.hpp"
#include "statusbuildoutputcontroller.hpp"
#include "uibuildsettings.hpp"
#include <eepp/core/string.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace EE::Scene;

namespace ecode {

static constexpr auto SidePanelLoadUniqueId = String::hash( "ProjectBuildManager::load::async" );

static const char* VAR_PROJECT_ROOT = "${project_root}";
static const char* VAR_BUILD_TYPE = "${build_type}";
static const char* VAR_OS = "${os}";
static const char* VAR_ARCH = "${arch}";
static const char* VAR_NPROC = "${nproc}";
static const char* VAR_CURRENT_DOC = "${current_doc}";
static const char* VAR_CURRENT_DOC_NAME = "${current_doc_name}";
static const char* VAR_CURRENT_DOC_DIR = "${current_doc_dir}";

static void replaceVar( ProjectBuildStep& s, const std::string& var, const std::string& val ) {
	static std::string slashDup = FileSystem::getOSSlash() + FileSystem::getOSSlash();
	String::replaceAll( s.workingDir, var, val );
	String::replaceAll( s.cmd, var, val );
	String::replaceAll( s.args, var, val );
	String::replaceAll( s.workingDir, slashDup, FileSystem::getOSSlash() );
	FileSystem::dirAddSlashAtEnd( s.workingDir );
}

ProjectBuildStep ProjectBuild::replaceVars( const ProjectBuildStep& step ) const {
	ProjectBuildStep s( step );
	replaceVar( s, VAR_PROJECT_ROOT, mProjectRoot );
	for ( auto& var : mVars ) {
		std::string varKey( "${" + var.first + "}" );
		std::string varVal( var.second );
		String::replaceAll( varVal, VAR_PROJECT_ROOT, mProjectRoot );
		replaceVar( s, varKey, varVal );
	}
	return s;
}

ProjectBuildSteps ProjectBuild::replaceVars( const ProjectBuildSteps& steps ) const {
	ProjectBuildSteps newSteps( deepCopySteps( steps ) );
	for ( auto& s : newSteps ) {
		replaceVar( *s.get(), VAR_PROJECT_ROOT, mProjectRoot );
		for ( auto& var : mVars ) {
			std::string varKey( "${" + var.first + "}" );
			std::string varVal( var.second );
			String::replaceAll( varVal, VAR_PROJECT_ROOT, mProjectRoot );
			replaceVar( *s.get(), varKey, varVal );
		}
	}
	return newSteps;
}

json ProjectBuild::serialize( const ProjectBuild::Map& builds ) {
	json j;

	for ( const auto& buildCfg : builds ) {
		const auto& curBuild = buildCfg.second;
		auto& bj = j[buildCfg.first];

		bj["build"] = json::array();
		auto& jbuild = bj["build"];
		for ( const auto& build : curBuild.buildSteps() ) {
			json step;
			step["working_dir"] = build->workingDir;
			step["args"] = build->args;
			step["command"] = build->cmd;
			if ( !build->enabled )
				step["enabled"] = build->enabled;
			jbuild.push_back( step );
		}

		bj["clean"] = json::array();
		auto& jclean = bj["clean"];
		for ( const auto& build : curBuild.cleanSteps() ) {
			json step;
			step["working_dir"] = build->workingDir;
			step["args"] = build->args;
			step["command"] = build->cmd;
			if ( !build->enabled )
				step["enabled"] = build->enabled;
			jclean.push_back( step );
		}

		if ( curBuild.hasRun() ) {
			bj["run"] = json::array();
			auto& jrun = bj["run"];
			for ( auto& run : curBuild.mRun ) {
				json step;
				step["name"] = run->name;
				step["working_dir"] = run->workingDir;
				step["args"] = run->args;
				step["command"] = run->cmd;
				if ( !run->enabled )
					step["enabled"] = run->enabled;
				if ( run->runInTerminal )
					step["run_in_terminal"] = run->runInTerminal;
				jrun.push_back( step );
			}
		}

		bj["build_types"] = curBuild.buildTypes();
		bj["config"]["clear_sys_env"] = curBuild.getConfig().clearSysEnv;
		bj["os"] = curBuild.os();

		if ( !curBuild.vars().empty() ) {
			auto& var = bj["var"];
			for ( const auto& v : curBuild.vars() )
				var[v.first] = v.second;
		}

		if ( !curBuild.envs().empty() ) {
			auto& env = bj["env"];
			for ( const auto& e : curBuild.envs() )
				env[e.first] = e.second;
		}

		auto& op = bj["output_parser"];
		auto& opc = op["config"];

		opc["relative_file_paths"] = curBuild.getOutputParser().useRelativeFilePaths();
		if ( !curBuild.getOutputParser().getPreset().empty() )
			opc["preset"] = curBuild.getOutputParser().getPreset();

		for ( const auto& opct : curBuild.getOutputParser().getConfig() ) {
			std::string type( ProjectBuildOutputParserConfig::typeToString( opct.type ) );
			if ( !op.contains( type ) )
				op[type] = json::array();
			json nopp;
			auto& po = nopp["pattern_order"];
			nopp["pattern"] = opct.pattern;
			po["col"] = opct.patternOrder.col;
			po["line"] = opct.patternOrder.line;
			po["file"] = opct.patternOrder.file;
			po["message"] = opct.patternOrder.message;
			op[type].push_back( nopp );
		}
	}

	return j;
}

ProjectBuildSteps ProjectBuild::deepCopySteps( const ProjectBuildSteps& steps ) const {
	ProjectBuildSteps copy;
	for ( const auto& step : steps )
		copy.push_back( std::make_unique<ProjectBuildStep>( *step.get() ) );
	return copy;
}

ProjectBuild::ProjectBuild( const ProjectBuild& other ) :
	mName( other.mName ),
	mProjectRoot( other.mProjectRoot ),
	mOS( other.mOS ),
	mBuildTypes( other.mBuildTypes ),
	mEnvs( other.mEnvs ),
	mVars( other.mVars ),
	mConfig( other.mConfig ),
	mOutputParser( other.mOutputParser ) {
	mBuild = deepCopySteps( other.mBuild );
	mClean = deepCopySteps( other.mClean );
	mRun = deepCopySteps( other.mRun );
}

ProjectBuild& ProjectBuild::operator=( const ProjectBuild& other ) {
	if ( this != &other ) {
		mName = other.mName;
		mProjectRoot = other.mProjectRoot;
		mOS = other.mOS;
		mBuildTypes = other.mBuildTypes;
		mEnvs = other.mEnvs;
		mVars = other.mVars;
		mConfig = other.mConfig;
		mOutputParser = other.mOutputParser;
		mBuild.clear();
		mClean.clear();
		mRun.clear();
		mBuild = deepCopySteps( other.mBuild );
		mClean = deepCopySteps( other.mClean );
		mRun = deepCopySteps( other.mRun );
	}
	return *this;
}

ProjectBuild::ProjectBuild( const std::string& name, const std::string& projectRoot ) :
	mName( name ), mProjectRoot( projectRoot ) {}

bool ProjectBuild::isOSSupported( const std::string& os ) const {
	return mOS.empty() || std::any_of( mOS.begin(), mOS.end(), [&os]( const auto& oos ) {
			   return oos == os || oos == "any";
		   } );
}

ProjectBuildManager::ProjectBuildManager( const std::string& projectRoot,
										  std::shared_ptr<ThreadPool> pool, UITabWidget* sidePanel,
										  App* app ) :
	mProjectRoot( projectRoot ),
	mThreadPool( pool ),
	mSidePanel( sidePanel ),
	mApp( app ),
	mNewBuild( mApp->i18n( "new_name", "New Name" ), mProjectRoot ) {
	FileSystem::dirAddSlashAtEnd( mProjectRoot );

	if ( mThreadPool ) {
		mThreadPool->run( [this]() { load(); } );
	} else {
		load();
	}
}

void ProjectBuildManager::addNewBuild() {
	std::string name = mNewBuild.getName();
	ProjectBuild newBuild = mNewBuild;
	bool found;
	do {
		found = false;
		for ( const auto& b : mBuilds ) {
			if ( b.first == name ) {
				name += " (" + mApp->i18n( "copy", "Copy" ) + ")";
				found = true;
			}
		}
	} while ( found );
	newBuild.mName = name;
	mBuilds.insert(
		std::make_pair<std::string, ProjectBuild>( std::move( name ), std::move( newBuild ) ) );
}

bool ProjectBuildManager::cloneBuild( const std::string& build, std::string newBuildName ) {
	auto oldBuild = mBuilds.find( build );
	if ( oldBuild == mBuilds.end() )
		return false;
	ProjectBuild newBuild = oldBuild->second;
	newBuild.mName = newBuildName;
	mBuilds.insert( std::make_pair<std::string, ProjectBuild>( std::move( newBuildName ),
															   std::move( newBuild ) ) );
	return true;
}

void ProjectBuildManager::addBuild( UIWidget* buildTab ) {
	mNewBuild = ProjectBuild( mApp->i18n( "new_name", "New Name" ), mProjectRoot );
	std::string hashName = String::toString( String::hash( "new_name" ) );
	UIWidget* widget = nullptr;
	if ( ( widget = buildTab->getUISceneNode()->getRoot()->querySelector( "#build_settings_" +
																		  hashName ) ) ) {
		widget->asType<UITab>()->select();
		return;
	}
	auto ret = mApp->getSplitter()->createWidget(
		UIBuildSettings::New( mNewBuild, mConfig, true, nullptr ),
		mApp->i18n( "build_settings", "Build Settings" ) );
	auto bs = ret.second->asType<UIBuildSettings>();
	bs->setTab( ret.first );
	mCbs[bs].insert( bs->on( Event::OnConfirm, [this, bs]( const Event* event ) {
		event->getNode()->removeEventListener( event->getCallbackId() );
		mCbs[bs].erase( event->getCallbackId() );
		addNewBuild();
		saveAsync();
		updateSidePanelTab();
	} ) );
	mCbs[bs].insert( bs->on( Event::OnClose, [this, bs]( auto ) { mCbs.erase( bs ); } ) );
	bs->on( Event::OnClear, [this]( const Event* event ) {
		if ( mBuilds.erase( event->asTextEvent()->getText() ) > 0 ) {
			if ( mConfig.buildName == event->asTextEvent()->getText() )
				mConfig.buildName = mBuilds.empty() ? "" : mBuilds.begin()->first;
			updateSidePanelTab();
		}
	} );
	ret.first->setIcon( mApp->findIcon( "hammer" ) );
}

void ProjectBuildManager::editBuild( std::string buildName, UIWidget* buildTab ) {
	if ( buildName.empty() )
		return;

	auto build = mBuilds.find( buildName );
	if ( build == mBuilds.end() )
		return;

	std::string hashName = String::toString( String::hash( buildName ) );
	UIWidget* widget = nullptr;
	if ( ( widget = buildTab->getUISceneNode()->getRoot()->querySelector( "#build_settings_" +
																		  hashName ) ) ) {
		widget->asType<UITab>()->getTabWidget()->setTabSelected( widget->asType<UITab>() );
		return;
	}
	auto ret = mApp->getSplitter()->createWidget(
		UIBuildSettings::New( build->second, mConfig, false,
							  [this]( const std::string& oldName, const std::string& newName ) {
								  auto buildIt = mBuilds.find( oldName );
								  if ( buildIt != mBuilds.end() ) {
									  auto build = mBuilds.extract( buildIt );
									  build.key() = newName;
									  mBuilds.insert( std::move( build ) );
								  }
							  } ),
		mApp->i18n( "build_settings", "Build Settings" ) );
	auto bs = ret.second->asType<UIBuildSettings>();
	bs->setTab( ret.first );
	mCbs[bs].insert( bs->on( Event::OnConfirm, [this, bs]( const Event* event ) {
		event->getNode()->removeEventListener( event->getCallbackId() );
		mCbs[bs].erase( event->getCallbackId() );
		saveAsync();
	} ) );
	mCbs[bs].insert( bs->on( Event::OnClose, [this, bs]( auto ) { mCbs.erase( bs ); } ) );
	bs->on( Event::OnClear, [this]( const Event* event ) {
		if ( mBuilds.erase( event->asTextEvent()->getText() ) > 0 ) {
			if ( mConfig.buildName == event->asTextEvent()->getText() )
				mConfig.buildName = mBuilds.empty() ? "" : mBuilds.begin()->first;
			updateSidePanelTab();
		}
	} );
	bs->on( Event::OnCopy, [this, buildName, buildTab]( const Event* event ) {
		std::string clonedName( event->asTextEvent()->getText() );
		if ( mBuilds.find( clonedName ) != mBuilds.end() ) {
			UIMessageBox::New(
				UIMessageBox::OK,
				mApp->i18n( "cloned_name_must_be_different",
							"Cloned name must be different from any existing build name." ) )
				->show();
		} else {
			cloneBuild( buildName, clonedName );
			updateSidePanelTab();
			editBuild( clonedName, buildTab );
		}
	} );
	ret.first->setIcon( mApp->findIcon( "hammer" ) );
}

void ProjectBuildManager::editCurrentBuild() {
	UIWidget* buildTab = mTab->getOwnedWidget()->find<UIWidget>( "build_tab_view" );
	if ( buildTab == nullptr )
		return;
	if ( !mConfig.buildName.empty() && !mBuilds.empty() )
		editBuild( mConfig.buildName, buildTab );
	else
		mTab->setTabSelected();
}

void ProjectBuildManager::selectTab() {
	mTab->setTabSelected();
}

ProjectBuildManager::~ProjectBuildManager() {
	mSidePanel->removeActionsByTag( SidePanelLoadUniqueId );

	if ( mUISceneNode && !SceneManager::instance()->isShuttingDown() && mSidePanel && mTab ) {
		mSidePanel->removeTab( mTab );
	}
	if ( mUISceneNode && mUISceneNode->getRoot()->querySelector( "#build_settings_new_name" ) )
		addNewBuild();

	for ( const auto& cbs : mCbs )
		for ( const auto& cb : cbs.second )
			cbs.first->removeEventListener( cb );

	mShuttingDown = true;
	mCancelBuild = true;
	mCancelRun = true;
	save();
	while ( mLoading )
		Sys::sleep( Milliseconds( 0.1f ) );
	while ( mBuilding )
		Sys::sleep( Milliseconds( 0.1f ) );
}

void ProjectBuildManager::replaceDynamicVars( ProjectBuildCommand& cmd ) {
	std::string currentOS = String::toLower( Sys::getPlatform() );
	std::string nproc = String::format( "%d", Sys::getCPUCount() );
	std::string curDoc = getCurrentDocument();
	std::string curDocName =
		FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( curDoc ) );
	std::string curDocDir = FileSystem::fileRemoveFileName( curDoc );
	replaceVar( cmd, VAR_OS, currentOS );
	replaceVar( cmd, VAR_ARCH, Sys::getOSArchitecture() );
	replaceVar( cmd, VAR_NPROC, nproc );
	replaceVar( cmd, VAR_CURRENT_DOC, curDoc );
	replaceVar( cmd, VAR_CURRENT_DOC_NAME, curDocName );
	replaceVar( cmd, VAR_CURRENT_DOC_DIR, curDocDir );
	if ( cmd.workingDir.empty() )
		cmd.workingDir = mProjectRoot;
}

ProjectBuildCommandsRes ProjectBuildManager::generateBuildCommands( const std::string& buildName,
																	const ProjectBuildi18nFn& i18n,
																	const std::string& buildType,
																	bool isClean ) {
	const auto& buildIt = mBuilds.find( buildName );

	if ( buildIt == mBuilds.end() )
		return { i18n( "build_name_not_found", "Build name not found!" ) };

	const auto& build = buildIt->second;

	if ( !build.mBuildTypes.empty() && buildType.empty() )
		return { i18n( "build_type_required", "Build type must be set!" ) };

	std::string currentOS = String::toLower( Sys::getPlatform() );

	if ( !build.isOSSupported( currentOS ) )
		return {
			i18n( "build_os_not_supported", "Operating System not supported for this build!" ) };

	auto finalBuild( build.replaceVars( isClean ? build.mClean : build.mBuild ) );

	ProjectBuildCommandsRes res;
	for ( const auto& step : finalBuild ) {
		ProjectBuildCommand buildCmd( *step );
		replaceDynamicVars( buildCmd );
		if ( !buildType.empty() )
			replaceVar( buildCmd, VAR_BUILD_TYPE, buildType );
		buildCmd.config = build.mConfig;
		res.cmds.emplace_back( std::move( buildCmd ) );
	}

	res.envs = build.mEnvs;

	return res;
}

ProjectBuildCommandsRes
ProjectBuildManager::build( const std::string& buildName, const ProjectBuildi18nFn& i18n,
							const std::string& buildType, const ProjectBuildProgressFn& progressFn,
							const ProjectBuildDoneFn& doneFn, bool isClean ) {
	ProjectBuildCommandsRes res = generateBuildCommands( buildName, i18n, buildType, isClean );
	if ( !res.isValid() )
		return res;
	if ( !mThreadPool ) {
		res.errorMsg = i18n( "no_threads", "Threaded ecode required to compile builds." );
		return res;
	}

	mThreadPool->run( [this, res, progressFn, doneFn, i18n, buildName, buildType]() {
		runBuild( buildName, buildType, i18n, res, progressFn, doneFn );
	} );

	return res;
}

ProjectBuildCommandsRes ProjectBuildManager::run( const ProjectBuildCommand& runData,
												  const ProjectBuildi18nFn& i18n,
												  const ProjectBuildProgressFn& progressFn,
												  const ProjectBuildDoneFn& doneFn ) {
	ProjectBuildCommandsRes res;

	if ( !mThreadPool ) {
		res.errorMsg = i18n( "no_threads", "Threaded ecode required to run builds." );
		return res;
	}

	mThreadPool->run( [this, res, progressFn, doneFn, i18n, runData]() {
		runApp( runData, i18n, res, progressFn, doneFn );
	} );

	return res;
}

static bool isValidType( const std::string& typeStr ) {
	return "error" == typeStr || "warning" == typeStr || "notice" == typeStr;
}

static ProjectOutputParserTypes outputParserType( const std::string& typeStr ) {
	if ( "error" == typeStr )
		return ProjectOutputParserTypes::Error;
	if ( "warning" == typeStr )
		return ProjectOutputParserTypes::Warning;
	if ( "notice" == typeStr )
		return ProjectOutputParserTypes::Notice;
	return ProjectOutputParserTypes::Notice;
}

ProjectBuild::Map ProjectBuild::deserialize( const json& j, const std::string& projectRoot ) {
	ProjectBuild::Map prjBuild;
	for ( const auto& build : j.items() ) {
		ProjectBuild b( build.key(), projectRoot );
		const auto& buildObj = build.value();

		if ( buildObj.contains( "os" ) && buildObj["os"].is_array() ) {
			const auto& oss = buildObj["os"];
			for ( const auto& os : oss )
				b.mOS.emplace( os );
		}

		if ( buildObj.contains( "build_types" ) && buildObj["build_types"].is_array() ) {
			const auto& bts = buildObj["build_types"];
			for ( const auto& bt : bts )
				b.mBuildTypes.emplace( bt );
		}

		if ( buildObj.contains( "config" ) && buildObj["config"].is_object() ) {
			b.mConfig.clearSysEnv = buildObj.value( "clear_sys_env", false );
		}

		if ( buildObj.contains( "var" ) && buildObj["var"].is_object() ) {
			const auto& vars = buildObj["var"];
			for ( const auto& var : vars.items() )
				b.mVars.push_back( std::make_pair( var.key(), var.value() ) );
		}

		if ( buildObj.contains( "env" ) && buildObj["env"].is_object() ) {
			const auto& vars = buildObj["env"];
			for ( const auto& var : vars.items() )
				b.mEnvs.push_back( std::make_pair( var.key(), var.value() ) );
		}

		if ( buildObj.contains( "build" ) && buildObj["build"].is_array() ) {
			const auto& buildArray = buildObj["build"];
			for ( const auto& step : buildArray ) {
				std::unique_ptr<ProjectBuildStep> bstep = std::make_unique<ProjectBuildStep>();
				bstep->cmd = step.value( "command", "" );
				bstep->args = step.value( "args", "" );
				bstep->workingDir = step.value( "working_dir", "" );
				bstep->enabled = step.value( "enabled", true );
				b.mBuild.emplace_back( std::move( bstep ) );
			}
		}

		if ( buildObj.contains( "clean" ) && buildObj["clean"].is_array() ) {
			const auto& cleanArray = buildObj["clean"];
			for ( const auto& step : cleanArray ) {
				std::unique_ptr<ProjectBuildStep> cstep = std::make_unique<ProjectBuildStep>();
				cstep->cmd = step.value( "command", "" );
				cstep->args = step.value( "args", "" );
				cstep->workingDir = step.value( "working_dir", "" );
				cstep->enabled = step.value( "enabled", true );
				b.mClean.emplace_back( std::move( cstep ) );
			}
		}

		if ( buildObj.contains( "run" ) && buildObj["run"].is_array() ) {
			const auto& runArray = buildObj["run"];
			for ( const auto& step : runArray ) {
				std::unique_ptr<ProjectBuildStep> rstep = std::make_unique<ProjectBuildStep>();
				rstep->name = step.value( "name", "" );
				rstep->cmd = step.value( "command", "" );
				rstep->args = step.value( "args", "" );
				rstep->workingDir = step.value( "working_dir", "" );
				rstep->enabled = step.value( "enabled", true );
				rstep->runInTerminal = step.value( "run_in_terminal", false );
				b.mRun.emplace_back( std::move( rstep ) );
			}
		}

		if ( buildObj.contains( "output_parser" ) && buildObj["output_parser"].is_object() ) {
			const auto& op = buildObj["output_parser"];

			ProjectBuildOutputParser outputParser;

			if ( op.contains( "config" ) ) {
				const auto& config = op["config"];
				outputParser.mRelativeFilePaths = config.value( "relative_file_paths", true );

				if ( config.contains( "preset" ) ) {
					auto preset = config.value( "preset", "" );
					if ( !preset.empty() ) {
						outputParser.mPreset = preset;
						auto presets = ProjectBuildOutputParser::getPresets();
						if ( presets.find( preset ) != presets.end() ) {
							outputParser.mPresetConfig = presets[preset].mConfig;
						}
					}
				}
			}

			for ( const auto& item : op.items() ) {
				if ( item.key() != "config" ) {
					auto typeStr = String::toLower( item.key() );

					if ( !isValidType( typeStr ) )
						continue;

					const auto& ptrnCfgs = item.value();
					if ( ptrnCfgs.is_array() ) {
						for ( const auto& ptrnCfg : ptrnCfgs ) {
							ProjectBuildOutputParserConfig opc;
							opc.type = outputParserType( typeStr );
							opc.pattern = ptrnCfg.value( "pattern", "" );

							if ( ptrnCfg.contains( "pattern_order" ) ) {
								const auto& po = ptrnCfg["pattern_order"];
								if ( po.contains( "line" ) && po["line"].is_number() )
									opc.patternOrder.line = po["line"].get<int>();
								if ( po.contains( "col" ) && po["col"].is_number() )
									opc.patternOrder.col = po["col"].get<int>();
								if ( po.contains( "message" ) && po["message"].is_number() )
									opc.patternOrder.message = po["message"].get<int>();
								if ( po.contains( "file" ) && po["file"].is_number() )
									opc.patternOrder.file = po["file"].get<int>();
							}

							outputParser.mConfig.emplace_back( std::move( opc ) );
						}
					}
				}
			}

			b.mOutputParser = outputParser;
		}

		prjBuild.insert( { build.key(), std::move( b ) } );
	}

	return prjBuild;
}

bool ProjectBuildManager::load() {
	ScopedOp scopedOp( [this]() { mLoading = true; },
					   [this]() {
						   mLoading = false;
						   if ( mSidePanel )
							   mSidePanel->runOnMainThread( [this]() { buildSidePanelTab(); },
															Time::Zero, SidePanelLoadUniqueId );
					   } );

	mProjectFile = mProjectRoot + ".ecode/project_build.json";
	if ( !FileSystem::fileExists( mProjectFile ) )
		return false;
	std::string data;
	if ( !FileSystem::fileGet( mProjectFile, data ) )
		return false;
	json j;

	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "ProjectBuildManager::load - Error parsing project build config from "
					"path %s, error: %s, config file content:\n%s",
					mProjectFile.c_str(), e.what(), mProjectFile.c_str() );
		return false;
	}

	mBuilds = ProjectBuild::deserialize( j, mProjectRoot );
	mLoadedWithBuilds = !mBuilds.empty();

	return true;
}

bool ProjectBuildManager::save() {
	ScopedOp scopedOp( [this]() { mLoading = true; }, [this]() { mLoading = false; } );
	json j = ProjectBuild::serialize( mBuilds );
	std::string data( j.empty() ? "{}" : j.dump( 2 ) );
	FileInfo file( mProjectFile );
	bool shouldSave = mLoadedWithBuilds || !mBuilds.empty();

	if ( shouldSave && !file.exists() && !FileSystem::fileExists( file.getDirectoryPath() ) )
		FileSystem::makeDir( file.getDirectoryPath() );

	if ( shouldSave && !FileSystem::fileWrite( mProjectFile, data ) )
		return false;
	return true;
}

bool ProjectBuildManager::saveAsync() {
	if ( mThreadPool ) {
		mThreadPool->run( [this]() { save(); } );
		return true;
	} else {
		return save();
	}
}

ProjectBuildOutputParser ProjectBuildManager::getOutputParser( const std::string& buildName ) {
	auto buildIt = mBuilds.find( buildName );
	if ( buildIt != mBuilds.end() )
		return buildIt->second.mOutputParser;
	return {};
}

ProjectBuild* ProjectBuildManager::getBuild( const std::string& buildName ) {
	auto buildIt = mBuilds.find( buildName );
	if ( buildIt != mBuilds.end() )
		return &buildIt->second;
	return nullptr;
}

bool ProjectBuildManager::hasBuildCommands( const std::string& name ) {
	auto buildIt = mBuilds.find( name );
	if ( buildIt != mBuilds.end() )
		return buildIt->second.hasBuild();
	return false;
}

bool ProjectBuildManager::hasCleanCommands( const std::string& name ) {
	auto buildIt = mBuilds.find( name );
	if ( buildIt != mBuilds.end() )
		return buildIt->second.hasClean();
	return false;
}

void ProjectBuildManager::cancelBuild() {
	mCancelBuild = true;
	if ( mProcess )
		mProcess->kill();
}

void ProjectBuildManager::cancelRun() {
	mCancelRun = true;
	if ( mProcessRun )
		mProcessRun->kill();
}

ProjectBuildConfiguration ProjectBuildManager::getConfig() const {
	return mConfig;
}

void ProjectBuildManager::setConfig( const ProjectBuildConfiguration& config ) {
	if ( mConfig.buildName != config.buildName ) {
		mConfig.buildName = config.buildName;
		updateSidePanelTab();
	}

	if ( mConfig.buildType != config.buildType ) {
		mConfig.buildType = config.buildType;
		updateBuildType();
	}

	if ( mConfig.runName != config.runName ) {
		mConfig.runName = config.runName;
		updateRunConfig();
	}
}

ProjectBuild* ProjectBuildManager::getCurrentBuild() {
	return getBuild( mConfig.buildName );
}

void ProjectBuildManager::buildCurrentConfig( StatusBuildOutputController* sboc,
											  std::function<void( int exitStatus )> doneFn ) {
	if ( sboc && !isBuilding() && !getBuilds().empty() ) {
		const ProjectBuild* build = getCurrentBuild();
		if ( build ) {
			mApp->saveAll();
			sboc->runBuild( build->getName(), mConfig.buildType,
							getOutputParser( build->getName() ), false, doneFn );
		}
	}
}

void ProjectBuildManager::cleanCurrentConfig( StatusBuildOutputController* sboc ) {
	if ( sboc && !isBuilding() && !getBuilds().empty() ) {
		const ProjectBuild* build = getCurrentBuild();
		if ( build )
			sboc->runBuild( build->getName(), mConfig.buildType,
							getOutputParser( build->getName() ), true );
	}
}

void ProjectBuildManager::runCurrentConfig( StatusAppOutputController* saoc, bool build,
											StatusBuildOutputController* sboc ) {
	if ( build ) {
		buildCurrentConfig( sboc, [this, saoc]( int exitStatus ) {
			if ( EXIT_SUCCESS == exitStatus )
				mApp->getUISceneNode()->runOnMainThread( [saoc, this] { runConfig( saoc ); } );
		} );
	} else {
		runConfig( saoc );
	}
}

bool ProjectBuildManager::hasBuildConfig() const {
	return !getBuilds().empty() && !mConfig.buildName.empty();
}

bool ProjectBuildManager::hasRunConfig() {
	if ( hasBuildConfig() ) {
		auto build = getCurrentBuild();
		return build != nullptr && build->hasRun();
	}
	return false;
}

bool ProjectBuildManager::hasBuildConfigWithBuildSteps() {
	if ( hasBuildConfig() ) {
		auto build = getCurrentBuild();
		return build != nullptr && build->hasBuild();
	}
	return {};
}

std::optional<ProjectBuildStep> ProjectBuildManager::getCurrentRunConfig() {
	if ( hasBuildConfig() ) {
		auto build = getCurrentBuild();
		if ( build != nullptr && build->hasRun() ) {
			for ( const auto& crun : build->mRun ) {
				if ( crun->name == mConfig.runName || mConfig.runName.empty() ) {
					ProjectBuildCommand res( build->replaceVars( *crun.get() ) );
					replaceDynamicVars( res );
					return res;
				}
			}
		}
	}
	return {};
}

void ProjectBuildManager::runConfig( StatusAppOutputController* saoc ) {
	if ( !isRunningApp() && !getBuilds().empty() ) {
		BoolScopedOp op( mRunning, true );
		const ProjectBuild* build = getCurrentBuild();

		if ( nullptr == build || !build->hasRun() )
			return;

		const ProjectBuildStep* run = nullptr;
		for ( const auto& crun : build->mRun ) {
			if ( crun->name == mConfig.runName || mConfig.runName.empty() ) {
				run = crun.get();
				break;
			}
		}

		if ( nullptr == run ) {
			Log::info( "No run configuration found to run with runName: %s", mConfig.runName );
			return;
		}

		ProjectBuildCommand finalBuild( build->replaceVars( *run ) );
		replaceDynamicVars( finalBuild );
		String::trimInPlace( finalBuild.cmd );
		if ( finalBuild.cmd.find_first_of( "\\/" ) == std::string::npos &&
			 Sys::which( finalBuild.cmd ).empty() ) {
			FileSystem::dirAddSlashAtEnd( finalBuild.workingDir );
			finalBuild.cmd = finalBuild.workingDir + finalBuild.cmd;
		}

		auto cmd = finalBuild.cmd + " " + finalBuild.args;
		if ( finalBuild.runInTerminal ) {
			UITerminal* term = mApp->getTerminalManager()->createTerminalInSplitter(
				finalBuild.workingDir, "", {}, {}, false );

			Log::info( "Running \"%s\" in terminal", cmd );
			if ( term == nullptr || term->getTerm() == nullptr ) {
				mApp->getTerminalManager()->openInExternalTerminal( cmd, finalBuild.workingDir );
			} else {
				term->executeFile( cmd );
			}
		} else {
			Log::info( "Running \"%s\" in app", cmd );
			finalBuild.config = build->getConfig();
			saoc->run( finalBuild, {} );
		}
	} else {
		Log::info( "ProjectBuildManager::runConfig is already running or isRunningApp() is true" );
	}
}

std::string ProjectBuildManager::getCurrentDocument() {
	return mApp->getSplitter() && mApp->getSplitter()->getCurEditor() &&
				   mApp->getSplitter()->getCurEditor()->getDocument().hasFilepath()
			   ? mApp->getSplitter()->getCurEditor()->getDocument().getFilePath()
			   : "";
}

static std::unordered_map<std::string, std::string>
toUnorderedMap( const ProjectBuildKeyVal& vec ) {
	std::unordered_map<std::string, std::string> map;
	for ( const auto& v : vec )
		map[v.first] = v.second;
	return map;
}

void ProjectBuildManager::runBuild( const std::string& buildName, const std::string& buildType,
									const ProjectBuildi18nFn& i18n,
									const ProjectBuildCommandsRes& res,
									const ProjectBuildProgressFn& progressFn,
									const ProjectBuildDoneFn& doneFn ) {
	BoolScopedOp scopedOp( mBuilding, true );
	Clock clock;

	auto printElapsed = [&clock, &i18n, &progressFn]() {
		if ( progressFn ) {
			progressFn(
				100,
				Sys::getDateTimeStr() + ": " +
					String::format(
						i18n( "build_elapsed_time", "Elapsed Time: %s.\n" ).toUtf8().c_str(),
						clock.getElapsedTime().toString().c_str() ),
				nullptr );
		}
	};

	if ( progressFn ) {
		progressFn( 0,
					Sys::getDateTimeStr() + ": " +
						String::format( i18n( "running_steps_for_project_ellipsis",
											  "Running steps for project %s...\n" )
											.toUtf8()
											.c_str(),
										buildName.c_str() ),
					nullptr );

		if ( !buildType.empty() )
			progressFn(
				0,
				Sys::getDateTimeStr() + ": " +
					String::format(
						i18n( "using_build_type", "Using build type: %s.\n" ).toUtf8().c_str(),
						buildType.c_str() ),
				nullptr );
	}

	int c = 0;
	int totSteps = 0;
	for ( const auto& cmd : res.cmds )
		totSteps += cmd.enabled ? 1 : 0;

	for ( const auto& cmd : res.cmds ) {
		int progress = c > 0 ? c / (Float)totSteps : 0;
		if ( mProcess )
			mProcess->kill();
		mProcess = std::make_unique<Process>();
		auto options = Process::SearchUserPath | Process::NoWindow | Process::CombinedStdoutStderr;
		if ( !cmd.config.clearSysEnv )
			options |= Process::InheritEnvironment;

		if ( !cmd.enabled ) {
			c++;
			continue;
		}

		if ( progressFn ) {
			progressFn( progress,
						Sys::getDateTimeStr() + ": " +
							String::format( i18n( "starting_process", "Starting %s %s\n" ).toUtf8(),
											cmd.cmd, cmd.args ),
						nullptr );

			if ( Sys::which( cmd.cmd ).empty() ) {
				progressFn(
					progress,
					Sys::getDateTimeStr() + ": " +
						String::format( i18n( "command_path_does_not_exists",
											  "WARNING: Command \"%s\" path cannot be found!\n" )
											.toUtf8(),
										cmd.cmd ),
					nullptr );
			}

			if ( FileSystem::fileExists( cmd.workingDir ) ) {
				progressFn(
					progress,
					Sys::getDateTimeStr() + ": " +
						String::format( i18n( "working_dir_at", "Working Dir %s\n" ).toUtf8(),
										cmd.workingDir ),
					nullptr );
			} else {
				progressFn(
					progress,
					Sys::getDateTimeStr() + ": " +
						String::format(
							i18n(
								"working_dir_at_does_not_exists",
								"WARNING: Working Dir is set to \"%s\" but it does not exists!\n" )
								.toUtf8(),
							cmd.workingDir ),
					nullptr );
			}
		}

		if ( mProcess->create( cmd.cmd, cmd.args, options, toUnorderedMap( res.envs ),
							   cmd.workingDir ) ) {
			std::string buffer( 4096, '\0' );
			unsigned bytesRead = 0;
			int returnCode;
			do {
				bytesRead = mProcess->readStdOut( buffer );
				std::string data( buffer.substr( 0, bytesRead ) );
				if ( progressFn )
					progressFn( progress, std::move( data ), &cmd );
			} while ( bytesRead != 0 && mProcess->isAlive() && !mShuttingDown && !mCancelBuild );

			if ( mShuttingDown || mCancelBuild ) {
				mProcess->kill();
				mCancelBuild = false;
				printElapsed();
				if ( doneFn )
					doneFn( EXIT_FAILURE, &cmd );
				return;
			}

			mProcess->join( &returnCode );
			mProcess->destroy();

			if ( returnCode != EXIT_SUCCESS ) {
				if ( progressFn ) {
					progressFn(
						100,
						String::format(
							i18n( "process_exited_with_errors",
								  "The process \"%s\" exited with errors. Returned code: %d\n" )
								.toUtf8()
								.c_str(),
							cmd.cmd.c_str(), returnCode ),
						nullptr );
				}
				printElapsed();
				if ( doneFn )
					doneFn( returnCode, &cmd );
				return;
			} else {
				if ( progressFn ) {
					progressFn( progress,
								String::format( i18n( "process_exited_normally",
													  "The process \"%s\" exited normally.\n" )
													.toUtf8()
													.c_str(),
												cmd.cmd.c_str() ),
								nullptr );
				}
			}
		} else {
			printElapsed();
			if ( doneFn )
				doneFn( EXIT_FAILURE, nullptr );
			return;
		}

		c++;
		if ( c == (int)res.cmds.size() ) {
			printElapsed();
			if ( doneFn )
				doneFn( EXIT_SUCCESS, &cmd );
		}
	}
}

void ProjectBuildManager::runApp( const ProjectBuildCommand& cmd, const ProjectBuildi18nFn& i18n,
								  const ProjectBuildCommandsRes& res,
								  const ProjectBuildProgressFn& progressFn,
								  const ProjectBuildDoneFn& doneFn ) {
	BoolScopedOp op( mRunning, true );
	Clock clock;

	auto printElapsed = [&clock, &i18n, &progressFn]() {
		if ( progressFn ) {
			progressFn(
				100,
				Sys::getDateTimeStr() + ": " +
					String::format(
						i18n( "build_elapsed_time", "Elapsed Time: %s.\n" ).toUtf8().c_str(),
						clock.getElapsedTime().toString().c_str() ),
				nullptr );
		}
	};

	if ( mProcessRun )
		mProcessRun->kill();
	mProcessRun = std::make_unique<Process>();

	auto options = Process::SearchUserPath | Process::CombinedStdoutStderr | Process::NoWindow;
	if ( !cmd.config.clearSysEnv )
		options |= Process::InheritEnvironment;

	if ( progressFn ) {
		progressFn(
			0,
			Sys::getDateTimeStr() + ": " +
				String::format( i18n( "starting_process", "Starting %s %s\n" ).toUtf8().c_str(),
								cmd.cmd.c_str(), cmd.args.c_str() ),
			nullptr );

		progressFn(
			0,
			Sys::getDateTimeStr() + ": " +
				String::format( i18n( "working_dir_at", "Working Dir %s\n" ).toUtf8().c_str(),
								cmd.workingDir.c_str() ),
			nullptr );
	}

	if ( mProcessRun->create( cmd.cmd, cmd.args, options, toUnorderedMap( res.envs ),
							  cmd.workingDir ) ) {
		std::string buffer( 4096, '\0' );
		unsigned bytesRead = 0;
		int returnCode = 0;
		do {
			bytesRead = mProcessRun->readStdOut( buffer );
			std::string data( buffer.substr( 0, bytesRead ) );
			if ( progressFn )
				progressFn( 0, std::move( data ), &cmd );
		} while ( bytesRead != 0 && mProcessRun->isAlive() && !mShuttingDown && !mCancelRun );

		if ( mShuttingDown || mCancelRun ) {
			if ( mProcessRun )
				mProcessRun->kill();
			mCancelRun = false;
			printElapsed();
			if ( doneFn )
				doneFn( EXIT_FAILURE, &cmd );
			return;
		}

		if ( mProcessRun ) {
			mProcessRun->join( &returnCode );
			mProcessRun->destroy();
		}

		if ( returnCode != EXIT_SUCCESS ) {
			if ( progressFn ) {
				progressFn( 100,
							String::format( i18n( "process_exited_with_errors",
												  "The process \"%s\" exited with errors.\n" )
												.toUtf8()
												.c_str(),
											cmd.cmd.c_str() ),
							nullptr );
			}
			printElapsed();
			if ( doneFn )
				doneFn( returnCode, &cmd );
			return;
		} else {
			if ( progressFn ) {
				progressFn( 100,
							String::format( i18n( "process_exited_normally",
												  "The process \"%s\" exited normally.\n" )
												.toUtf8()
												.c_str(),
											cmd.cmd.c_str() ),
							nullptr );
			}
		}
	} else {
		printElapsed();
		if ( doneFn )
			doneFn( EXIT_FAILURE, nullptr );
		return;
	}

	printElapsed();
	if ( doneFn )
		doneFn( EXIT_SUCCESS, &cmd );
}

void ProjectBuildManager::buildSidePanelTab() {
	mUISceneNode = mSidePanel->getUISceneNode();
	UIIcon* icon = mUISceneNode->findIcon( "symbol-property" );
	UIWidget* node = mUISceneNode->loadLayoutFromString(
		R"html(
			<ScrollView id="build_tab_view" lw="mp" lh="mp">
				<vbox lw="mp" lh="wc" padding="4dp">
					<TextView text="@string(build_settings, Build Settings)" font-size="15dp" focusable="false" />
					<TextView text="@string(build_configuration, Build Configuration)" focusable="false" />
					<hbox lw="mp" lh="wc" margin-top="2dp" margin-bottom="4dp">
						<DropDownList id="build_list" layout_width="0" lw8="1" layout_height="wrap_content" />
						<PushButton id="build_edit" id="build_edit" text="@string(edit_build, Edit Build)" tooltip="@string(edit_build, Edit Build)"  text-as-fallback="true" icon="icon(file-edit, 12dp)" margin-left="2dp" />
						<PushButton id="build_add" id="build_add" text="@string(add_build, Add Build)" tooltip="@string(add_build, Add Build)" text-as-fallback="true" icon="icon(add, 12dp)" margin-left="2dp" />
					</hbox>
					<TextView text="@string(build_target, Build Target)" margin-top="8dp" focusable="false" />
					<DropDownList lw="mp" id="build_type_list" margin-top="2dp" />
					<PushButton id="build_button" lw="mp" lh="wc" text="@string(build, Build)" margin-top="8dp" icon="icon(hammer, 12dp)" />
					<PushButton id="clean_button" lw="mp" lh="wc" text="@string(clean, Clean)" margin-top="8dp" icon="icon(eraser, 12dp)" />
					<TextView text="@string(run_target, Run Target)" margin-top="8dp" focusable="false" />
					<DropDownList lw="mp" id="run_config_list" margin-top="2dp" />
					<PushButton id="run_button" lw="mp" lh="wc" text="@string(run, Run)" margin-top="8dp" icon="icon(play, 12dp)" />
					<PushButton id="build_and_run_button" lw="mp" lh="wc" text="@string(build_and_run, Build & Run)" margin-top="8dp" icon="icon(play, 12dp)" />
				</vbox>
			</ScrollView>
		)html" );
	mTab = mSidePanel->add( mUISceneNode->i18n( "build", "Build" ), node,
							icon ? icon->getSize( PixelDensity::dpToPx( 12 ) ) : nullptr );
	mTab->setId( "build_tab" );
	mTab->setTextAsFallback( true );

	updateSidePanelTab();
}

void ProjectBuildManager::updateSidePanelTab() {
	if ( mTab == nullptr )
		return;
	UIWidget* buildTab = mTab->getOwnedWidget()->find<UIWidget>( "build_tab_view" );
	if ( buildTab == nullptr )
		return;
	UIDropDownList* buildList = buildTab->find<UIDropDownList>( "build_list" );
	UIPushButton* buildButton = buildTab->find<UIPushButton>( "build_button" );
	UIPushButton* cleanButton = buildTab->find<UIPushButton>( "clean_button" );
	UIPushButton* runButton = buildTab->find<UIPushButton>( "run_button" );
	UIPushButton* buildAdd = buildTab->find<UIPushButton>( "build_add" );
	UIPushButton* buildEdit = buildTab->find<UIPushButton>( "build_edit" );
	UIPushButton* buildAndRun = buildTab->find<UIPushButton>( "build_and_run_button" );

	buildList->getListBox()->clear();

	String first = mConfig.buildName;
	std::vector<String> buildNamesItems;
	for ( const auto& tbuild : mBuilds ) {
		buildNamesItems.push_back( tbuild.first );
		if ( first.empty() )
			first = tbuild.first;
	}

	std::sort( buildNamesItems.begin(), buildNamesItems.end() );

	buildList->getListBox()->addListBoxItems( buildNamesItems );

	if ( !first.empty() && buildList->getListBox()->getItemIndex( first ) != eeINDEX_NOT_FOUND ) {
		buildList->getListBox()->setSelected( first );
		mConfig.buildName = first;
	} else if ( !buildList->getListBox()->isEmpty() ) {
		buildList->getListBox()->setSelected( 0L );
		mConfig.buildName = buildList->getListBox()->getItemSelectedText();
	}

	buildList->setEnabled( !buildList->getListBox()->isEmpty() );
	buildEdit->setEnabled( !buildList->getListBox()->isEmpty() );

	updateBuildType();

	updateRunConfig();

	if ( !buildList->hasEventsOfType( Event::OnItemSelected ) ) {
		buildList->on( Event::OnItemSelected, [this, buildEdit, buildList]( const Event* ) {
			mConfig.buildName = buildList->getListBox()->getItemSelectedText();
			mConfig.buildType = "";
			mConfig.runName = "";
			buildEdit->setEnabled( true );
			updateBuildType();
			updateRunConfig();
		} );
	}

	buildButton->setEnabled( !mConfig.buildName.empty() && hasBuild( mConfig.buildName ) &&
							 hasBuildCommands( mConfig.buildName ) );

	cleanButton->setEnabled( !mConfig.buildName.empty() && hasBuild( mConfig.buildName ) &&
							 hasCleanCommands( mConfig.buildName ) );

	buildAndRun->setEnabled( !mConfig.buildName.empty() && hasBuild( mConfig.buildName ) &&
							 hasBuildCommands( mConfig.buildName ) );

	buildButton->setTooltipTextIfNotEmpty( mApp->getKeybind( "project-build-start-cancel" ) );
	buildButton->setTooltipTextIfNotEmpty( mApp->getKeybind( "project-build-clean" ) );
	runButton->setTooltipTextIfNotEmpty( mApp->getKeybind( "project-run-executable" ) );
	buildAndRun->setTooltipTextIfNotEmpty( mApp->getKeybind( "project-build-and-run" ) );

	if ( !buildAndRun->hasEventsOfType( Event::MouseClick ) ) {
		buildAndRun->onClick( [this]( auto ) { mApp->runCommand( "project-build-and-run" ); } );
	}

	if ( !buildButton->hasEventsOfType( Event::MouseClick ) ) {
		buildButton->onClick( [this]( auto ) {
			if ( isBuilding() ) {
				cancelBuild();
			} else {
				buildCurrentConfig( mApp->getStatusBuildOutputController() );
			}
		} );
	}

	if ( !cleanButton->hasEventsOfType( Event::MouseClick ) ) {
		cleanButton->onClick( [this]( auto ) {
			if ( isBuilding() ) {
				cancelBuild();
			} else {
				cleanCurrentConfig( mApp->getStatusBuildOutputController() );
			}
		} );
	}

	if ( !runButton->hasEventsOfType( Event::MouseClick ) ) {
		runButton->onClick( [this]( auto ) {
			if ( isRunningApp() ) {
				cancelRun();
			} else {
				runCurrentConfig( mApp->getStatusAppOutputController(), false );
			}
		} );
	}

	if ( !buildAdd->hasEventsOfType( Event::MouseClick ) ) {
		buildAdd->onClick( [this, buildTab]( auto ) { addBuild( buildTab ); } );
	}

	if ( !buildEdit->hasEventsOfType( Event::MouseClick ) ) {
		buildEdit->onClick(
			[this, buildTab]( auto ) { editBuild( mConfig.buildName, buildTab ); } );
	}
}

void ProjectBuildManager::updateBuildType() {
	if ( mTab == nullptr )
		return;
	UIWidget* buildTab = mTab->getOwnedWidget()->find<UIWidget>( "build_tab_view" );
	if ( buildTab == nullptr )
		return;
	UIDropDownList* buildList = buildTab->find<UIDropDownList>( "build_list" );
	UIDropDownList* buildTypeList = buildTab->find<UIDropDownList>( "build_type_list" );

	buildTypeList->getListBox()->clear();

	String first = buildList->getListBox()->getItemSelectedText();
	if ( !first.empty() ) {
		auto foundIt = mBuilds.find( first );
		if ( foundIt != mBuilds.end() ) {
			const auto& buildTypes = foundIt->second.buildTypes();
			std::vector<String> buildTypesItems;
			for ( const auto& buildType : buildTypes )
				buildTypesItems.emplace_back( buildType );
			buildTypeList->getListBox()->addListBoxItems( buildTypesItems );
			if ( buildTypeList->getListBox()->getItemIndex( mConfig.buildType ) !=
				 eeINDEX_NOT_FOUND ) {
				buildTypeList->getListBox()->setSelected( mConfig.buildType );
			} else if ( !buildTypeList->getListBox()->isEmpty() ) {
				buildTypeList->getListBox()->setSelected( 0 );
				mConfig.buildType = buildTypeList->getListBox()->getItemSelectedText();
			}
		}
	}
	buildTypeList->setEnabled( !buildTypeList->getListBox()->isEmpty() );

	if ( !buildTypeList->hasEventsOfType( Event::OnItemSelected ) ) {
		buildTypeList->on( Event::OnItemSelected, [this, buildTypeList]( const Event* ) {
			mConfig.buildType = buildTypeList->getListBox()->getItemSelectedText();
		} );
	}
}

void ProjectBuildManager::updateRunConfig() {
	if ( mTab == nullptr )
		return;
	UIWidget* buildTab = mTab->getOwnedWidget()->find<UIWidget>( "build_tab_view" );
	if ( buildTab == nullptr )
		return;
	UIDropDownList* buildList = buildTab->find<UIDropDownList>( "build_list" );
	UIDropDownList* runConfigList = buildTab->find<UIDropDownList>( "run_config_list" );

	auto runName = mConfig.runName;
	runConfigList->getListBox()->clear();
	mConfig.runName = runName;

	String first = buildList->getListBox()->getItemSelectedText();
	if ( !first.empty() ) {
		auto foundIt = mBuilds.find( first );
		if ( foundIt != mBuilds.end() ) {
			const auto& runConfigs = foundIt->second.runConfigs();
			std::vector<String> items;
			size_t i = 1;
			for ( const auto& run : runConfigs ) {
				auto name = run->name.empty() ? String::format( mApp->i18n( "custom_executable_num",
																			"Custom Executable %d" )
																	.toUtf8(),
																i )
											  : run->name;
				items.emplace_back( name );
				i++;
			}
			runConfigList->getListBox()->addListBoxItems( items );
			if ( runConfigList->getListBox()->getItemIndex( mConfig.runName ) !=
				 eeINDEX_NOT_FOUND ) {
				runConfigList->getListBox()->setSelected( mConfig.runName );
			} else if ( !runConfigList->getListBox()->isEmpty() ) {
				runConfigList->getListBox()->setSelected( 0 );
				mConfig.runName = runConfigList->getListBox()->getItemSelectedText();
			}
		}
	}
	runConfigList->setEnabled( !runConfigList->getListBox()->isEmpty() );
	buildTab->find( "run_button" )->setEnabled( !runConfigList->getListBox()->isEmpty() );

	if ( !runConfigList->hasEventsOfType( Event::OnItemSelected ) ) {
		runConfigList->on( Event::OnItemSelected, [this, runConfigList, buildTab]( const Event* ) {
			mConfig.runName = runConfigList->getListBox()->getItemSelectedText();
			buildTab->find( "run_button" )->setEnabled( !runConfigList->getListBox()->isEmpty() );
		} );
	}
	if ( !runConfigList->hasEventsOfType( Event::OnClear ) ) {
		runConfigList->on( Event::OnClear, [this, runConfigList, buildTab]( const Event* ) {
			mConfig.runName = "";
			buildTab->find( "run_button" )->setEnabled( !runConfigList->getListBox()->isEmpty() );
		} );
	}
}

std::map<std::string, ProjectBuildOutputParser> ProjectBuildOutputParser::getPresets() {
	std::map<std::string, ProjectBuildOutputParser> presets;
	presets["generic"] = getGeneric();
	return presets;
}

bool ProjectBuildOutputParser::existsPreset( const std::string& name ) {
	auto presets = getPresets();
	return presets.find( name ) != presets.end();
}

ProjectBuildOutputParser ProjectBuildOutputParser::getGeneric() {
	ProjectBuildOutputParser parser;
	parser.mConfig.reserve( 6 );

	ProjectBuildOutputParserConfig cfg;
	cfg.pattern = "([^:]+):(%d+):(%d+):%s?[%w%s]*error:%s?(.*)";
	cfg.patternOrder.col = 3;
	cfg.patternOrder.file = 1;
	cfg.patternOrder.line = 2;
	cfg.patternOrder.message = 4;
	cfg.type = ProjectOutputParserTypes::Error;
	parser.mConfig.push_back( cfg );

	cfg.pattern = "([^:]+):(%d+):%s?[%w%s]*error:%s?(.*)";
	cfg.patternOrder.col = 0;
	cfg.type = ProjectOutputParserTypes::Error;
	parser.mConfig.push_back( cfg );

	cfg.pattern = "([^:]+):(%d+):(%d+):%s?[%w%s]*warning:%s?(.*)";
	cfg.patternOrder.col = 3;
	cfg.type = ProjectOutputParserTypes::Warning;
	parser.mConfig.push_back( cfg );

	cfg.pattern = "([^:]+):(%d+):%s?[%w%s]*warning:%s?(.*)";
	cfg.patternOrder.col = 0;
	cfg.type = ProjectOutputParserTypes::Warning;
	parser.mConfig.push_back( cfg );

	cfg.pattern = "([^:]+):(%d+):(%d+):%s?[%w%s]*notice:%s?(.*)";
	cfg.patternOrder.col = 3;
	cfg.type = ProjectOutputParserTypes::Notice;
	parser.mConfig.emplace_back( cfg );

	cfg.pattern = "([^:]+):(%d+):%s?[%w%s]*notice:%s?(.*)";
	cfg.patternOrder.col = 0;
	cfg.type = ProjectOutputParserTypes::Notice;
	parser.mConfig.emplace_back( cfg );

	return parser;
}

} // namespace ecode
