#include "git.hpp"
#include <eepp/core/containers.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>

#include <algorithm>
#include <charconv>
#include <cstdio>

using namespace EE;
using namespace EE::System;

using namespace std::literals;

namespace ecode {

static bool parseHistoryTimestamp( std::string_view value, int64_t& timestamp ) {
	const char* end = value.data() + value.size();
	auto result = std::from_chars( value.data(), end, timestamp );
	return result.ec == std::errc{} && result.ptr == end;
}

Git::HistoryPage Git::history( const HistoryQuery& query, const std::string& projectDir ) const {
	HistoryPage page;
	if ( query.limit == 0 || query.limit > 1000 ||
		 ( query.revision.empty() && query.continuation.empty() ) ) {
		page.returnCode = EXIT_FAILURE;
		page.result = "Invalid Git history query";
		return page;
	}
	std::vector<std::string> args{ "log",
								   "--first-parent",
								   String::format( "--max-count=%zu", query.limit + 1 ),
								   "-z",
								   "--format=%H%x00%h%x00%P%x00%an%x00%ae%x00%at%x00%ct%x00%B",
								   query.continuation.empty() ? query.revision
															  : query.continuation };
	if ( !query.exclusions.empty() ) {
		args.emplace_back( "--not" );
		args.insert( args.end(), query.exclusions.begin(), query.exclusions.end() );
	}
	page.returnCode = git( args, projectDir, page.result );
	if ( page.fail() ) {
		std::string head;
		if ( query.revision == "HEAD" && query.continuation.empty() &&
			 git( { "rev-parse", "--verify", "HEAD" }, projectDir, head ) != EXIT_SUCCESS ) {
			page.returnCode = EXIT_SUCCESS;
			page.result.clear();
		}
		return page;
	}
	constexpr size_t FieldCount = 8;
	size_t offset = 0;
	while ( offset < page.result.size() ) {
		std::string_view fields[FieldCount];
		for ( size_t field = 0; field < FieldCount; ++field ) {
			const size_t end = page.result.find( '\0', offset );
			if ( end == std::string::npos ) {
				page.returnCode = EXIT_FAILURE;
				page.commits.clear();
				page.result = "Invalid NUL-framed git log output";
				return page;
			}
			fields[field] = std::string_view( page.result ).substr( offset, end - offset );
			offset = end + 1;
		}
		if ( offset < page.result.size() && page.result[offset] == '\0' )
			++offset;
		Commit commit;
		commit.hash = fields[0];
		commit.shortHash = fields[1];
		size_t parentOffset = 0;
		while ( parentOffset < fields[2].size() ) {
			const size_t separator = fields[2].find( ' ', parentOffset );
			commit.parents.emplace_back( fields[2].substr(
				parentOffset, separator == std::string_view::npos ? fields[2].size() - parentOffset
																  : separator - parentOffset ) );
			if ( separator == std::string_view::npos )
				break;
			parentOffset = separator + 1;
		}
		commit.authorName = fields[3];
		commit.authorEmail = fields[4];
		commit.message = fields[7];
		while ( !commit.message.empty() &&
				( commit.message.back() == '\n' || commit.message.back() == '\r' ) )
			commit.message.pop_back();
		const size_t subjectEnd = commit.message.find_first_of( "\r\n" );
		commit.subject = commit.message.substr( 0, subjectEnd );
		if ( !parseHistoryTimestamp( fields[5], commit.authorTime ) ||
			 !parseHistoryTimestamp( fields[6], commit.commitTime ) ) {
			page.returnCode = EXIT_FAILURE;
			page.commits.clear();
			page.result = "Invalid timestamp in git log output";
			return page;
		}
		page.commits.emplace_back( std::move( commit ) );
	}
	page.result.clear();
	page.hasMore = page.commits.size() > query.limit;
	if ( page.hasMore )
		page.commits.resize( query.limit );
	return page;
}

Git::CommitFiles Git::commitFiles( const Commit& commit, const std::string& projectDir ) const {
	CommitFiles result;
	if ( commit.hash.empty() ) {
		result.returnCode = EXIT_FAILURE;
		result.result = "Invalid commit";
		return result;
	}
	std::vector<std::string> args;
	if ( commit.parents.empty() ) {
		args = { "diff-tree", "--root", "--no-commit-id", "--name-status", "-r", "-z", "-M",
				 commit.hash, "--" };
	} else {
		args = { "diff", "--name-status", "-z", "-M", commit.parents[0], commit.hash, "--" };
	}
	result.returnCode = git( args, projectDir, result.result );
	if ( result.fail() )
		return result;

	size_t offset = 0;
	auto nextField = [&result, &offset]( std::string_view& field ) {
		if ( offset >= result.result.size() )
			return false;
		const size_t end = result.result.find( '\0', offset );
		if ( end == std::string::npos )
			return false;
		field = std::string_view( result.result ).substr( offset, end - offset );
		offset = end + 1;
		return true;
	};
	while ( offset < result.result.size() ) {
		std::string_view status;
		std::string_view path;
		if ( !nextField( status ) || !nextField( path ) || status.empty() ) {
			result.returnCode = EXIT_FAILURE;
			result.files.clear();
			result.result = "Invalid NUL-framed git diff output";
			return result;
		}
		CommitFile file;
		file.status = status;
		if ( status[0] == 'R' || status[0] == 'C' ) {
			std::string_view newPath;
			if ( !nextField( newPath ) ) {
				result.returnCode = EXIT_FAILURE;
				result.files.clear();
				result.result = "Invalid renamed path in git diff output";
				return result;
			}
			file.oldPath = path;
			file.path = newPath;
		} else {
			file.path = path;
		}
		result.files.emplace_back( std::move( file ) );
	}
	std::string numstat;
	if ( commit.parents.empty() ) {
		args = { "diff-tree", "--root", "--no-commit-id", "--numstat", "-r",
				 "-z",		  "-M",		commit.hash,	  "--" };
	} else {
		args = { "diff", "--numstat", "-z", "-M", commit.parents[0], commit.hash, "--" };
	}
	result.returnCode = git( args, projectDir, numstat );
	if ( result.fail() ) {
		result.result = std::move( numstat );
		result.files.clear();
		return result;
	}
	offset = 0;
	while ( offset < numstat.size() ) {
		const size_t end = numstat.find( '\0', offset );
		if ( end == std::string::npos ) {
			result.returnCode = EXIT_FAILURE;
			result.result = "Invalid NUL-framed git numstat output";
			result.files.clear();
			return result;
		}
		const std::string_view record( numstat.data() + offset, end - offset );
		offset = end + 1;
		const size_t firstTab = record.find( '\t' );
		const size_t secondTab =
			firstTab == std::string_view::npos ? firstTab : record.find( '\t', firstTab + 1 );
		if ( firstTab == std::string_view::npos || secondTab == std::string_view::npos ) {
			result.returnCode = EXIT_FAILURE;
			result.result = "Invalid git numstat record";
			result.files.clear();
			return result;
		}
		std::string_view path = record.substr( secondTab + 1 );
		std::string_view oldPath;
		if ( path.empty() ) {
			const size_t oldEnd = numstat.find( '\0', offset );
			if ( oldEnd == std::string::npos ) {
				result.returnCode = EXIT_FAILURE;
				result.result = "Invalid renamed path in git numstat output";
				result.files.clear();
				return result;
			}
			oldPath = std::string_view( numstat ).substr( offset, oldEnd - offset );
			offset = oldEnd + 1;
			const size_t newEnd = numstat.find( '\0', offset );
			if ( newEnd == std::string::npos ) {
				result.returnCode = EXIT_FAILURE;
				result.result = "Invalid renamed path in git numstat output";
				result.files.clear();
				return result;
			}
			path = std::string_view( numstat ).substr( offset, newEnd - offset );
			offset = newEnd + 1;
		}
		auto file = std::find_if( result.files.begin(), result.files.end(),
								  [path, oldPath]( const CommitFile& candidate ) {
									  return candidate.path == path &&
											 ( oldPath.empty() || candidate.oldPath == oldPath );
								  } );
		if ( file == result.files.end() )
			continue;
		const std::string_view inserted = record.substr( 0, firstTab );
		const std::string_view deleted = record.substr( firstTab + 1, secondTab - firstTab - 1 );
		file->isBinary = inserted == "-" || deleted == "-";
		auto parseCount = []( std::string_view value, int& count ) {
			const char* end = value.data() + value.size();
			auto parsed = std::from_chars( value.data(), end, count );
			return parsed.ec == std::errc{} && parsed.ptr == end;
		};
		if ( !file->isBinary &&
			 ( !parseCount( inserted, file->inserts ) || !parseCount( deleted, file->deletes ) ) ) {
			result.returnCode = EXIT_FAILURE;
			result.result = "Invalid line count in git numstat output";
			result.files.clear();
			return result;
		}
	}
	std::string message;
	result.returnCode = git( { "show", "-s", "--format=%B", commit.hash }, projectDir, message );
	if ( result.fail() ) {
		result.result = std::move( message );
		result.files.clear();
		return result;
	}
	while ( !message.empty() && ( message.back() == '\n' || message.back() == '\r' ) )
		message.pop_back();
	result.message = std::move( message );
	std::string patch;
	if ( commit.parents.empty() ) {
		args = { "show", "--format=", "--no-ext-diff", "--no-color", "-M", commit.hash };
	} else {
		args = { "diff", "--no-ext-diff", "--no-color", "-M", commit.parents[0], commit.hash };
	}
	result.returnCode = git( args, projectDir, patch );
	if ( result.fail() ) {
		result.result = std::move( patch );
		result.files.clear();
		return result;
	}
	result.patch = std::move( patch );
	std::string remote;
	if ( git( { "remote", "get-url", "origin" }, projectDir, remote ) == EXIT_SUCCESS ) {
		String::trimInPlace( remote, " \t\r\n" );
		const size_t host = remote.find( "github.com" );
		if ( host != std::string::npos ) {
			size_t pathStart = host + std::string_view( "github.com" ).size();
			while ( pathStart < remote.size() &&
					( remote[pathStart] == '/' || remote[pathStart] == ':' ) )
				++pathStart;
			std::string path = remote.substr( pathStart );
			while ( !path.empty() && path.back() == '/' )
				path.pop_back();
			if ( String::endsWith( path, ".git" ) )
				path.resize( path.size() - 4 );
			if ( !path.empty() )
				result.commitURL = "https://github.com/" + path + "/commit/" + commit.hash;
		}
	}
	result.result.clear();
	return result;
}

Git::CommitFiles Git::workingTreeFiles( const std::string& projectDir ) {
	CommitFiles result;
	Status current = status( false, projectDir );
	for ( const auto& [_, files] : current.files ) {
		for ( const auto& file : files ) {
			auto found = std::find_if(
				result.files.begin(), result.files.end(),
				[&file]( const CommitFile& item ) { return item.path == file.file; } );
			if ( found == result.files.end() ) {
				CommitFile item;
				item.path = file.file;
				item.status = std::string( 1, static_cast<char>( file.report.symbol ) );
				item.inserts = file.inserts;
				item.deletes = file.deletes;
				item.isBinary = file.isBinary;
				result.files.emplace_back( std::move( item ) );
			} else {
				found->inserts += file.inserts;
				found->deletes += file.deletes;
				found->isBinary |= file.isBinary;
			}
		}
	}

	result.returnCode = git( { "diff", "--no-ext-diff", "--no-color", "-M", "HEAD", "--" },
							 projectDir, result.patch );
	if ( result.fail() ) {
		result.patch.clear();
		result.returnCode =
			git( { "diff", "--no-ext-diff", "--no-color", "-M", "--" }, projectDir, result.patch );
	}
	if ( result.fail() ) {
		result.result = std::move( result.patch );
		return result;
	}

	for ( const auto& file : result.files ) {
		auto statusFile =
			std::find_if( current.files.begin(), current.files.end(), [&file]( const auto& repo ) {
				return std::any_of( repo.second.begin(), repo.second.end(),
									[&]( const DiffFile& item ) {
										return item.file == file.path &&
											   item.report.type == GitStatusType::Untracked;
									} );
			} );
		if ( statusFile == current.files.end() )
			continue;
		auto patch = diffUntracked( file.path, projectDir );
		if ( patch.success() ) {
			if ( !result.patch.empty() && result.patch.back() != '\n' )
				result.patch += '\n';
			result.patch += patch.result;
		}
	}
	result.result.clear();
	return result;
}

Git::Result Git::commitDiff( const Commit& commit, const CommitFile& file,
							 const std::string& projectDir ) const {
	Result result;
	if ( commit.hash.empty() || file.path.empty() ) {
		result.returnCode = EXIT_FAILURE;
		result.result = "Invalid commit diff query";
		return result;
	}
	std::vector<std::string> args;
	if ( commit.parents.empty() ) {
		args = { "show", "--format=", "--no-ext-diff", "--no-color", "-M", commit.hash, "--" };
	} else {
		args = { "diff", "--no-ext-diff", "--no-color", "-M", commit.parents[0], commit.hash,
				 "--" };
	}
	if ( !file.oldPath.empty() )
		args.emplace_back( file.oldPath );
	args.emplace_back( file.path );
	result.returnCode = git( args, projectDir, result.result );
	return result;
}

static constexpr auto sNotCommittedYetHash = "0000000000000000000000000000000000000000";
static constexpr std::string_view sAsciiWhitespace = " \t\r\n";

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
	return git( Process::parseArgs( args ), projectDir, buf );
}

int Git::git( const std::vector<std::string>& args, const std::string& projectDir,
			  std::string& buf ) const {
	return git( args, projectDir, buf, {} );
}

int Git::git( const std::vector<std::string>& args, const std::string& projectDir, std::string& buf,
			  std::string_view input ) const {
	Clock clock;
	buf.clear();
	Process p;
	if ( !p.create( mGitPath, args,
					Process::CombinedStdoutStderr | Process::Options::NoWindow |
						Process::Options::EnableAsync | Process::Options::InheritEnvironment,
					{ { "LC_ALL", "en_US.UTF-8" } },
					projectDir.empty() ? mProjectPath : projectDir ) ) {
		return EXIT_FAILURE;
	}
	int retCode = 0;
	if ( !input.empty() ) {
		size_t written = 0;
		while ( written < input.size() ) {
			const size_t count = p.write( input.substr( written ) );
			if ( count == 0 )
				break;
			written += count;
		}
		p.join( &retCode );
		p.readAllStdOut( buf );
	} else {
		p.readAllStdOut( buf );
		p.join( &retCode );
	}
	if ( !mSilent || retCode != EXIT_SUCCESS ) {
		const std::string joinedArgs = String::join( args );
		Log::instance()->writef( retCode != EXIT_SUCCESS ? LogLevel::Info : LogLevel::Debug,
								 "GitPlugin cmd in %s (%d): %s %s",
								 clock.getElapsedTime().toString(), retCode, mGitPath, joinedArgs );
	}
	return retCode;
}

Git::ConflictState Git::parseUnmergedIndex( const std::string& output ) {
	ConflictState state;
	UnorderedMap<std::string, size_t> fileIndices;
	size_t recordStart = 0;
	while ( recordStart < output.size() ) {
		const size_t recordEnd = output.find( '\0', recordStart );
		const size_t end = recordEnd == std::string::npos ? output.size() : recordEnd;
		const std::string_view record( output.data() + recordStart, end - recordStart );
		const size_t space = record.find( ' ' );
		const size_t secondSpace =
			space == std::string_view::npos ? space : record.find( ' ', space + 1 );
		const size_t tab = secondSpace == std::string_view::npos
							   ? secondSpace
							   : record.find( '\t', secondSpace + 1 );
		if ( space == std::string_view::npos || secondSpace == std::string_view::npos ||
			 tab == std::string_view::npos || tab <= secondSpace + 1 ) {
			state.error = "Invalid git ls-files --unmerged output";
			return state;
		}

		ConflictStage conflictStage;
		try {
			conflictStage.mode = static_cast<Uint32>(
				std::stoul( std::string( record.substr( 0, space ) ), nullptr, 8 ) );
			conflictStage.stage = static_cast<Uint8>( std::stoul(
				std::string( record.substr( secondSpace + 1, tab - secondSpace - 1 ) ) ) );
		} catch ( const std::exception& ) {
			state.error = "Invalid mode or stage in git ls-files --unmerged output";
			return state;
		}
		conflictStage.objectId = std::string( record.substr( space + 1, secondSpace - space - 1 ) );
		std::string path( record.substr( tab + 1 ) );
		auto [it, inserted] = fileIndices.emplace( path, state.files.size() );
		if ( inserted )
			state.files.emplace_back( ConflictFile{ std::move( path ) } );
		auto& file = state.files[it->second];
		switch ( conflictStage.stage ) {
			case 1:
				file.base = std::move( conflictStage );
				break;
			case 2:
				file.stage2 = std::move( conflictStage );
				break;
			case 3:
				file.stage3 = std::move( conflictStage );
				break;
			default:
				state.error = "Invalid index stage in git ls-files --unmerged output";
				return state;
		}
		recordStart = end + 1;
	}
	return state;
}

Git::GitOperation Git::operation( const std::string& projectDir ) const {
	const auto hasRef = [this, &projectDir]( const char* ref ) {
		std::string output;
		return git( { "rev-parse", "-q", "--verify", ref }, projectDir, output ) == EXIT_SUCCESS;
	};
	if ( hasRef( "MERGE_HEAD" ) )
		return GitOperation::Merge;
	if ( hasRef( "CHERRY_PICK_HEAD" ) )
		return GitOperation::CherryPick;
	if ( hasRef( "REVERT_HEAD" ) )
		return GitOperation::Revert;

	const auto hasGitPath = [this, &projectDir]( const char* name ) {
		std::string output;
		if ( git( { "rev-parse", "--git-path", name }, projectDir, output ) != EXIT_SUCCESS )
			return false;
		String::trimInPlace( output, sAsciiWhitespace );
		const bool absolute =
			!output.empty() && ( output.front() == '/' || output.front() == '\\' ||
								 ( output.size() > 1 && output[1] == ':' ) );
		if ( !absolute ) {
			const std::string& repo = projectDir.empty() ? mProjectPath : projectDir;
			output = repo + ( !repo.empty() && repo.back() == '/' ? "" : "/" ) + output;
		}
		return FileSystem::fileExists( output ) || FileSystem::isDirectory( output );
	};
	if ( hasGitPath( "rebase-merge" ) || hasGitPath( "rebase-apply" ) )
		return GitOperation::Rebase;
	if ( hasGitPath( "MERGE_AUTOSTASH" ) )
		return GitOperation::StashApply;
	return GitOperation::None;
}

Git::ConflictState Git::conflictState( const std::string& projectDir, bool loadContents ) const {
	std::string output;
	const int ret = git( { "ls-files", "--unmerged", "--stage", "-z" }, projectDir, output );
	ConflictState state = parseUnmergedIndex( output );
	state.operation = operation( projectDir );
	if ( ret != EXIT_SUCCESS ) {
		state.error = std::move( output );
		state.files.clear();
		return state;
	}
	const std::string& repo = projectDir.empty() ? mProjectPath : projectDir;
	for ( auto& file : state.files ) {
		file.workingTreeExists = FileSystem::fileExists(
			repo + ( !repo.empty() && repo.back() == '/' ? "" : "/" ) + file.path );
		if ( !loadContents )
			continue;
		for ( auto* stage : { &file.base, &file.stage2, &file.stage3 } ) {
			if ( !stage->has_value() )
				continue;
			std::string contents;
			if ( git( { "cat-file", "blob", ( *stage )->objectId }, projectDir, contents ) !=
				 EXIT_SUCCESS ) {
				state.error = std::move( contents );
				return state;
			}
			( *stage )->contents = std::move( contents );
			file.binary = file.binary || ( *stage )->contents.find( '\0' ) != std::string::npos;
		}
	}
	return state;
}

Git::Result Git::resolveConflict( const std::string& path, bool remove,
								  const std::string& projectDir ) const {
	Result result;
	result.returnCode = git( remove ? std::vector<std::string>{ "rm", "--", path }
									: std::vector<std::string>{ "add", "--", path },
							 projectDir, result.result );
	return result;
}

Git::Result Git::acceptConflictStage( const std::string& path, bool stage2, bool present,
									  const std::string& projectDir ) const {
	if ( !present )
		return resolveConflict( path, true, projectDir );
	Result result;
	result.returnCode = git( { "checkout", stage2 ? "--ours" : "--theirs", "--", path }, projectDir,
							 result.result );
	if ( result.success() )
		return resolveConflict( path, false, projectDir );
	return result;
}

Git::Result Git::restoreConflictStages( const ConflictFile& conflict,
										const std::string& projectDir ) const {
	Result result;
	const ConflictStage* firstStage = conflict.base		? &*conflict.base
									  : conflict.stage2 ? &*conflict.stage2
									  : conflict.stage3 ? &*conflict.stage3
														: nullptr;
	if ( !firstStage ) {
		result.returnCode = EXIT_FAILURE;
		return result;
	}

	std::string indexInfo;
	indexInfo.reserve( conflict.path.size() * 4 + 512 );
	indexInfo += "0 ";
	indexInfo.append( firstStage->objectId.size(), '0' );
	indexInfo += '\t';
	indexInfo += conflict.path;
	indexInfo += '\0';
	for ( const auto* stage : { &conflict.base, &conflict.stage2, &conflict.stage3 } ) {
		if ( !stage->has_value() )
			continue;
		char header[128];
		const int length = std::snprintf( header, sizeof( header ), "%06o %s %u\t",
										  ( *stage )->mode, ( *stage )->objectId.c_str(),
										  static_cast<unsigned int>( ( *stage )->stage ) );
		if ( length <= 0 || static_cast<size_t>( length ) >= sizeof( header ) ) {
			result.returnCode = EXIT_FAILURE;
			return result;
		}
		indexInfo.append( header, static_cast<size_t>( length ) );
		indexInfo += conflict.path;
		indexInfo += '\0';
	}
	result.returnCode =
		git( { "update-index", "-z", "--index-info" }, projectDir, result.result, indexInfo );
	return result;
}

Git::Result Git::preparedMergeMessage( const std::string& projectDir ) const {
	Result result;
	std::string path;
	result.returnCode = git( { "rev-parse", "--path-format=absolute", "--git-path", "MERGE_MSG" },
							 projectDir, path );
	String::trimInPlace( path, sAsciiWhitespace );
	if ( result.success() && FileSystem::fileGet( path, result.result ) )
		return result;

	std::string gitDir;
	result.returnCode =
		git( std::vector<std::string>{ "rev-parse", "--absolute-git-dir" }, projectDir, gitDir );
	String::trimInPlace( gitDir, sAsciiWhitespace );
	if ( result.success() ) {
		FileSystem::dirAddSlashAtEnd( gitDir );
		path = gitDir + "MERGE_MSG";
		if ( FileSystem::fileGet( path, result.result ) )
			return result;
	}

	std::string mergeName;
	result.returnCode = git( std::vector<std::string>{ "name-rev", "--name-only", "--no-undefined",
													   "--refs=refs/heads/*",
													   "--refs=refs/remotes/*", "MERGE_HEAD" },
							 projectDir, mergeName );
	String::trimInPlace( mergeName, sAsciiWhitespace );
	if ( result.fail() || mergeName.empty() ) {
		result.returnCode = git( std::vector<std::string>{ "rev-parse", "--short", "MERGE_HEAD" },
								 projectDir, mergeName );
		String::trimInPlace( mergeName, sAsciiWhitespace );
	}
	if ( result.success() && !mergeName.empty() ) {
		result.result = "Merge '" + mergeName + "'";
		return result;
	}

	// MERGE_HEAD was already verified when the operation state was detected. Keep the commit
	// workflow usable even when the tool that initiated the merge did not create MERGE_MSG.
	result.returnCode = EXIT_SUCCESS;
	result.result = "Merge";
	return result;
}

static std::vector<std::string> operationArgs( Git::GitOperation operation, bool abort ) {
	const char* action = abort ? "--abort" : "--continue";
	switch ( operation ) {
		case Git::GitOperation::Merge:
			return { "-c", "core.editor=true", "merge", action };
		case Git::GitOperation::Rebase:
			return { "-c", "core.editor=true", "rebase", action };
		case Git::GitOperation::CherryPick:
			return { "-c", "core.editor=true", "cherry-pick", action };
		case Git::GitOperation::Revert:
			return { "-c", "core.editor=true", "revert", action };
		case Git::GitOperation::None:
		case Git::GitOperation::StashApply:
			return {};
	}
	return {};
}

Git::Result Git::continueOperation( GitOperation operation, const std::string& projectDir ) const {
	Result result;
	auto args = operationArgs( operation, false );
	if ( args.empty() ) {
		result.returnCode = EXIT_FAILURE;
		return result;
	}
	result.returnCode = git( args, projectDir, result.result );
	return result;
}

Git::Result Git::abortOperation( GitOperation operation, const std::string& projectDir ) const {
	Result result;
	auto args = operationArgs( operation, true );
	if ( args.empty() ) {
		result.returnCode = EXIT_FAILURE;
		return result;
	}
	result.returnCode = git( args, projectDir, result.result );
	return result;
}

void Git::gitSubmodules( const std::string& args, const std::string& projectDir,
						 std::string& buf ) {
	git( String::format( "submodule foreach \"git %s\"", args ), projectDir, buf );
}

bool Git::isGitRepo( const std::string& projectDir ) {
	std::string buf;
	git( "rev-parse --is-inside-work-tree", projectDir, buf );
	String::trimInPlace( buf, sAsciiWhitespace );
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

Git::Result Git::pushNewBranch( const std::string& branch, const std::string& projectDir ) {
	auto res = gitSimple(
		String::format(
			"push --porcelain --recurse-submodules=check origin refs/heads/%s:refs/heads/%s",
			branch, branch ),
		projectDir );

	if ( res.fail() )
		return res;

	gitSimple( String::format( "branch --set-upstream-to=origin/%s %s", branch, branch ),
			   projectDir );

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

static std::string asList( std::vector<std::string>& files ) {
	for ( auto& file : files )
		file = "\"" + file + "\"";
	return String::join( files );
}

Git::Result Git::add( std::vector<std::string> files, const std::string& projectDir ) {
	return gitSimple( String::format( "add --force -- %s", asList( files ) ), projectDir );
}

Git::Result Git::stash( std::vector<std::string> files, const std::string& projectDir ) {
	return gitSimple( String::format( "stash push --include-untracked -- %s", asList( files ) ),
					  projectDir );
}

Git::Result Git::restore( std::vector<std::string> files, const std::string& projectDir ) {
	return gitSimple( String::format( "restore -- %s", asList( files ) ), projectDir );
}

Git::Result Git::restore( const std::string& file, const std::string& projectDir ) {
	return gitSimple( String::format( "restore \"%s\"", file ), projectDir );
}

Git::Result Git::reset( std::vector<std::string> files, const std::string& projectDir ) {
	return gitSimple( String::format( "reset -q HEAD -- %s", asList( files ) ), projectDir );
}

Git::Result Git::diff( DiffMode mode, const std::string& projectDir ) {
	std::string modeTxt;
	switch ( mode ) {
		case DiffHead: {
			modeTxt = "HEAD";
			break;
		}
		case DiffStaged: {
			modeTxt = "--staged";
			break;
		}
	}
	return gitSimple( String::format( "diff %s", modeTxt ), projectDir );
}

Git::Result Git::diff( const std::string& file, bool isStaged, const std::string& projectDir ) {
	return gitSimple( String::format( "diff%s \"%s\"", isStaged ? " --staged" : "", file ),
					  projectDir );
}

Git::Result Git::diffUntracked( const std::string& file, const std::string& projectDir ) {
	const std::string emptyFilePath =
		Sys::getTempPath() + ".ecode-git-empty-" + String::randString( 16 );
	if ( !FileSystem::fileWrite( emptyFilePath, "" ) )
		return { "Could not create temporary file for untracked file diff.", EXIT_FAILURE };

	auto result = gitSimple(
		String::format( "diff --no-index -- \"%s\" \"%s\"", emptyFilePath, file ), projectDir );
	FileSystem::fileRemove( emptyFilePath );

	// git diff --no-index returns 1 when differences were found.
	if ( result.returnCode == 1 && !result.result.empty() ) {
		result.returnCode = 0;
		const auto oldFileHeader = result.result.find( "\n--- " );
		if ( oldFileHeader != std::string::npos ) {
			const auto headerEnd = result.result.find( '\n', oldFileHeader + 1 );
			if ( headerEnd != std::string::npos )
				result.result.replace( oldFileHeader + 1, headerEnd - oldFileHeader - 1,
									   "--- /dev/null" );
		}
	}
	return result;
}

Git::Result Git::showFile( const std::string& file, const std::string& ref,
						   const std::string& projectDir ) {
	std::string relativePath( file );
	const std::string& repoPath = projectDir.empty() ? mProjectPath : projectDir;
	if ( !repoPath.empty() )
		FileSystem::filePathRemoveBasePath( repoPath, relativePath );

	std::string refSpec( ref == ":" ? ":" + relativePath : ref + ":" + relativePath );
	return gitSimple( String::format( "show \"%s\"", refSpec ), projectDir );
}

Git::Result Git::createBranch( const std::string& branchName, bool _checkout,
							   const std::string& projectDir ) {
	auto res = gitSimple( String::format( "branch --no-track %s", branchName ), projectDir );
	if ( _checkout )
		checkout( branchName, projectDir );
	return res;
}

Git::Result Git::renameBranch( const std::string& branch, const std::string& newName,
							   const std::string& projectDir ) {
	return gitSimple( String::format( "branch -M %s %s", branch, newName ), projectDir );
}

Git::Result Git::deleteBranch( const std::string& branch, const std::string& projectDir ) {
	return gitSimple( String::format( "branch -D %s", branch ), projectDir );
}

Git::Result Git::mergeBranch( const std::string& branch, bool fastForward,
							  const std::string& projectDir ) {
	return gitSimple( String::format( "merge %s %s", fastForward ? "--ff" : "--no-ff", branch ),
					  projectDir );
}

Git::Result Git::commit( const std::string& commitMsg, bool amend, bool byPassCommitHook,
						 const std::string& projectDir, bool cleanupComments ) {
	auto tmpPath = Sys::getTempPath() + ".ecode-git-commit-" + String::randString( 16 );
	if ( !FileSystem::fileWrite( tmpPath, commitMsg ) ) {
		Git::Result res;
		res.returnCode = -1;
		res.result = "Could not write commit message into a file";
		return res;
	}
	std::string buf;
	std::string opts;
	if ( amend )
		opts += " --amend";

	if ( byPassCommitHook )
		opts += " --no-verify";

	int retCode = git( String::format( "commit %s --cleanup=%s --allow-empty --file=%s", opts,
									   cleanupComments ? "strip" : "whitespace", tmpPath ),
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
		String::trimInPlace( buf, sAsciiWhitespace );
		auto results = String::split( buf, '\t' );
		if ( results.size() == 2 ) {
			Int64 behind = 0;
			Int64 ahead = 0;
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
	if ( aheadBehind == "gone" ) {
		branch.gone = true;
		return;
	}
	auto split = String::split( aheadBehind, ',' );
	for ( auto s : split ) {
		s = String::trim( s );
		if ( String::startsWith( s, BEHIND ) ) {
			std::string numStr = std::string{ s.substr( BEHIND.size() ) };
			Int64 val = 0;
			if ( String::fromString( val, numStr ) )
				branch.behind = val;
		} else if ( String::startsWith( s, AHEAD ) ) {
			std::string numStr = std::string{ s.substr( AHEAD.size() ) };
			Int64 val = 0;
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
							std::move( commitHash ), "" };
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
							std::move( commitHash ), "" };
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

	if ( EXIT_SUCCESS == git( args, projectDir, buf ) ) {
		branches.reserve( String::countLines( buf ) );

		String::readBySeparator( std::string_view{ buf }, [&]( std::string_view line ) {
			auto branch = String::trim( String::trim( line, '\'' ), '\t' );
			if ( ( ref & Head ) && String::startsWith( branch, "refs/heads/" ) ) {
				auto parsedBranch = parseLocalBranch( branch );
				if ( !parsedBranch.isEmpty() )
					branches.emplace_back( std::move( parsedBranch ) );
			} else if ( ( ref & Remote ) && String::startsWith( branch, "refs/remotes/" ) ) {
				auto parsedBranch = parseRemoteBranch( branch );
				if ( !parsedBranch.isEmpty() )
					branches.emplace_back( std::move( parsedBranch ) );
			} else if ( ( ref & Tag ) && String::startsWith( branch, "refs/tags/" ) ) {
				auto parsedBranch = parseTag( branch );
				if ( !parsedBranch.isEmpty() )
					branches.emplace_back( std::move( parsedBranch ) );
			}
		} );
	}

	if ( ( ref & RefType::Stash ) &&
		 EXIT_SUCCESS == git( "stash list --date=format:\"%Y-%m-%d %H:%M\"", projectDir, buf ) ) {
		branches.reserve( branches.size() + String::countLines( buf ) );
		std::string ptrn( "stash@{(.*)}:%s(.*)" );
		LuaPattern pattern( ptrn );
		Uint64 id = 0;
		String::readBySeparator( std::string_view{ buf }, [&]( std::string_view line ) {
			PatternMatcher::Range matches[3];
			if ( pattern.matches( line.data(), 0, matches, line.size() ) ) {
				std::string date(
					line.substr( matches[1].start, matches[1].end - matches[1].start ) );
				std::string name(
					line.substr( matches[2].start, matches[2].end - matches[2].start ) );
				Git::Branch newBranch;
				newBranch.type = RefType::Stash;
				newBranch.name = std::move( name );
				newBranch.remote = String::format( "stash@{%llu}", id );
				newBranch.date = date;
				if ( !newBranch.isEmpty() )
					branches.emplace_back( std::move( newBranch ) );
				id++;
			}
		} );
	}

	return branches;
}

std::vector<std::string> Git::fetchSubModules( const std::string& projectDir ) {
	std::vector<std::string> submodules;
	std::string buf;
	FileSystem::fileGet( ( !projectDir.empty() ? projectDir : mProjectPath ) + ".gitmodules", buf );
	std::string ptrn( "^%s*path%s*=%s*(.+)" );
	LuaPattern pattern( ptrn );
	String::readBySeparator(
		std::string_view{ buf }, [&pattern, &submodules]( std::string_view line ) {
			PatternMatcher::Range matches[2];
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

std::string Git::repoName( std::string file, bool allowExactMatch, const std::string& projectDir ) {
	if ( String::startsWith( file, !projectDir.empty() ? projectDir : mProjectPath ) )
		FileSystem::filePathRemoveBasePath( !projectDir.empty() ? projectDir : mProjectPath, file );
	Lock l( mSubModulesMutex );
	for ( const auto& subRepo : mSubModules ) {
		if ( String::startsWith( file, subRepo ) &&
			 ( allowExactMatch || file.size() != subRepo.size() ) )
			return subRepo;
	}
	return FileSystem::fileNameFromPath( !projectDir.empty() ? projectDir : mProjectPath );
}

std::string Git::repoPath( const std::string& file ) {
	Lock l( mSubModulesMutex );
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

	std::string enteringPtrn( "^Entering '(.*)'" );
	LuaPattern subModulePattern( enteringPtrn );

	bool modifiedSubmodule = false;

	std::vector<std::string> curSubModules;
	{
		Lock l( mSubModulesMutex );
		curSubModules = mSubModules;
	}

	const auto isSubmodule = [&curSubModules]( const std::string_view& file ) -> bool {
		return std::any_of( curSubModules.begin(), curSubModules.end(),
							[&file]( const auto& submodule ) { return submodule == file; } );
	};

	auto parseStatus = [&s, &buf, &modifiedSubmodule, &projectDir, this, &subModulePattern,
						submodules, &isSubmodule]() {
		std::string subModulePath = "";
		std::string ptrn = "^([mMARTUD?%s][mMARTUD?%s])%s(.*)";
		LuaPattern pattern( ptrn );
		size_t changesCount = String::countLines( buf );

		if ( changesCount > 1000 )
			return;

		String::readBySeparator( std::string_view{ buf }, [&]( std::string_view line ) {
			PatternMatcher::Range matches[3];
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

				if ( submodules && !mSubModules.empty() && isSubmodule( file ) )
					modifiedSubmodule = true;

				bool isStagedAndModified =
					status.type == GitStatusType::Staged && statusStr[1] != ' ';

				if ( status.symbol == GitStatusChar::Renamed ) {
					std::string rptrn( ".*%s%-%>%s(.*)" );
					LuaPattern rpattern( rptrn );
					PatternMatcher::Range rranges[2];
					if ( rpattern.matches( file.data(), 0, rranges, file.size() ) )
						file = file.substr( rranges[1].start, rranges[1].end - rranges[1].start );
				}

				std::string filePath = subModulePath + file;
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
					s.files[repo].push_back( { filePath, 0, 0, status } );

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
		std::string ptrn( "([-%d]+)%s+([-%d]+)%s+(.+)" );
		LuaPattern pattern( ptrn );
		std::string subModulePath = "";
		String::readBySeparator( std::string_view{ buf }, [&]( std::string_view line ) {
			PatternMatcher::Range matches[4];
			if ( subModulePattern.matches( line.data(), 0, matches, line.size() ) ) {
				subModulePath = String::trim(
					line.substr( matches[1].start, matches[1].end - matches[1].start ) );
				FileSystem::dirAddSlashAtEnd( subModulePath );
			} else if ( pattern.matches( line.data(), 0, matches, line.size() ) ) {
				auto inserted = line.substr( matches[1].start, matches[1].end - matches[1].start );
				auto deleted = line.substr( matches[2].start, matches[2].end - matches[2].start );
				std::string file = std::string{
					line.substr( matches[3].start, matches[3].end - matches[3].start ) };
				int inserts = 0;
				int deletes = 0;
				bool isBinary = inserted == "-" || deleted == "-";
				if ( !isBinary ) {
					String::fromString( inserts, inserted );
					String::fromString( deletes, deleted );
				}

				if ( isBinary || ( inserts || deletes ) ) {
					std::string rptrn( "(.*)%{.*%s->%s(.*)%}" );
					LuaPattern pattern( rptrn );
					if ( pattern.matches( file.data(), 0, matches, file.size() ) ) {
						file = file.substr( matches[1].start, matches[1].end - matches[1].start ) +
							   file.substr( matches[2].start, matches[2].end - matches[2].start );
					}

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
								fileIt.isBinary = isBinary;
								found = true;
								break;
							}
						}
					}
					if ( !found ) {
						s.files[repo].push_back(
							{ std::move( filePath ), inserts, deletes, status, isBinary } );
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
			if ( !val.isBinary && val.report.symbol == GitStatusChar::Added && val.inserts == 0 ) {
				val.inserts = FileSystem::fileCountLines(
					( projectDir.empty() ? mProjectPath : projectDir ) + val.file, &val.isBinary );
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

	if ( commitHash == sNotCommittedYetHash )
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

Git::Result Git::stashPush( std::vector<std::string> files, const std::string& name, bool keepIndex,
							const std::string& projectDir ) {
	std::string args;
	if ( keepIndex )
		args += " --keep-index";
	if ( !name.empty() )
		args += " --message \"" + name + "\"";
	return gitSimple(
		String::format( "stash push --include-untracked %s -- %s", args, asList( files ) ),
		projectDir );
}

Git::Result Git::stashApply( const std::string& stashId, bool restoreIndex,
							 const std::string& projectDir ) {
	return gitSimple( String::format( "stash apply%s %s", restoreIndex ? " --index" : "", stashId ),
					  projectDir );
}

Git::Result Git::stashDrop( const std::string& stashId, const std::string& projectDir ) {
	return gitSimple( String::format( "stash drop %s", stashId ), projectDir );
}

} // namespace ecode
