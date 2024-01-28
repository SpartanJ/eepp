#include "git.hpp"
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>

using namespace EE;
using namespace EE::System;

using namespace std::literals;

namespace ecode {

static size_t countLines( const std::string& text ) {
	const char* startPtr = text.c_str();
	const char* endPtr = text.c_str() + text.size();
	size_t count = 0;
	if ( startPtr != endPtr ) {
		count = 1 + *startPtr == '\n' ? 1 : 0;
		while ( ++startPtr && startPtr != endPtr )
			count += ( '\n' == *startPtr ) ? 1 : 0;
	}
	return count;
}

static void readAllLines( const std::string_view& buf,
						  std::function<void( const std::string_view& )> onLineRead ) {
	auto lastNL = 0;
	auto nextNL = buf.find_first_of( '\n' );
	while ( nextNL != std::string_view::npos ) {
		onLineRead( buf.substr( lastNL, nextNL - lastNL ) );
		lastNL = nextNL + 1;
		nextNL = buf.find_first_of( '\n', nextNL + 1 );
	}
}

static constexpr auto sNotCommitedYetHash = "0000000000000000000000000000000000000000";

Git::Blame::Blame( const std::string& error ) : error( error ), line( 0 ) {}

Git::Blame::Blame( std::string&& author, std::string&& authorEmail, std::string&& date,
				   std::string&& commitHash, std::string&& commitShortHash,
				   std::string&& commitMessage, std::size_t line ) :
	author( std::move( author ) ),
	authorEmail( std::move( authorEmail ) ),
	date( std::move( date ) ),
	commitHash( std::move( commitHash ) ),
	commitShortHash( std::move( commitShortHash ) ),
	commitMessage( std::move( commitMessage ) ),
	line( line ) {}

Git::Git( const std::string& projectDir, const std::string& gitPath ) : mGitPath( gitPath ) {
	if ( gitPath.empty() )
		mGitPath = Sys::which( "git" );
	if ( !projectDir.empty() )
		setProjectPath( projectDir );
}

int Git::git( const std::string& args, const std::string& projectDir, std::string& buf ) const {
	Clock clock;
	buf.clear();
	Process p;
	p.create( mGitPath, args,
			  Process::CombinedStdoutStderr | Process::Options::NoWindow |
				  Process::Options::InheritEnvironment,
			  { { "LC_ALL", "en_US.UTF-8" } }, projectDir.empty() ? mProjectPath : projectDir );
	p.readAllStdOut( buf );
	int retCode = 0;
	p.join( &retCode );
	if ( mLogLevel == LogLevel::Info ||
		 ( mLogLevel >= LogLevel::Warning && retCode != EXIT_SUCCESS ) ) {
		Log::instance()->writef( mLogLevel, "GitPlugin cmd in %s (%d): %s %s",
								 clock.getElapsedTime().toString(), retCode, mGitPath, args );
	}
	return retCode;
}

void Git::gitSubmodules( const std::string& args, const std::string& projectDir,
						 std::string& buf ) {
	git( String::format( "submodule foreach \"git %s\"", args ), projectDir, buf );
}

bool Git::isGitRepo( const std::string& projectDir ) {
	std::string buf;
	git( "rev-parse --is-inside-work-tree", projectDir, buf );
	String::trimInPlace( buf );
	return "true" == buf;
}

std::string Git::branch( const std::string& projectDir ) {
	std::string buf;

	if ( EXIT_SUCCESS == git( "rev-parse --abbrev-ref HEAD", projectDir, buf ) )
		return String::rTrim( buf, '\n' );

	return "HEAD";
}

std::unordered_map<std::string, std::string>
Git::branches( const std::vector<std::string>& repos ) {
	std::unordered_map<std::string, std::string> ret;
	for ( const auto& repo : repos )
		ret[repo] = branch( repo );
	return ret;
}

bool Git::setProjectPath( const std::string& projectPath ) {
	auto lastProjectPath = mProjectPath;
	mProjectPath = "";
	mGitFolder = "";
	mSubModules = {};
	mSubModulesUpdated = true;
	FileInfo f( projectPath );
	if ( !f.isDirectory() )
		return false;
	std::string oriPath( f.getDirectoryPath() );
	std::string path( oriPath );
	std::string lPath;
	FileSystem::dirAddSlashAtEnd( path );
	while ( path != lPath ) {
		std::string gitFolder( path + ".git" );
		if ( FileSystem::fileExists( gitFolder ) ) {
			mProjectPath = path;
			mGitFolder = std::move( gitFolder );
			if ( lastProjectPath != mProjectPath )
				mSubModulesUpdated = false;
			return true;
		}
		lPath = path;
		path = FileSystem::removeLastFolderFromPath( path );
	}
	return false;
}

const std::string& Git::getGitPath() const {
	return mGitPath;
}

const std::string& Git::getProjectPath() const {
	return mProjectPath;
}

const std::string& Git::getGitFolder() const {
	return mGitFolder;
}

std::string Git::setSafeDirectory( const std::string& projectDir ) const {
	std::string dir( projectDir.empty() ? mProjectPath : projectDir );
	std::string buf;
	git( String::format( "config --global --add safe.directory %s", dir ), dir, buf );
	return buf;
}

Git::Result Git::pull( const std::string& projectDir ) {
	return gitSimple( "pull", projectDir );
}

Git::Result Git::push( const std::string& projectDir ) {
	return gitSimple( "push", projectDir );
}

Git::CheckoutResult Git::checkout( const std::string& branch,
								   const std::string& projectDir ) const {
	std::string buf;
	int retCode = git( String::format( "checkout %s", branch ), projectDir, buf );
	Git::CheckoutResult res;
	res.returnCode = retCode;
	res.result = buf;
	res.branch = branch;
	return res;
}

Git::CheckoutResult Git::checkoutAndCreateLocalBranch( const std::string& remoteBranch,
													   const std::string& newBranch,
													   const std::string& projectDir ) const {
	std::string newBranchName =
		newBranch.empty() ? ( remoteBranch.find_last_of( '/' ) != std::string::npos
								  ? remoteBranch.substr( remoteBranch.find_last_of( '/' ) + 1 )
								  : remoteBranch )
						  : newBranch;
	Git::CheckoutResult res;
	std::string buf;
	int retCode =
		git( String::format( "branch --no-track %s refs/remotes/%s", newBranchName, remoteBranch ),
			 projectDir, buf );
	if ( retCode != EXIT_SUCCESS ) {
		res.returnCode = retCode;
		res.result = buf;
		return res;
	}

	retCode = git( String::format( "branch --set-upstream-to=refs/remotes/%s %s", remoteBranch,
								   newBranchName ),
				   projectDir, buf );
	if ( retCode != EXIT_SUCCESS ) {
		res.returnCode = retCode;
		res.result = buf;
		return res;
	}

	return checkout( newBranchName, projectDir );
}

static std::string asList( std::vector<std::string>& files ) {
	for ( auto& file : files )
		file = "\"" + file + "\"";
	return String::join( files );
}

Git::Result Git::add( std::vector<std::string> files, const std::string& projectDir ) {
	return gitSimple( String::format( "add --force -- %s", asList( files ) ), projectDir );
}

Git::Result Git::restore( const std::string& file, const std::string& projectDir ) {
	return gitSimple( String::format( "restore \"%s\"", file ), projectDir );
}

Git::Result Git::reset( std::vector<std::string> files, const std::string& projectDir ) {
	return gitSimple( String::format( "reset -q HEAD -- %s", asList( files ) ), projectDir );
}

Git::Result Git::diff( const std::string& file, bool isStaged, const std::string& projectDir ) {
	return gitSimple( String::format( "diff%s \"%s\"", isStaged ? " --staged" : "", file ),
					  projectDir );
}

Git::Result Git::createBranch( const std::string& branchName, bool _checkout,
							   const std::string& projectDir ) {
	auto res = gitSimple( String::format( "branch --no-track %s", branchName ), projectDir );
	if ( _checkout )
		checkout( branchName );
	return res;
}

Git::Result Git::renameBranch( const std::string& branch, const std::string& newName,
							   const std::string& projectDir ) {
	return gitSimple( String::format( "branch -M %s %s", branch, newName ), projectDir );
}

Git::Result Git::deleteBranch( const std::string& branch, const std::string& projectDir ) {
	return gitSimple( String::format( "branch -D %s", branch ), projectDir );
}

Git::Result Git::commit( const std::string& commitMsg, bool ammend, bool byPassCommitHook,
						 const std::string& projectDir ) {
	auto tmpPath = Sys::getTempPath() + ".ecode-git-commit-" + String::randString( 16 );
	if ( !FileSystem::fileWrite( tmpPath, commitMsg ) ) {
		Git::Result res;
		res.returnCode = -1;
		res.result = "Could not write commit message into a file";
		return res;
	}
	std::string buf;
	std::string opts;
	if ( ammend )
		opts += " --ammend";

	if ( byPassCommitHook )
		opts += " --no-verify";

	int retCode = git(
		String::format( "commit %s --cleanup=whitespace --allow-empty --file=%s", opts, tmpPath ),
		projectDir, buf );
	FileSystem::fileRemove( tmpPath );
	Git::Result res;
	res.returnCode = retCode;
	res.result = buf;
	return res;
}

Git::Result Git::fetch( const std::string& projectDir ) {
	return gitSimple( "fetch --all --prune", projectDir );
}

Git::Result Git::fastForwardMerge( const std::string& projectDir ) {
	return gitSimple( "merge --no-commit --ff --ff-only", projectDir );
}

Git::Result Git::updateRef( const std::string& headBranch, const std::string& toCommit,
							const std::string& projectDir ) {
	return gitSimple( String::format( "update-ref refs/heads/%s %s", headBranch, toCommit ),
					  projectDir );
}

Git::CountResult Git::branchHistoryPosition( const std::string& localBranch,
											 const std::string& remoteBranch,
											 const std::string& projectDir ) {
	std::string buf;
	int retCode =
		git( String::format( "rev-list --left-right --count %s...%s", localBranch, remoteBranch ),
			 projectDir, buf );
	Git::CountResult res;
	res.returnCode = retCode;
	if ( res.success() ) {
		String::trimInPlace( buf );
		auto results = String::split( buf, '\t' );
		if ( results.size() == 2 ) {
			int64_t behind = 0;
			int64_t ahead = 0;
			if ( String::fromString( ahead, results[0] ) &&
				 String::fromString( behind, results[1] ) ) {
				res.ahead = ahead;
				res.behind = behind;
			}
		}
	} else {
		res.result = buf;
		return res;
	}
	return res;
}

Git::CountResult Git::branchHistoryPosition( const Branch& branch, const std::string& projectDir ) {
	return branchHistoryPosition( branch.name, branch.remote, projectDir );
}

Git::CheckoutResult Git::checkoutNewBranch( const std::string& newBranch,
											const std::string& fromBranch,
											const std::string& projectDir ) {
	std::string buf;
	std::string args( String::format( "checkout -q -b %s", newBranch ) );
	if ( !fromBranch.empty() )
		args += " " + fromBranch;
	int retCode = git( args, projectDir, buf );
	Git::CheckoutResult res;
	res.returnCode = retCode;
	res.result = buf;
	res.branch = buf;
	return res;
}

std::vector<Git::Branch> Git::getAllBranches( const std::string& projectDir ) {
	return getAllBranchesAndTags( static_cast<RefType>( RefType::Head | RefType::Remote ),
								  projectDir );
}

static void parseAheadBehind( std::string_view aheadBehind, Git::Branch& branch ) {
	static constexpr auto BEHIND = "behind "sv;
	static constexpr auto AHEAD = "ahead "sv;
	if ( aheadBehind.empty() )
		return;
	auto split = String::split( aheadBehind, ',' );
	for ( auto s : split ) {
		s = String::trim( s );
		if ( String::startsWith( s, BEHIND ) ) {
			std::string numStr = std::string{ s.substr( BEHIND.size() ) };
			int64_t val = 0;
			if ( String::fromString( val, numStr ) )
				branch.behind = val;
		} else if ( String::startsWith( s, AHEAD ) ) {
			std::string numStr = std::string{ s.substr( AHEAD.size() ) };
			int64_t val = 0;
			if ( String::fromString( val, numStr ) )
				branch.ahead = val;
		}
	}
}

Git::Branch parseLocalBranch( const std::string_view& raw ) {
	auto split = String::split( raw, '\t', true );
	if ( split.size() < 4 )
		return {};
	std::string name( std::string{ split[1] } );
	std::string remote( std::string{ split[2] } );
	std::string commitHash( std::string{ split[3] } );
	auto ret = Git::Branch{ std::move( name ), std::move( remote ), Git::RefType::Head,
							std::move( commitHash ) };
	if ( split.size() > 4 )
		parseAheadBehind( split[4], ret );
	return ret;
}

static Git::Branch parseRemoteBranch( std::string_view raw ) {
	auto split = String::split( raw, '\t', true );
	if ( split.size() < 4 )
		return {};
	std::string name( std::string{ split[1] } );
	std::string remote( std::string{ split[1] } );
	std::string commitHash( std::string{ split[3] } );
	auto ret = Git::Branch{ std::move( name ), std::move( remote ), Git::RefType::Remote,
							std::move( commitHash ) };
	if ( split.size() > 4 )
		parseAheadBehind( split[4], ret );
	return ret;
}

static Git::Branch parseTag( std::string_view raw ) {
	auto split = String::split( raw, '\t', true );
	if ( split.size() < 4 )
		return {};
	Git::Branch newBranch;
	newBranch.name = std::string{ split[1] };
	newBranch.lastCommit = std::string{ split[3] };
	newBranch.type = Git::RefType::Tag;
	if ( split.size() > 4 )
		parseAheadBehind( split[4], newBranch );
	return newBranch;
}

std::vector<Git::Branch> Git::getAllBranchesAndTags( RefType ref, std::string_view filterBranch,
													 const std::string& projectDir ) {
	// clang-format off
	std::string args( "for-each-ref --format '%(refname)	%(refname:short)	%(upstream:short)	%(objectname)	%(upstream:track,nobracket)' --sort=v:refname" );
	// clang-format on

	if ( filterBranch.empty() ) {
		if ( ref & RefType::Head )
			args.append( " refs/heads" );
		if ( ref & RefType::Remote )
			args.append( " refs/remotes" );
		if ( ref & RefType::Tag )
			args.append( " refs/tags" );
	} else {
		args.append( " " + filterBranch );
	}

	std::vector<Branch> branches;
	std::string buf;

	if ( EXIT_SUCCESS != git( args, projectDir, buf ) )
		return branches;

	getSubModules( projectDir );

	branches.reserve( countLines( buf ) );

	readAllLines( buf, [&]( const std::string_view& line ) {
		auto branch = String::trim( String::trim( line, '\'' ), '\t' );
		if ( ( ref & Head ) && String::startsWith( branch, "refs/heads/" ) ) {
			branches.emplace_back( parseLocalBranch( branch ) );
		} else if ( ( ref & Remote ) && String::startsWith( branch, "refs/remotes/" ) ) {
			branches.emplace_back( parseRemoteBranch( branch ) );
		} else if ( ( ref & Tag ) && String::startsWith( branch, "refs/tags/" ) ) {
			branches.emplace_back( parseTag( branch ) );
		}
	} );

	return branches;
}

std::vector<std::string> Git::fetchSubModules( const std::string& projectDir ) {
	std::vector<std::string> submodules;
	std::string buf;
	FileSystem::fileGet( ( !projectDir.empty() ? projectDir : mProjectPath ) + ".gitmodules", buf );
	LuaPattern pattern( "^%s*path%s*=%s*(.+)" );
	readAllLines( buf, [&pattern, &submodules]( const std::string_view& line ) {
		LuaPattern::Range matches[2];
		if ( pattern.matches( line.data(), 0, matches, line.size() ) ) {
			submodules.emplace_back( String::trim(
				line.substr( matches[1].start, matches[1].end - matches[1].start ), '\n' ) );
		}
	} );
	return submodules;
}

std::vector<std::string> Git::getSubModules( const std::string& projectDir ) {
	Lock l( mSubModulesMutex );
	if ( !mSubModulesUpdated ) {
		mSubModules = fetchSubModules( projectDir );
		mSubModulesUpdated = true;
	}
	return mSubModules;
}

bool Git::hasSubmodules( const std::string& projectDir ) {
	return ( !projectDir.empty() && FileSystem::fileExists( projectDir + ".gitmodules" ) ) ||
		   ( !mProjectPath.empty() && FileSystem::fileExists( mProjectPath + ".gitmodules" ) );
}

std::string Git::repoName( const std::string& file, bool allowExactMatch,
						   const std::string& projectDir ) {
	for ( const auto& subRepo : mSubModules ) {
		if ( String::startsWith( file, subRepo ) &&
			 ( allowExactMatch || file.size() != subRepo.size() ) )
			return subRepo;
	}
	return FileSystem::fileNameFromPath( !projectDir.empty() ? projectDir : mProjectPath );
}

std::string Git::repoPath( const std::string& file ) {
	for ( const auto& subRepo : mSubModules ) {
		if ( String::startsWith( file, subRepo ) && file.size() != subRepo.size() )
			return mProjectPath + subRepo;
	}
	return mProjectPath;
}

Git::Result Git::gitSimple( const std::string& cmd, const std::string& projectDir ) {
	std::string buf;
	int retCode = git( cmd, projectDir, buf );
	Git::Result res;
	res.returnCode = retCode;
	res.result = buf;
	return res;
}

Git::Status Git::status( bool recurseSubmodules, const std::string& projectDir ) {
	static constexpr auto DIFF_CMD = "diff --numstat";
	static constexpr auto DIFF_STAGED_CMD = "diff --numstat --staged";
	static constexpr auto STATUS_CMD = "-c color.status=never status -b -u -s";
	Status s;
	std::string buf;

	getSubModules( projectDir );
	bool submodules = hasSubmodules( projectDir );

	LuaPattern subModulePattern( "^Entering '(.*)'" );

	bool modifiedSubmodule = false;
	auto parseStatus = [&s, &buf, &modifiedSubmodule, &projectDir, this, &subModulePattern]() {
		std::string subModulePath = "";
		LuaPattern pattern( "^([mMARTUD?%s][mMARTUD?%s])%s(.*)" );
		size_t changesCount = countLines( buf );

		if ( changesCount > 1000 )
			return;

		readAllLines( buf, [&]( const std::string_view& line ) {
			LuaPattern::Range matches[3];
			if ( subModulePattern.matches( line.data(), 0, matches, line.size() ) ) {
				subModulePath = String::trim(
					line.substr( matches[1].start, matches[1].end - matches[1].start ) );
				FileSystem::dirAddSlashAtEnd( subModulePath );
			} else if ( pattern.matches( line.data(), 0, matches, line.size() ) ) {
				auto statusStr = line.substr( matches[1].start, matches[1].end - matches[1].start );
				auto file = line.substr( matches[2].start, matches[2].end - matches[2].start );
				if ( statusStr.size() < 2 )
					return;

				auto status = statusFromShortStatusStr( statusStr );

				if ( status.status == GitStatus::NotSet )
					return;

				if ( status.symbol == GitStatusChar::ModifiedSubmodule ) {
					modifiedSubmodule = true;
					return;
				}

				bool isStagedAndModified =
					status.type == GitStatusType::Staged && statusStr[1] != ' ';

				auto filePath = subModulePath + file;
				auto repo = repoName( filePath, false, projectDir );
				auto repoIt = s.files.find( repo );
				bool found = false;
				if ( repoIt != s.files.end() ) {
					for ( auto& fileIt : repoIt->second ) {
						if ( fileIt.file == filePath ) {
							fileIt.report = status;
							found = true;
							break;
						}
					}
				}
				if ( !found ) {
					s.files[repo].push_back( { std::move( filePath ), 0, 0, status } );

					if ( isStagedAndModified ) {
						status.type = GitStatusType::Changed;

						s.files[repo].push_back( { std::move( filePath ), 0, 0, status } );
					}
				}
			}
		} );
	};

	if ( EXIT_SUCCESS != git( STATUS_CMD, projectDir, buf ) )
		return s;

	parseStatus();

	if ( modifiedSubmodule && recurseSubmodules && submodules ) {
		gitSubmodules( STATUS_CMD, projectDir, buf );
		parseStatus();
	}

	auto parseNumStat = [&s, &buf, &projectDir, this, &subModulePattern]( bool isStaged ) {
		LuaPattern pattern( "(%d+)%s+(%d+)%s+(.+)" );
		std::string subModulePath = "";
		readAllLines( buf, [&]( const std::string_view& line ) {
			LuaPattern::Range matches[4];
			if ( subModulePattern.matches( line.data(), 0, matches, line.size() ) ) {
				subModulePath = String::trim(
					line.substr( matches[1].start, matches[1].end - matches[1].start ) );
				FileSystem::dirAddSlashAtEnd( subModulePath );
			} else if ( pattern.matches( line.data(), 0, matches, line.size() ) ) {
				auto inserted = line.substr( matches[1].start, matches[1].end - matches[1].start );
				auto deleted = line.substr( matches[2].start, matches[2].end - matches[2].start );
				auto file = line.substr( matches[3].start, matches[3].end - matches[3].start );
				int inserts;
				int deletes;
				if ( String::fromString( inserts, inserted ) &&
					 String::fromString( deletes, deleted ) && ( inserts || deletes ) ) {
					auto filePath = subModulePath + file;
					auto repo = repoName( filePath, false, projectDir );
					auto repoIt = s.files.find( repo );
					GitStatusReport status = { GitStatus::NotSet, GitStatusType::Untracked,
											   GitStatusChar::Untracked };
					bool found = false;
					if ( repoIt != s.files.end() ) {
						for ( auto& fileIt : repoIt->second ) {
							if ( fileIt.file == filePath ) {
								if ( isStaged && fileIt.report.type != Git::GitStatusType::Staged )
									continue;
								if ( !isStaged && fileIt.report.type == Git::GitStatusType::Staged )
									continue;
								fileIt.inserts = inserts;
								fileIt.deletes = deletes;
								found = true;
								break;
							}
						}
					}
					if ( !found ) {
						s.files[repo].push_back(
							{ std::move( filePath ), inserts, deletes, status } );
					}
					s.totalInserts += inserts;
					s.totalDeletions += deletes;
				}
			}
		} );
	};

	if ( EXIT_SUCCESS != git( DIFF_CMD, projectDir, buf ) )
		return s;

	parseNumStat( false );

	git( DIFF_STAGED_CMD, projectDir, buf );
	parseNumStat( true );

	if ( recurseSubmodules && submodules ) {
		gitSubmodules( DIFF_CMD, projectDir, buf );
		parseNumStat( false );

		gitSubmodules( DIFF_STAGED_CMD, projectDir, buf );
		parseNumStat( true );
	}

	for ( auto& [_, repo] : s.files ) {
		for ( auto& val : repo ) {
			if ( val.report.symbol == GitStatusChar::Added && val.inserts == 0 ) {
				std::string fileText;
				FileSystem::fileGet( ( projectDir.empty() ? mProjectPath : projectDir ) + val.file,
									 fileText );
				val.inserts = countLines( fileText );
				s.totalInserts += val.inserts;
			}
		}
	}

	return s;
}

Git::Blame Git::blame( const std::string& filepath, std::size_t line ) const {
	std::string buf;
	const auto getText = [&buf]( const std::string_view& txt ) -> std::string {
		std::string search = "\n" + txt + " ";
		auto pos = buf.find( search );
		if ( pos != std::string::npos ) {
			pos = pos + search.length();
			auto endPos = buf.find_first_of( '\n', pos );
			if ( endPos != std::string::npos )
				return buf.substr( pos, endPos - pos );
		}
		return "";
	};

	std::string workingDir( FileSystem::fileRemoveFileName( filepath ) );
	if ( EXIT_SUCCESS !=
		 git( String::format( "blame %s -p -L%zu,%zu", filepath.data(), line, line ), workingDir,
			  buf ) )
		return { buf };

	if ( String::startsWith( buf, "fatal: " ) )
		return { buf.substr( 7 ) };

	auto hashEnd = buf.find_first_of( ' ' );

	if ( hashEnd == std::string::npos )
		return { "No commit hash found" };

	auto commitHash = buf.substr( 0, hashEnd );

	if ( commitHash == sNotCommitedYetHash )
		return { "Not Committed Yet" };

	auto author = getText( "author"sv );
	auto authorEmail = getText( "author-mail"sv );
	if ( authorEmail.size() > 3 )
		authorEmail = authorEmail.substr( 1, authorEmail.size() - 2 );
	auto datetime = getText( "author-time"sv );
	auto tz = getText( "author-tz"sv );
	Uint64 epoch;
	if ( !datetime.empty() && String::fromString( epoch, datetime ) )
		datetime = Sys::epochToString( epoch ) + ( tz.empty() ? "" : " " + tz );

	auto commitMessage = getText( "summary"sv );

	git( String::format( "rev-parse --short %s", commitHash ), workingDir, buf );

	auto commitShortHash = String::rTrim( buf, '\n' );

	return { std::move( author ),
			 std::move( authorEmail ),
			 std::move( datetime ),
			 std::move( commitHash ),
			 std::move( commitShortHash ),
			 std::move( commitMessage ),
			 line };
}

Git::GitStatusReport Git::statusFromShortStatusStr( const std::string_view& statusStr ) {
	Uint16 status = git_xy( statusStr[0], statusStr[1] );
	GitStatus gitStatus = GitStatus::NotSet;
	GitStatusChar gitStatusChar = GitStatusChar::Unknown;
	GitStatusType gitStatusType = GitStatusType::Untracked;

	switch ( status ) {
		case StatusXY::DD: {
			gitStatus = GitStatus::Unmerge_BothDeleted;
			gitStatusChar = GitStatusChar::Deleted;
			gitStatusType = GitStatusType::Unmerged;
			break;
		}
		case StatusXY::AU: {
			gitStatus = GitStatus::Unmerge_AddedByUs;
			gitStatusChar = GitStatusChar::Added;
			gitStatusType = GitStatusType::Unmerged;
			break;
		}
		case StatusXY::UD: {
			gitStatus = GitStatus::Unmerge_DeletedByThem;
			gitStatusChar = GitStatusChar::Deleted;
			gitStatusType = GitStatusType::Unmerged;
			break;
		}
		case StatusXY::UA: {
			gitStatus = GitStatus::Unmerge_AddedByThem;
			gitStatusChar = GitStatusChar::Added;
			gitStatusType = GitStatusType::Unmerged;
			break;
		}
		case StatusXY::DU: {
			gitStatus = GitStatus::Unmerge_DeletedByUs;
			gitStatusChar = GitStatusChar::Deleted;
			gitStatusType = GitStatusType::Unmerged;
			break;
		}
		case StatusXY::AA: {
			gitStatus = GitStatus::Unmerge_BothAdded;
			gitStatusChar = GitStatusChar::Added;
			gitStatusType = GitStatusType::Unmerged;
			break;
		}
		case StatusXY::UU: {
			gitStatus = GitStatus::Unmerge_BothModified;
			gitStatusChar = GitStatusChar::Modified;
			gitStatusType = GitStatusType::Unmerged;
			break;
		}
		case StatusXY::QQ: {
			gitStatus = GitStatus::Untracked;
			gitStatusChar = GitStatusChar::Untracked;
			gitStatusType = GitStatusType::Untracked;
			break;
		}
		case StatusXY::II: {
			gitStatus = GitStatus::Ignored;
			gitStatusChar = GitStatusChar::Ignored;
			gitStatusType = GitStatusType::Ignored;
			break;
		}
		case StatusXY::m: {
			gitStatus = GitStatus::WorkingTree_ModifiedSubmodule;
			gitStatusChar = GitStatusChar::ModifiedSubmodule;
			gitStatusType = GitStatusType::Changed;
			break;
		}
		default:
			break;
	}

	if ( gitStatus == GitStatus::NotSet ) {
		char x = statusStr[0];

		switch ( x ) {
			case 'M': {
				gitStatus = GitStatus::Index_Modified;
				gitStatusChar = GitStatusChar::Modified;
				gitStatusType = GitStatusType::Staged;
				break;
			}
			case 'A': {
				gitStatus = GitStatus::Index_Added;
				gitStatusChar = GitStatusChar::Added;
				gitStatusType = GitStatusType::Staged;
				break;
			}
			case 'D': {
				gitStatus = GitStatus::Index_Deleted;
				gitStatusChar = GitStatusChar::Deleted;
				gitStatusType = GitStatusType::Staged;
				break;
			}
			case 'R': {
				gitStatus = GitStatus::Index_Renamed;
				gitStatusChar = GitStatusChar::Renamed;
				gitStatusType = GitStatusType::Staged;
				break;
			}
			case 'C': {
				gitStatus = GitStatus::Index_Copied;
				gitStatusChar = GitStatusChar::Copied;
				gitStatusType = GitStatusType::Staged;
				break;
			}
			case 'm': {
				gitStatus = GitStatus::Index_ModifiedSubmodule;
				gitStatusChar = GitStatusChar::ModifiedSubmodule;
				gitStatusType = GitStatusType::Staged;
				break;
			}
		}
	}

	if ( gitStatus == GitStatus::NotSet ) {
		char y = statusStr[1];
		switch ( y ) {
			case 'M': {
				gitStatus = GitStatus::WorkingTree_Modified;
				gitStatusChar = GitStatusChar::Modified;
				gitStatusType = GitStatusType::Changed;
				break;
			}
			case 'D': {
				gitStatus = GitStatus::WorkingTree_Deleted;
				gitStatusChar = GitStatusChar::Deleted;
				gitStatusType = GitStatusType::Changed;
				break;
			}
			case 'A': {
				gitStatus = GitStatus::WorkingTree_IntentToAdd;
				gitStatusChar = GitStatusChar::Added;
				gitStatusType = GitStatusType::Changed;
				break;
			}
		}
	}

	return { gitStatus, gitStatusType, gitStatusChar };
}

} // namespace ecode
