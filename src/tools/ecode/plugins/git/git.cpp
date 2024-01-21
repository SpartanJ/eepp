#include "git.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>

using namespace EE;
using namespace EE::System;

using namespace std::literals;

namespace ecode {

static int countLines( const std::string& text ) {
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
	buf.clear();
	Process p;
	p.create( mGitPath, args, Process::CombinedStdoutStderr | Process::Options::NoWindow,
			  { { "LC_ALL", "en_US.UTF-8" } }, projectDir.empty() ? mProjectPath : projectDir );
	p.readAllStdOut( buf );
	int retCode;
	p.join( &retCode );
	if ( mLastProjectPath != projectDir ) {
		const_cast<Git*>( this )->mLastProjectPath = projectDir;
		const_cast<Git*>( this )->mSubModulesUpdated = false;
	}
	Log::debug( "GitPlugin run: %s %s", mGitPath, args );
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

bool Git::setProjectPath( const std::string& projectPath ) {
	mProjectPath = "";
	mGitFolder = "";
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
	std::string buf;
	Git::Result res;
	int retCode = git( "pull", projectDir, buf );
	res.returnCode = retCode;
	res.result = buf;
	return res;
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

Git::Result Git::add( const std::string& file, const std::string& projectDir ) {
	std::string buf;
	int retCode = git( String::format( "add --force -- \"%s\"", file ), projectDir, buf );
	Git::Result res;
	res.returnCode = retCode;
	res.result = buf;
	return res;
}

Git::Result Git::restore( const std::string& file, const std::string& projectDir ) {
	std::string buf;
	int retCode = git( String::format( "restore \"%s\"", file ), projectDir, buf );
	Git::Result res;
	res.returnCode = retCode;
	res.result = buf;
	return res;
}

Git::Result Git::reset( const std::string& file, const std::string& projectDir ) {
	std::string buf;
	int retCode = git( String::format( "reset -q HEAD -- \"%s\"", file ), projectDir, buf );
	Git::Result res;
	res.returnCode = retCode;
	res.result = buf;
	return res;
}

Git::Result Git::renameBranch( const std::string& branch, const std::string& newName,
							   const std::string& projectDir ) {
	std::string buf;
	int retCode = git( String::format( "branch -M %s %s", branch, newName ), projectDir, buf );
	Git::Result res;
	res.returnCode = retCode;
	res.result = buf;
	return res;
}

Git::Result Git::deleteBranch( const std::string& branch, const std::string& projectDir ) {
	std::string buf;
	int retCode = git( String::format( "branch -D %s", branch ), projectDir, buf );
	Git::Result res;
	res.returnCode = retCode;
	res.result = buf;
	return res;
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

Git::Branch Git::parseLocalBranch( const std::string_view& raw, const std::string& projectDir ) {
	static constexpr size_t len = std::string_view{ "refs/heads/"sv }.size();
	if ( len >= raw.size() )
		return {};

	auto split = String::split( raw, '\t' );
	if ( split.size() != 2 )
		return {};
	std::string name( std::string{ split[0].substr( len ) } );
	std::string remote( std::string{ split[1] } );
	int64_t ahead = 0;
	int64_t behind = 0;

	auto res = branchHistoryPosition( name, remote );
	if ( res.success() ) {
		ahead = res.ahead;
		behind = res.behind;
	}

	return Git::Branch{
		std::move( name ), std::move( remote ), Git::RefType::Head, std::string{}, ahead, behind };
}

static Git::Branch parseRemoteBranch( std::string_view raw ) {
	static constexpr size_t len = std::string_view( "refs/remotes/"sv ).size();
	size_t indexOfRemote = raw.find_first_of( '/', len );
	if ( indexOfRemote != std::string::npos && len < raw.size() ) {
		return Git::Branch{ std::string{ raw.substr( len ) },
							std::string{ raw.substr( len, indexOfRemote - len ) },
							Git::RefType::Remote, std::string{} };
	}
	return {};
}

std::vector<Git::Branch> Git::getAllBranchesAndTags( RefType ref, const std::string& projectDir ) {
	std::string args( "for-each-ref --format '%(refname)	%(upstream:short)' --sort=v:refname" );
	if ( ref & RefType::Head )
		args.append( " refs/heads" );
	if ( ref & RefType::Remote )
		args.append( " refs/remotes" );
	if ( ref & RefType::Tag )
		args.append( " refs/tags" );

	std::vector<Branch> branches;
	std::string buf;

	if ( EXIT_SUCCESS != git( args, projectDir, buf ) )
		return branches;

	branches.reserve( countLines( buf ) );

	readAllLines( buf, [&]( const std::string_view& line ) {
		auto branch = String::trim( String::trim( line, '\'' ), '\t' );
		if ( ( ref & Head ) && String::startsWith( branch, "refs/heads/" ) ) {
			branches.emplace_back( parseLocalBranch( branch, projectDir ) );
		} else if ( ( ref & Remote ) && String::startsWith( branch, "refs/remotes/" ) ) {
			branches.emplace_back( parseRemoteBranch( branch ) );
		} else if ( ( ref & Tag ) && String::startsWith( branch, "refs/tags/" ) ) {
			static constexpr size_t len = std::string_view{ "refs/tags/"sv }.size();
			Branch newBranch;
			newBranch.name = branch.substr( len );
			newBranch.type = RefType::Tag;
			branches.emplace_back( std::move( newBranch ) );
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

std::string Git::inSubModule( const std::string& file, const std::string& projectDir ) {
	for ( const auto& subRepo : mSubModules ) {
		if ( String::startsWith( file, subRepo ) && file.size() != subRepo.size() )
			return subRepo;
	}
	return FileSystem::fileNameFromPath( !projectDir.empty() ? projectDir : mProjectPath );
}

Git::Status Git::status( bool recurseSubmodules, const std::string& projectDir ) {
	static constexpr auto DIFF_CMD = "diff --numstat";
	static constexpr auto DIFF_STAGED_CMD = "diff --numstat --staged";
	static constexpr auto STATUS_CMD = "-c color.status=never status -b -u -s";
	Status s;
	std::string buf;
	if ( EXIT_SUCCESS != git( DIFF_CMD, projectDir, buf ) )
		return s;

	getSubModules( projectDir );

	LuaPattern subModulePattern( "^Entering '(.*)'" );

	auto parseNumStat = [&s, &buf, &projectDir, this, &subModulePattern]() {
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
					auto repo = inSubModule( filePath, projectDir );
					auto repoIt = s.files.find( repo );
					if ( repoIt != s.files.end() ) {
						bool found = false;
						for ( auto& fileIt : repoIt->second ) {
							if ( fileIt.file == filePath ) {
								fileIt.inserts = inserts;
								fileIt.deletes = deletes;
								found = true;
								break;
							}
						}
						if ( !found ) {
							s.files[repo].push_back( { std::move( filePath ), inserts, deletes,
													   GitStatus::NotSet, GitStatusType::Untracked,
													   GitStatusChar::Untracked } );
						}
					} else {
						s.files.insert(
							{ repo,
							  { { std::move( filePath ), inserts, deletes, GitStatus::NotSet,
								  GitStatusType::Untracked, GitStatusChar::Untracked } } } );
					}
					s.totalInserts += inserts;
					s.totalDeletions += deletes;
				}
			}
		} );
	};

	parseNumStat();

	git( DIFF_STAGED_CMD, projectDir, buf );
	parseNumStat();

	bool submodules = hasSubmodules( projectDir );

	if ( recurseSubmodules && submodules ) {
		gitSubmodules( DIFF_CMD, projectDir, buf );
		parseNumStat();

		gitSubmodules( DIFF_STAGED_CMD, projectDir, buf );
		parseNumStat();
	}

	bool modifiedSubmodule = false;
	auto parseStatus = [&s, &buf, &modifiedSubmodule, &projectDir, this, &subModulePattern]() {
		std::string subModulePath = "";
		LuaPattern pattern( "^([mMARTUD?%s][mMARTUD?%s])%s(.*)" );
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

				if ( gitStatus != GitStatus::NotSet ) {
					if ( gitStatusChar == GitStatusChar::ModifiedSubmodule )
						modifiedSubmodule = true;
					else {
						auto filePath = subModulePath + file;
						auto repo = inSubModule( filePath, projectDir );
						auto repoIt = s.files.find( repo );
						if ( repoIt != s.files.end() ) {
							bool found = false;
							for ( auto& fileIt : repoIt->second ) {
								if ( fileIt.file == filePath ) {
									fileIt.statusChar = gitStatusChar;
									fileIt.status = gitStatus;
									fileIt.statusType = gitStatusType;
									found = true;
									break;
								}
							}
							if ( !found ) {
								s.files[repo].push_back( { std::move( filePath ), 0, 0, gitStatus,
														   gitStatusType, gitStatusChar } );
							}
						} else {
							s.files.insert( { repo,
											  { { std::move( filePath ), 0, 0, gitStatus,
												  gitStatusType, gitStatusChar } } } );
						}
					}
				}
			}
		} );
	};

	git( STATUS_CMD, projectDir, buf );
	parseStatus();

	if ( modifiedSubmodule && recurseSubmodules && submodules ) {
		gitSubmodules( STATUS_CMD, projectDir, buf );
		parseStatus();
	}

	for ( auto& [_, repo] : s.files ) {
		for ( auto& val : repo ) {
			if ( val.statusChar == GitStatusChar::Added && val.inserts == 0 ) {
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

} // namespace ecode
