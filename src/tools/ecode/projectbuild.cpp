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

static const char* VAR_PROJECT_ROOT = "${project_root}";
static const char* VAR_BUILD_TYPE = "${build_type}";
static const char* VAR_OS = "${os}";
static const char* VAR_NPROC = "${nproc}";

static void replaceVar( ProjectBuildStep& s, const std::string& var, const std::string& val ) {
	static std::string slashDup = FileSystem::getOSSlash() + FileSystem::getOSSlash();
	String::replaceAll( s.workingDir, var, val );
	String::replaceAll( s.cmd, var, val );
	String::replaceAll( s.args, var, val );
	String::replaceAll( s.workingDir, slashDup, FileSystem::getOSSlash() );
	FileSystem::dirAddSlashAtEnd( s.workingDir );
}

ProjectBuildSteps ProjectBuild::replaceVars( const ProjectBuildSteps& steps ) const {
	ProjectBuildSteps newSteps( steps );
	for ( auto& s : newSteps ) {
		replaceVar( s, VAR_PROJECT_ROOT, mProjectRoot );
		for ( auto& var : mVars ) {
			std::string varKey( "${" + var.first + "}" );
			std::string varVal( var.second );
			String::replaceAll( varVal, VAR_PROJECT_ROOT, mProjectRoot );
			replaceVar( s, varKey, varVal );
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
			step["working_dir"] = build.workingDir;
			step["args"] = build.args;
			step["command"] = build.cmd;
			if ( !build.enabled )
				step["enabled"] = build.enabled;
			jbuild.push_back( step );
		}

		bj["clean"] = json::array();
		auto& jclean = bj["clean"];
		for ( const auto& build : curBuild.cleanSteps() ) {
			json step;
			step["working_dir"] = build.workingDir;
			step["args"] = build.args;
			step["command"] = build.cmd;
			if ( !build.enabled )
				step["enabled"] = build.enabled;
			jclean.push_back( step );
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
	mNewBuild( mApp->i18n( "new_name", "new_name" ), mProjectRoot ) {
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
	mNewBuild = ProjectBuild( mApp->i18n( "new_name", "new_name" ), mProjectRoot );
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
		widget->asType<UITab>()->select();
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

ProjectBuildManager::~ProjectBuildManager() {
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
	save();
	while ( mLoading )
		Sys::sleep( Milliseconds( 0.1f ) );
	while ( mBuilding )
		Sys::sleep( Milliseconds( 0.1f ) );
}

ProjectBuildCommandsRes ProjectBuildManager::generateBuildCommands( const std::string& buildName,
																	const ProjectBuildi18nFn& i18n,
																	const std::string& buildType ) {
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

	std::string nproc = String::format( "%d", Sys::getCPUCount() );
	ProjectBuildCommandsRes res;

	auto finalBuild( build.replaceVars( build.mBuild ) );

	for ( const auto& step : finalBuild ) {
		ProjectBuildCommand buildCmd( step );
		replaceVar( buildCmd, VAR_OS, currentOS );
		replaceVar( buildCmd, VAR_NPROC, nproc );
		if ( buildCmd.workingDir.empty() )
			buildCmd.workingDir = mProjectRoot;
		if ( !buildType.empty() )
			replaceVar( buildCmd, VAR_BUILD_TYPE, buildType );
		buildCmd.config = build.mConfig;
		res.cmds.emplace_back( std::move( buildCmd ) );
	}

	res.envs = build.mEnvs;

	return res;
}

ProjectBuildCommandsRes ProjectBuildManager::build( const std::string& buildName,
													const ProjectBuildi18nFn& i18n,
													const std::string& buildType,
													const ProjectBuildProgressFn& progressFn,
													const ProjectBuildDoneFn& doneFn ) {
	ProjectBuildCommandsRes res = generateBuildCommands( buildName, i18n, buildType );
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
};

ProjectBuildCommandsRes ProjectBuildManager::generateCleanCommands( const std::string& buildName,
																	const ProjectBuildi18nFn& i18n,
																	const std::string& buildType ) {
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

	std::string nproc = String::format( "%d", Sys::getCPUCount() );
	ProjectBuildCommandsRes res;

	auto finalBuild( build.replaceVars( build.mClean ) );

	for ( const auto& step : finalBuild ) {
		ProjectBuildCommand buildCmd( step );
		replaceVar( buildCmd, VAR_OS, currentOS );
		replaceVar( buildCmd, VAR_NPROC, nproc );
		if ( !buildType.empty() )
			replaceVar( buildCmd, VAR_BUILD_TYPE, buildType );
		buildCmd.config = build.mConfig;
		res.cmds.emplace_back( std::move( buildCmd ) );
	}

	res.envs = build.mEnvs;

	return res;
}

ProjectBuildCommandsRes ProjectBuildManager::clean( const std::string& buildName,
													const ProjectBuildi18nFn& i18n,
													const std::string& buildType,
													const ProjectBuildProgressFn& progressFn,
													const ProjectBuildDoneFn& doneFn ) {
	ProjectBuildCommandsRes res = generateCleanCommands( buildName, i18n, buildType );
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
};

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
				ProjectBuildStep bstep;
				bstep.cmd = step.value( "command", "" );
				bstep.args = step.value( "args", "" );
				bstep.workingDir = step.value( "working_dir", "" );
				bstep.enabled = step.value( "enabled", true );
				b.mBuild.emplace_back( std::move( bstep ) );
			}
		}

		if ( buildObj.contains( "clean" ) && buildObj["clean"].is_array() ) {
			const auto& buildArray = buildObj["clean"];
			for ( const auto& step : buildArray ) {
				ProjectBuildStep bstep;
				bstep.cmd = step.value( "command", "" );
				bstep.args = step.value( "args", "" );
				bstep.workingDir = step.value( "working_dir", "" );
				bstep.enabled = step.value( "enabled", true );
				b.mClean.emplace_back( std::move( bstep ) );
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
							   mSidePanel->runOnMainThread( [this]() { buildSidePanelTab(); } );
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
	if ( mProcess ) {
		mProcess->destroy();
		mProcess->kill();
	}
}

ProjectBuildConfiguration ProjectBuildManager::getConfig() const {
	return mConfig;
}

void ProjectBuildManager::setConfig( const ProjectBuildConfiguration& config ) {
	bool buildNameChanged = false;
	bool buildTypeChanged = false;

	if ( mConfig.buildName != config.buildName ) {
		mConfig.buildName = config.buildName;
		buildNameChanged = true;
		updateSidePanelTab();
	}

	if ( mConfig.buildType != config.buildType ) {
		mConfig.buildType = config.buildType;
		buildTypeChanged = true;
		updateBuildType();
	}

	if ( buildNameChanged )
		updateSidePanelTab();
	else if ( buildTypeChanged )
		updateBuildType();
}

void ProjectBuildManager::buildCurrentConfig( StatusBuildOutputController* sboc ) {
	if ( sboc && !isBuilding() && !getBuilds().empty() ) {
		const ProjectBuild* build = nullptr;
		for ( const auto& buildIt : getBuilds() )
			if ( buildIt.second.getName() == mConfig.buildName )
				build = &buildIt.second;

		if ( build ) {
			mApp->saveAll();
			sboc->runBuild( build->getName(), mConfig.buildType,
							getOutputParser( build->getName() ) );
		}
	}
}

void ProjectBuildManager::cleanCurrentConfig( StatusBuildOutputController* sboc ) {
	if ( sboc && !isBuilding() && !getBuilds().empty() ) {
		const ProjectBuild* build = nullptr;
		for ( const auto& buildIt : getBuilds() )
			if ( buildIt.second.getName() == mConfig.buildName )
				build = &buildIt.second;

		if ( build )
			sboc->runClean( build->getName(), mConfig.buildType,
							getOutputParser( build->getName() ) );
	}
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
						String::format(
							i18n( "running_steps_for_project", "Running steps for project %s...\n" )
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
		mProcess = std::make_unique<Process>();
		auto options = Process::SearchUserPath | Process::NoWindow | Process::CombinedStdoutStderr;
		if ( !cmd.config.clearSysEnv )
			options |= Process::InheritEnvironment;

		if ( !cmd.enabled ) {
			c++;
			continue;
		}

		if ( progressFn ) {
			progressFn(
				progress,
				Sys::getDateTimeStr() + ": " +
					String::format( i18n( "starting_process", "Starting %s %s\n" ).toUtf8().c_str(),
									cmd.cmd.c_str(), cmd.args.c_str() ),
				nullptr );

			progressFn(
				progress,
				Sys::getDateTimeStr() + ": " +
					String::format( i18n( "working_dir_at", "Working Dir %s\n" ).toUtf8().c_str(),
									cmd.workingDir.c_str() ),
				nullptr );
		}

		if ( mProcess->create( cmd.cmd, cmd.args, options, toUnorderedMap( res.envs ),
							   cmd.workingDir ) ) {
			std::string buffer( 1024, '\0' );
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

void ProjectBuildManager::buildSidePanelTab() {
	mUISceneNode = mSidePanel->getUISceneNode();
	UIIcon* icon = mUISceneNode->findIcon( "symbol-property" );
	UIWidget* node = mUISceneNode->loadLayoutFromString(
		R"html(
			<ScrollView id="build_tab_view" lw="mp" lh="mp">
				<vbox lw="mp" lh="wc" padding="4dp">
					<TextView text="@string(build_settings, Build Settings)" font-size="15dp" />
					<TextView text="@string(build_configuration, Build Configuration)" />
					<hbox lw="mp" lh="wc" margin-bottom="4dp">
						<DropDownList id="build_list" layout_width="0" lw8="1" layout_height="wrap_content" />
						<PushButton id="build_edit" id="build_edit" text="@string(edit_build, Edit Build)" tooltip="@string(edit_build, Edit Build)"  text-as-fallback="true" icon="icon(file-edit, 12dp)" margin-left="2dp" />
						<PushButton id="build_add" id="build_add" text="@string(add_build, Add Build)" tooltip="@string(add_build, Add Build)" text-as-fallback="true" icon="icon(add, 12dp)" margin-left="2dp" />
					</hbox>
					<TextView text="@string(build_target, Build Target)" margin-top="8dp" />
					<DropDownList lw="mp" id="build_type_list" />
					<PushButton id="build_button" lw="mp" lh="wc" text="@string(build, Build)" margin-top="8dp" icon="icon(hammer, 12dp)" />
					<PushButton id="clean_button" lw="mp" lh="wc" text="@string(clean, Clean)" margin-top="8dp" icon="icon(eraser, 12dp)" />
				</vbox>
			</ScrollView>
		)html" );
	mTab = mSidePanel->add( mUISceneNode->i18n( "build", "Build" ), node,
							icon ? icon->getSize( PixelDensity::dpToPx( 12 ) ) : nullptr );
	mTab->setId( "build_tab" );
	mTab->setTextAsFallback( true );

	auto tabIndex = mSidePanel->getTabIndex( mTab );
	if ( tabIndex > 0 ) {
		auto prevTab = mSidePanel->getTab( tabIndex - 1 );
		if ( prevTab && prevTab->getId() != "treeview_tab" )
			mSidePanel->swapTabs( mTab, prevTab );
	}

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
	UIPushButton* buildAdd = buildTab->find<UIPushButton>( "build_add" );
	UIPushButton* buildEdit = buildTab->find<UIPushButton>( "build_edit" );

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

	if ( !buildList->hasEventsOfType( Event::OnItemSelected ) ) {
		buildList->on( Event::OnItemSelected, [this, buildEdit, buildList]( const Event* ) {
			mConfig.buildName = buildList->getListBox()->getItemSelectedText();
			mConfig.buildType = "";
			buildEdit->setEnabled( true );
			updateBuildType();
		} );
	}

	buildButton->setEnabled( !mConfig.buildName.empty() && hasBuild( mConfig.buildName ) &&
							 hasBuildCommands( mConfig.buildName ) );

	cleanButton->setEnabled( !mConfig.buildName.empty() && hasBuild( mConfig.buildName ) &&
							 hasCleanCommands( mConfig.buildName ) );

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

	if ( !buildTypeList->hasEventsOfType( Event::OnItemSelected ) )
		buildTypeList->on( Event::OnItemSelected, [this, buildTypeList]( const Event* ) {
			mConfig.buildType = buildTypeList->getListBox()->getItemSelectedText();
		} );
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
