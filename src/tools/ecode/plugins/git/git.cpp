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

Git::CheckoutResult Git::checkout( const std::string& branch,
								   const std::string& projectDir ) const {
	std::string buf;
	int retCode = git( String::format( "checkout %s", branch ), projectDir, buf );
	Git::CheckoutResult res;
	if ( EXIT_SUCCESS != retCode ) {
		res.returnCode = retCode;
		res.error = buf;
	} else {
		res.branch = branch;
	}
	return res;
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
	if ( EXIT_SUCCESS != retCode ) {
		res.returnCode = retCode;
		res.error = buf;
	} else {
		res.branch = buf;
	}
	return res;
}

std::vector<Git::Branch> Git::getAllBranches( const std::string& projectDir ) {
	return getAllBranchesAndTags( static_cast<RefType>( RefType::Head | RefType::Remote ),
								  projectDir );
}

static Git::Branch parseLocalBranch( const std::string_view& raw ) {
	static constexpr size_t len = std::string_view{ "refs/heads/"sv }.size();
	return Git::Branch{ std::string{ raw.substr( len ) }, std::string{}, Git::RefType::Head,
						std::string{} };
}

static Git::Branch parseRemoteBranch( const std::string_view& raw ) {
	static constexpr size_t len = std::string_view( "refs/remotes/"sv ).size();
	size_t indexOfRemote = raw.find_first_of( '/', len );
	if ( indexOfRemote != std::string::npos )
		return Git::Branch{ std::string{ raw.substr( len ) },
							std::string{ raw.substr( len, indexOfRemote - len ) },
							Git::RefType::Remote, std::string{} };
	return {};
}

std::vector<Git::Branch> Git::getAllBranchesAndTags( RefType ref, const std::string& projectDir ) {
	std::string args( "for-each-ref --format '%(refname)' --sort=-committerdate" );
	if ( ref & RefType::Head ) {
		args.append( " refs/heads" );
	}
	if ( ref & RefType::Remote ) {
		args.append( " refs/remotes" );
	}
	if ( ref & RefType::Tag ) {
		args.append( " refs/tags" );
		args.append( " --sort=-taggerdate" );
	}

	std::vector<Branch> branches;
	std::string buf;

	if ( EXIT_SUCCESS != git( args, projectDir, buf ) )
		return branches;

	readAllLines( buf, [&branches, ref]( const std::string_view& line ) {
		auto branch = String::trim( line, '\n' );
		branch = String::trim( line, '\'' );
		if ( ( ref & Head ) && String::startsWith( branch, "refs/heads/" ) ) {
			branches.emplace_back( parseLocalBranch( branch ) );
		} else if ( ( ref & Remote ) && String::startsWith( branch, "refs/remotes/" ) ) {
			branches.emplace_back( parseRemoteBranch( branch ) );
		} else if ( ( ref & Tag ) && String::startsWith( branch, "refs/tags/" ) ) {
			static constexpr size_t len = std::string_view{ "refs/tags/"sv }.size();
			branches.push_back( { std::string{ branch.substr( len ) }, std::string{}, RefType::Tag,
								  std::string{} } );
		}
	} );

	std::sort( branches.begin(), branches.end(), []( const Branch& left, const Branch& right ) {
		return left.type < right.type || left.name < right.name;
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
		if ( String::startsWith( file, subRepo ) )
			return subRepo;
	}
	return FileSystem::fileNameFromPath( !projectDir.empty() ? projectDir : mProjectPath );
}

Git::Status Git::status( bool recurseSubmodules, const std::string& projectDir ) {
	static constexpr auto DIFF_CMD = "diff --numstat";
	static constexpr auto STATUS_CMD = "-c color.status=never status -b -u -s";
	Status s;
	std::string buf;
	if ( EXIT_SUCCESS != git( DIFF_CMD, projectDir, buf ) )
		return s;
	auto parseNumStat = [&s, &buf, &projectDir, this]() {
		auto lastNL = 0;
		auto nextNL = buf.find_first_of( '\n' );
		LuaPattern pattern( "(%d+)%s+(%d+)%s+(.+)" );
		while ( nextNL != std::string_view::npos ) {
			LuaPattern::Range matches[4];
			if ( pattern.matches( buf.c_str(), lastNL, matches, nextNL ) ) {
				auto inserted = buf.substr( matches[1].start, matches[1].end - matches[1].start );
				auto deleted = buf.substr( matches[2].start, matches[2].end - matches[2].start );
				auto file = buf.substr( matches[3].start, matches[3].end - matches[3].start );
				int inserts;
				int deletes;
				if ( String::fromString( inserts, inserted ) &&
					 String::fromString( deletes, deleted ) ) {
					auto repo = inSubModule( file, projectDir );
					auto repoIt = s.files.find( repo );
					if ( repoIt != s.files.end() ) {
						bool found = false;
						for ( auto& fileIt : repoIt->second ) {
							if ( fileIt.file == file ) {
								fileIt.inserts = inserts;
								fileIt.deletes = deletes;
								found = true;
								break;
							}
						}
						if ( !found )
							s.files[repo].push_back( { std::move( file ), inserts, deletes } );
					} else {
						s.files.insert( { repo, { { std::move( file ), inserts, deletes } } } );
					}
					s.totalInserts += inserts;
					s.totalDeletions += deletes;
				}
			}
			lastNL = nextNL;
			nextNL = buf.find_first_of( '\n', nextNL + 1 );
		}
	};

	parseNumStat();

	bool submodules = hasSubmodules( projectDir );

	if ( recurseSubmodules && submodules ) {
		gitSubmodules( DIFF_CMD, projectDir, buf );
		parseNumStat();
	}

	getSubModules( projectDir );

	bool modifiedSubmodule = false;
	auto parseStatus = [&s, &buf, &modifiedSubmodule, &projectDir, this]() {
		auto lastNL = 0;
		auto nextNL = buf.find_first_of( '\n' );
		LuaPattern pattern( "\n([%sA?][MARTUD?%s])%s(.*)" );
		while ( nextNL != std::string_view::npos ) {
			LuaPattern::Range matches[3];
			if ( pattern.matches( buf.c_str(), lastNL, matches, nextNL ) ) {
				auto status = buf.substr( matches[1].start, matches[1].end - matches[1].start );
				String::trimInPlace( status );
				auto file = buf.substr( matches[2].start, matches[2].end - matches[2].start );
				FileStatus rstatus = FileStatus::Unknown;
				if ( "??" == status )
					rstatus = FileStatus::Untracked;
				else if ( "M" == status )
					rstatus = FileStatus::Modified;
				else if ( "A" == status )
					rstatus = FileStatus::Added;
				else if ( "D" == status )
					rstatus = FileStatus::Deleted;
				else if ( "R" == status )
					rstatus = FileStatus::Renamed;
				else if ( "T" == status )
					rstatus = FileStatus::TypeChanged;
				else if ( "U" == status )
					rstatus = FileStatus::UpdatedUnmerged;
				else if ( "m" == status )
					rstatus = FileStatus::ModifiedSubmodule;

				if ( rstatus != FileStatus::Unknown ) {
					if ( rstatus == FileStatus::ModifiedSubmodule )
						modifiedSubmodule = true;
					else {
						auto repo = inSubModule( file, projectDir );
						auto repoIt = s.files.find( repo );
						if ( repoIt != s.files.end() ) {
							bool found = false;
							for ( auto& fileIt : repoIt->second ) {
								if ( fileIt.file == file ) {
									fileIt.status = rstatus;
									found = true;
									break;
								}
							}
							if ( !found )
								s.files[repo].push_back( { std::move( file ), 0, 0, rstatus } );
						} else {
							s.files.insert( { file, { { file, 0, 0, rstatus } } } );
						}
					}
				}
			}
			lastNL = nextNL;
			nextNL = buf.find_first_of( '\n', nextNL + 1 );
		}
	};

	git( STATUS_CMD, projectDir, buf );
	parseStatus();

	if ( modifiedSubmodule && submodules ) {
		gitSubmodules( STATUS_CMD, projectDir, buf );
		parseStatus();
	}

	for ( auto& [_, repo] : s.files ) {
		for ( auto& val : repo ) {
			if ( val.status == FileStatus::Added && val.inserts == 0 ) {
				std::string fileText;
				FileSystem::fileGet( ( projectDir.empty() ? mProjectPath : projectDir ) + val.file,
									 fileText );
				val.inserts = countLines( fileText );
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
