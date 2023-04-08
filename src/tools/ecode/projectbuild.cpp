#include "projectbuild.hpp"
#include "scopedop.hpp"
#include <eepp/core/string.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/process.hpp>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace EE;

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
										  std::shared_ptr<ThreadPool> pool ) :
	mProjectRoot( projectRoot ), mThreadPool( pool ) {
	FileSystem::dirAddSlashAtEnd( mProjectRoot );

	if ( mThreadPool ) {
		mThreadPool->run( [this]() { load(); } );
	} else {
		load();
	}
}

ProjectBuildManager::~ProjectBuildManager() {
	mShuttingDown = true;
	while ( mLoading )
		Sys::sleep( Milliseconds( 0.1f ) );
	while ( mBuilding )
		Sys::sleep( Milliseconds( 0.1f ) );
}

ProjectBuildCommandsRes
ProjectBuildManager::run( const std::string& buildName,
						  std::function<String( const std::string&, const String& )> i18n,
						  const std::string& buildType, const ProjectBuildProgressFn& progressFn,
						  const ProjectBuildDoneFn& doneFn ) {
	ProjectBuildCommandsRes res = generateBuildCommands( buildName, i18n, buildType );
	if ( !res.isValid() )
		return res;
	if ( !mThreadPool ) {
		res.errorMsg = i18n( "no_threads", "Threaded ecode required to compile builds." );
		return res;
	}

	mThreadPool->run( [this, res, progressFn, doneFn]() { runBuild( res, progressFn, doneFn ); } );

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
	ScopedOp scopedOp( [this]() { mLoading = true; }, [this]() { mLoading = false; } );

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
					"path %s, error: ",
					mProjectFile.c_str(), e.what() );
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
			b.mConfig.enabled = buildObj.value( "enabled", true );
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
				b.mClean.emplace_back( std::move( bstep ) );
			}
		}

		if ( buildObj.contains( "output_parser" ) && buildObj["output_parser"].is_object() ) {
			const auto& op = buildObj["output_parser"];

			ProjectBuildOutputParser outputParser;

			for ( const auto& item : op.items() ) {
				if ( item.key() == "config" ) {
					const auto& config = item.value();
					outputParser.mRelativeFilePaths = config.value( "output_parser", true );
				} else {
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
		}

		b.replaceVars();

		mBuilds.insert( { build.key(), std::move( b ) } );
	}

	mLoaded = true;
	return true;
}

ProjectBuildCommandsRes ProjectBuildManager::generateBuildCommands(
	const std::string& buildName,
	std::function<String( const std::string& /*key*/, const String& /*defaultvalue*/ )> i18n,
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

void ProjectBuildManager::runBuild( const ProjectBuildCommandsRes& res,
									const ProjectBuildProgressFn& progressFn,
									const ProjectBuildDoneFn& doneFn ) {
	ScopedOp scopedOp( [this]() { mBuilding = true; }, [this]() { mBuilding = false; } );
	Clock clock;
	for ( const auto& cmd : res.cmds ) {
		Process process;
		auto options = Process::SearchUserPath | Process::NoWindow | Process::CombinedStdoutStderr;
		if ( !cmd.config.clearSysEnv )
			options |= Process::InheritEnvironment;
		if ( process.create( cmd.cmd, cmd.args, options, cmd.envs, cmd.workingDir ) ) {
			std::string buffer( 1024, '\0' );
			unsigned bytesRead = 0;
			int returnCode;
			do {
				bytesRead = process.readStdOut( buffer );
				std::string data( buffer.substr( 0, bytesRead ) );
				if ( progressFn )
					progressFn( 50, std::move( data ) );
			} while ( bytesRead != 0 && process.isAlive() && !mShuttingDown );

			if ( mShuttingDown ) {
				process.kill();
				doneFn( EXIT_FAILURE );
				return;
			}

			if ( progressFn )
				progressFn( 90, {} );

			process.join( &returnCode );
			process.destroy();

			if ( doneFn && returnCode != EXIT_SUCCESS ) {
				progressFn( 100, {} );
				doneFn( returnCode );
				return;
			}
		}
	}

	doneFn( EXIT_SUCCESS );
}

} // namespace ecode
