#include "projectbuild.hpp"
#include "ecode.hpp"
#include "scopedop.hpp"
#include "statusbuildoutputcontroller.hpp"
#include <eepp/core/string.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/process.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace EE::Scene;

/** @return The process environment variables */
static std::unordered_map<std::string, std::string> getEnvironmentVariables() {
	std::unordered_map<std::string, std::string> ret;
	char** env;
#if defined( WIN ) && ( _MSC_VER >= 1900 )
	env = *__p__environ();
#else
	extern char** environ;
	env = environ;
#endif

	for ( ; *env; ++env ) {
		auto var = String::split( *env, "=" );

		if ( var.size() == 2 ) {
			ret[var[0]] = var[1];
		} else if ( var.size() > 2 ) {
			auto val( var[1] );
			for ( size_t i = 2; i < var.size(); ++i )
				val += var[i];
			ret[var[0]] = val;
		}
	}

	return ret;
}

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
}

void ProjectBuild::replaceVars() {
	const std::vector<ProjectBuildSteps*> steps{ &mBuild, &mClean };
	for ( auto& step : steps ) {
		for ( auto& s : *step ) {
			replaceVar( s, VAR_PROJECT_ROOT, mProjectRoot );
			for ( auto& var : mVars ) {
				std::string varKey( "${" + var.first + "}" );
				String::replaceAll( var.second, VAR_PROJECT_ROOT, mProjectRoot );
				replaceVar( s, varKey, var.second );
			}
		}
	}
}

bool ProjectBuild::isOSSupported( const std::string& os ) const {
	return mOS.empty() || std::any_of( mOS.begin(), mOS.end(), [&]( const auto& oos ) {
			   return oos == os || oos == "any";
		   } );
}

ProjectBuildManager::ProjectBuildManager( const std::string& projectRoot,
										  std::shared_ptr<ThreadPool> pool, UITabWidget* sidePanel,
										  App* app ) :
	mProjectRoot( projectRoot ), mThreadPool( pool ), mSidePanel( sidePanel ), mApp( app ) {
	FileSystem::dirAddSlashAtEnd( mProjectRoot );

	if ( mThreadPool ) {
		mThreadPool->run( [this]() { load(); } );
	} else {
		load();
	}
}

ProjectBuildManager::~ProjectBuildManager() {
	if ( mUISceneNode && !SceneManager::instance()->isShuttingDown() && mSidePanel && mTab ) {
		mSidePanel->removeTab( mTab );
	}
	mShuttingDown = true;
	mCancelBuild = true;
	while ( mLoading )
		Sys::sleep( Milliseconds( 0.1f ) );
	while ( mBuilding )
		Sys::sleep( Milliseconds( 0.1f ) );
}

ProjectBuildCommandsRes ProjectBuildManager::generateBuildCommands( const std::string& buildName,
																	const ProjectBuildi18nFn& i18n,
																	const std::string& buildType ) {
	if ( !mLoaded )
		return { i18n( "project_build_not_loaded", "No project build loaded!" ) };

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

	for ( const auto& step : build.mBuild ) {
		ProjectBuildCommand buildCmd( step, build.mEnvs );
		replaceVar( buildCmd, VAR_OS, currentOS );
		replaceVar( buildCmd, VAR_NPROC, nproc );
		if ( !buildType.empty() )
			replaceVar( buildCmd, VAR_BUILD_TYPE, buildType );
		buildCmd.config = build.mConfig;
		res.cmds.emplace_back( std::move( buildCmd ) );
	}

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
	if ( !mLoaded )
		return { i18n( "project_build_not_loaded", "No project build loaded!" ) };

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

	for ( const auto& step : build.mClean ) {
		ProjectBuildCommand buildCmd( step, build.mEnvs );
		replaceVar( buildCmd, VAR_OS, currentOS );
		replaceVar( buildCmd, VAR_NPROC, nproc );
		if ( !buildType.empty() )
			replaceVar( buildCmd, VAR_BUILD_TYPE, buildType );
		buildCmd.config = build.mConfig;
		res.cmds.emplace_back( std::move( buildCmd ) );
	}

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

	for ( const auto& build : j.items() ) {
		ProjectBuild b( build.key(), mProjectRoot );
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
				b.mVars[var.key()] = var.value();
		}

		if ( buildObj.contains( "env" ) && buildObj["env"].is_object() ) {
			const auto& vars = buildObj["env"];
			for ( const auto& var : vars.items() )
				b.mEnvs[var.key()] = var.value();
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
						auto presets = ProjectBuildOutputParser::getPresets();
						if ( presets.find( preset ) != presets.end() ) {
							outputParser = presets[preset];
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

		b.replaceVars();

		mBuilds.insert( { build.key(), std::move( b ) } );
	}

	mLoaded = true;
	return true;
}

ProjectBuildOutputParser ProjectBuildManager::getOutputParser( const std::string& buildName ) {
	auto buildIt = mBuilds.find( buildName );
	if ( buildIt != mBuilds.end() )
		return buildIt->second.mOutputParser;
	return {};
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
	mConfig = config;
}

void ProjectBuildManager::buildCurrentConfig( StatusBuildOutputController* sboc ) {
	if ( sboc && !isBuilding() && !getBuilds().empty() ) {
		const ProjectBuild* build = nullptr;
		for ( const auto& buildIt : getBuilds() )
			if ( buildIt.second.getName() == mConfig.buildName )
				build = &buildIt.second;

		if ( build )
			sboc->runBuild( build->getName(), mConfig.buildType,
							getOutputParser( build->getName() ) );
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

void ProjectBuildManager::runBuild( const std::string& buildName, const std::string& buildType,
									const ProjectBuildi18nFn& i18n,
									const ProjectBuildCommandsRes& res,
									const ProjectBuildProgressFn& progressFn,
									const ProjectBuildDoneFn& doneFn ) {
	ScopedOp scopedOp( [this]() { mBuilding = true; }, [this]() { mBuilding = false; } );
	Clock clock;

	auto printElapsed = [&clock, &i18n, &progressFn]() {
		if ( progressFn ) {
			progressFn(
				100, Sys::getDateTimeStr() + ": " +
						 String::format(
							 i18n( "build_elapsed_time", "Elapsed Time: %s.\n" ).toUtf8().c_str(),
							 clock.getElapsedTime().toString().c_str() ) );
		}
	};

	if ( progressFn ) {
		progressFn( 0, Sys::getDateTimeStr() + ": " +
						   String::format( i18n( "running_steps_for_project",
												 "Running steps for project %s...\n" )
											   .toUtf8()
											   .c_str(),
										   buildName.c_str() ) );

		if ( !buildType.empty() )
			progressFn(
				0, Sys::getDateTimeStr() + ": " +
					   String::format(
						   i18n( "using_build_type", "Using build type: %s.\n" ).toUtf8().c_str(),
						   buildType.c_str() ) );
	}

	int c = 0;
	int totSteps = 0;
	for ( const auto& cmd : res.cmds )
		totSteps += cmd.enabled ? 1 : 0;

	for ( const auto& cmd : res.cmds ) {
		int progress = c > 0 ? c / (Float)totSteps : 0;
		mProcess = std::make_unique<Process>();
		auto options = Process::SearchUserPath | Process::NoWindow | Process::CombinedStdoutStderr;
		std::unordered_map<std::string, std::string> env;
		if ( !cmd.config.clearSysEnv ) {
			if ( !env.empty() ) {
				env = getEnvironmentVariables();
				env.insert( cmd.envs.begin(), cmd.envs.end() );
			} else {
				options |= Process::InheritEnvironment;
			}
		} else {
			env = cmd.envs;
		}

		if ( !cmd.enabled ) {
			c++;
			continue;
		}

		if ( mProcess->create( cmd.cmd, cmd.args, options, env, cmd.workingDir ) ) {
			if ( progressFn )
				progressFn( progress,
							Sys::getDateTimeStr() + ": " +
								String::format(
									i18n( "starting_process", "Starting %s %s\n" ).toUtf8().c_str(),
									cmd.cmd.c_str(), cmd.args.c_str() ) );

			std::string buffer( 1024, '\0' );
			unsigned bytesRead = 0;
			int returnCode;
			do {
				bytesRead = mProcess->readStdOut( buffer );
				std::string data( buffer.substr( 0, bytesRead ) );
				if ( progressFn )
					progressFn( progress, std::move( data ) );
			} while ( bytesRead != 0 && mProcess->isAlive() && !mShuttingDown && !mCancelBuild );

			if ( mShuttingDown || mCancelBuild ) {
				mProcess->kill();
				mCancelBuild = false;
				printElapsed();
				if ( doneFn )
					doneFn( EXIT_FAILURE );
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
												cmd.cmd.c_str() ) );
				}
				printElapsed();
				if ( doneFn )
					doneFn( returnCode );
				return;
			} else {
				if ( progressFn ) {
					progressFn( progress,
								String::format( i18n( "process_exited_normally",
													  "The process \"%s\" exited normally.\n" )
													.toUtf8()
													.c_str(),
												cmd.cmd.c_str() ) );
				}
			}
		} else {
			printElapsed();
			if ( doneFn )
				doneFn( EXIT_FAILURE );
			return;
		}

		c++;
	}

	printElapsed();
	if ( doneFn )
		doneFn( EXIT_SUCCESS );
}

void ProjectBuildManager::buildSidePanelTab() {
	mUISceneNode = mSidePanel->getUISceneNode();
	UIIcon* icon = mUISceneNode->findIcon( "symbol-property" );
	UIWidget* node = mUISceneNode->loadLayoutFromString(
		R"html(
			<style>
			#build_tab {
				background-color: var(--list-back);
			}
			</style>
			<ScrollView id="build_tab" lw="mp" lh="mp">
				<vbox lw="mp" lh="wc" padding="4dp">
					<TextView text="@string(build_settings, Build Settings)" font-size="15dp" />
					<TextView text="@string(build_configuration, Build Configuration)" />
					<DropDownList id="build_list" layout_width="mp" layout_height="wrap_content" margin-bottom="4dp" />
					<!--
					<hbox lw="mp" lh="wc">
						<PushButton lw="0" lw8="0.5" id="build_edit" text="@string(edit_build, Edit Build)" margin-right="2dp" />
						<PushButton lw="0" lw8="0.5" id="build_add" text="@string(add_build, Add Build)" margin-left="2dp" />
					</hbox>
					-->
					<TextView text="@string(build_target, Build Target)" margin-top="8dp" />
					<hbox lw="mp" lh="wc">
						<DropDownList id="build_type_list" layout_width="0" layout_weight="1" layout_height="wrap_content" />
				<!--	<PushButton id="build_type_add" text="@string(add_build, Add Build)" text-as-fallback="true" icon="icon(add, 12dp)"  margin-left="2dp"/> -->
					</hbox>
					<PushButton id="build_button" lw="mp" lh="wc" text="@string(build, Build)" margin-top="8dp" icon="icon(hammer, 12dp)" />
					<PushButton id="clean_button" lw="mp" lh="wc" text="@string(clean, Clean)" margin-top="8dp" icon="icon(eraser, 12dp)" />
				</vbox>
			</ScrollView>
		)html" );
	mTab = mSidePanel->add( mUISceneNode->getTranslatorStringFromKey( "build", "Build" ), node,
							icon ? icon->getSize( PixelDensity::dpToPx( 12 ) ) : nullptr );
	mTab->setTextAsFallback( true );

	updateSidePanelTab();
}

void ProjectBuildManager::updateSidePanelTab() {
	UIWidget* buildTab = mTab->getOwnedWidget()->find<UIWidget>( "build_tab" );
	UIDropDownList* buildList = buildTab->find<UIDropDownList>( "build_list" );
	UIPushButton* buildButton = buildTab->find<UIPushButton>( "build_button" );
	UIPushButton* cleanButton = buildTab->find<UIPushButton>( "clean_button" );

	buildList->getListBox()->clear();

	String first = mConfig.buildName;
	std::vector<String> buildNamesItems;
	for ( const auto& build : mBuilds ) {
		buildNamesItems.push_back( build.first );
		if ( first.empty() )
			first = build.first;
	}

	buildList->getListBox()->addListBoxItems( buildNamesItems );

	if ( !first.empty() && buildList->getListBox()->getItemIndex( first ) != eeINDEX_NOT_FOUND ) {
		buildList->getListBox()->setSelected( first );
		mConfig.buildName = first;
	} else if ( !buildList->getListBox()->isEmpty() ) {
		buildList->getListBox()->setSelected( 0L );
		mConfig.buildName = buildList->getListBox()->getItemSelectedText();
	}

	buildList->setEnabled( !buildList->getListBox()->isEmpty() );

	updateBuildType();

	buildList->removeEventsOfType( Event::OnItemSelected );
	buildList->addEventListener( Event::OnItemSelected, [this, buildList]( const Event* ) {
		mConfig.buildName = buildList->getListBox()->getItemSelectedText();
		mConfig.buildType = "";
		updateBuildType();
	} );

	buildButton->setEnabled( !mConfig.buildName.empty() && hasBuild( mConfig.buildName ) &&
							 hasBuildCommands( mConfig.buildName ) );

	cleanButton->setEnabled( !mConfig.buildName.empty() && hasBuild( mConfig.buildName ) &&
							 hasCleanCommands( mConfig.buildName ) );

	buildButton->addMouseClickListener(
		[this]( const Event* ) {
			if ( isBuilding() ) {
				cancelBuild();
			} else {
				buildCurrentConfig( mApp->getStatusBuildOutputController() );
			}
		},
		MouseButton::EE_BUTTON_LEFT );

	cleanButton->addMouseClickListener(
		[this]( const Event* ) {
			if ( isBuilding() ) {
				cancelBuild();
			} else {
				cleanCurrentConfig( mApp->getStatusBuildOutputController() );
			}
		},
		MouseButton::EE_BUTTON_LEFT );
}

void ProjectBuildManager::updateBuildType() {
	UIWidget* buildTab = mTab->getOwnedWidget()->find<UIWidget>( "build_tab" );
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

	buildTypeList->removeEventsOfType( Event::OnItemSelected );
	buildTypeList->addEventListener( Event::OnItemSelected, [this, buildTypeList]( const Event* ) {
		mConfig.buildType = buildTypeList->getListBox()->getItemSelectedText();
	} );
}

std::map<std::string, ProjectBuildOutputParser> ProjectBuildOutputParser::getPresets() {
	std::map<std::string, ProjectBuildOutputParser> presets;
	presets["generic"] = getGeneric();
	return presets;
}

ProjectBuildOutputParser ProjectBuildOutputParser::getGeneric() {
	ProjectBuildOutputParser parser;

	ProjectBuildOutputParserConfig cfg;
	cfg.pattern = "([^:]+):(%d+):(%d+):%s?[%w%s]*error:%s?(.*)";
	cfg.patternOrder.col = 3;
	cfg.patternOrder.file = 1;
	cfg.patternOrder.line = 2;
	cfg.patternOrder.message = 4;
	cfg.type = ProjectOutputParserTypes::Error;
	parser.mConfig.push_back( cfg );

	cfg.pattern = "([^:]+):(%d+):(%d+):%s?[%w%s]*warning:%s?(.*)";
	cfg.type = ProjectOutputParserTypes::Warning;
	parser.mConfig.push_back( cfg );

	cfg.pattern = "([^:]+):(%d+):(%d+):%s?[%w%s]*notice:%s?(.*)";
	cfg.type = ProjectOutputParserTypes::Notice;
	parser.mConfig.emplace_back( cfg );

	return parser;
}

} // namespace ecode
