#include "usersnippetstore.hpp"
#include "../../jsonhelper.hpp"

#include <algorithm>
#include <cctype>
#include <eepp/core/string.hpp>
#include <limits>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ecode {

namespace {

static std::string normalizeScope( std::string scope ) {
	String::trimInPlace( scope );
	String::toLowerInPlace( scope );
	return scope;
}

static SmallVector<std::string, 2> parseScopes( const json& definition, std::string defaultScope ) {
	SmallVector<std::string, 2> scopes;
	if ( !defaultScope.empty() ) {
		defaultScope = normalizeScope( std::move( defaultScope ) );
		if ( !defaultScope.empty() )
			scopes.emplace_back( std::move( defaultScope ) );
		return scopes;
	}
	if ( !definition.contains( "scope" ) || !definition["scope"].is_string() )
		return scopes;
	for ( auto& scope : String::split( definition["scope"].get<std::string>(), ',' ) ) {
		auto normalized = normalizeScope( std::move( scope ) );
		if ( !normalized.empty() &&
			 std::find( scopes.begin(), scopes.end(), normalized ) == scopes.end() )
			scopes.emplace_back( std::move( normalized ) );
	}
	return scopes;
}

static bool parseStringList( const json& value, SmallVector<std::string, 2>& strings,
							 bool rejectEmpty ) {
	if ( value.is_string() ) {
		auto string = value.get<std::string>();
		if ( rejectEmpty && string.empty() )
			return false;
		strings.emplace_back( std::move( string ) );
		return true;
	}
	if ( !value.is_array() )
		return false;
	for ( const auto& item : value ) {
		if ( !item.is_string() )
			return false;
		auto string = item.get<std::string>();
		if ( rejectEmpty && string.empty() )
			continue;
		if ( std::find( strings.begin(), strings.end(), string ) == strings.end() )
			strings.emplace_back( std::move( string ) );
	}
	return !strings.empty() || !rejectEmpty;
}

static void normalizeFilePatterns( SmallVector<std::string, 2>& patterns ) {
	for ( auto& pattern : patterns )
		String::replaceAll( pattern, "\\", "/" );
}

static bool parseBody( const json& value, std::string& body ) {
	if ( value.is_string() ) {
		body = value.get<std::string>();
		return true;
	}
	if ( !value.is_array() )
		return false;
	bool first = true;
	for ( const auto& line : value ) {
		if ( !line.is_string() )
			return false;
		if ( !first )
			body += '\n';
		body += line.get_ref<const std::string&>();
		first = false;
	}
	return true;
}

static int sourcePriority( UserSnippetSource source ) {
	switch ( source ) {
		case UserSnippetSource::EcodeProject:
			return 2;
		case UserSnippetSource::VSCodeProject:
			return 1;
		case UserSnippetSource::User:
		default:
			return 0;
	}
}

static bool matchesFile( const UserSnippetDefinition& snippet, std::string_view filePath ) {
	if ( !snippet.includePatterns.empty() ) {
		bool included = false;
		for ( const auto& pattern : snippet.includePatterns ) {
			if ( String::globMatch( filePath, pattern ) ) {
				included = true;
				break;
			}
		}
		if ( !included )
			return false;
	}
	for ( const auto& pattern : snippet.excludePatterns )
		if ( String::globMatch( filePath, pattern ) )
			return false;
	return true;
}

} // namespace

UserSnippetParseResult UserSnippetStore::parseFile( std::string_view contents,
													std::string sourcePath,
													UserSnippetSource source,
													std::string defaultScope ) {
	UserSnippetParseResult result;
	const std::string sanitized = json_strip_trailing_commas( contents );
	json root = json::parse( sanitized, nullptr, false, true );
	if ( root.is_discarded() || !root.is_object() ) {
		result.diagnostics.emplace_back( sourcePath + ": invalid JSONC root" );
		return result;
	}
	result.valid = true;
	result.snippets.reserve( root.size() );
	for ( const auto& [name, value] : root.items() ) {
		if ( name == "$schema" && value.is_string() )
			continue;
		if ( !value.is_object() ) {
			result.diagnostics.emplace_back( sourcePath + ": snippet '" + name +
											 "' must be an object" );
			continue;
		}
		if ( !value.contains( "prefix" ) || !value.contains( "body" ) ) {
			result.diagnostics.emplace_back( sourcePath + ": snippet '" + name +
											 "' requires prefix and body" );
			continue;
		}
		UserSnippetDefinition snippet;
		snippet.name = name;
		snippet.sourcePath = sourcePath;
		snippet.source = source;
		if ( !parseStringList( value["prefix"], snippet.prefixes, true ) ||
			 !parseBody( value["body"], snippet.body ) ) {
			result.diagnostics.emplace_back( sourcePath + ": snippet '" + name +
											 "' has an invalid prefix or body" );
			continue;
		}
		if ( value.contains( "description" ) && value["description"].is_string() )
			snippet.description = value["description"].get<std::string>();
		if ( value.contains( "scope" ) && !value["scope"].is_string() && defaultScope.empty() ) {
			result.diagnostics.emplace_back( sourcePath + ": snippet '" + name +
											 "' has an invalid scope" );
			continue;
		}
		snippet.scopes = parseScopes( value, defaultScope );
		if ( value.contains( "include" ) &&
			 !parseStringList( value["include"], snippet.includePatterns, true ) ) {
			result.diagnostics.emplace_back( sourcePath + ": snippet '" + name +
											 "' has an invalid include pattern" );
			continue;
		}
		if ( value.contains( "exclude" ) &&
			 !parseStringList( value["exclude"], snippet.excludePatterns, true ) ) {
			result.diagnostics.emplace_back( sourcePath + ": snippet '" + name +
											 "' has an invalid exclude pattern" );
			continue;
		}
		normalizeFilePatterns( snippet.includePatterns );
		normalizeFilePatterns( snippet.excludePatterns );
		result.snippets.emplace_back( std::move( snippet ) );
	}
	return result;
}

bool UserSnippetStore::updateFile( std::string_view contents, std::string sourcePath,
								   UserSnippetSource source, std::string defaultScope,
								   std::vector<std::string>* diagnostics ) {
	const String::HashType hash = String::hash( contents );
	{
		Lock lock( mMutex );
		auto found = mFiles.find( sourcePath );
		if ( found != mFiles.end() && found->second.hash == hash ) {
			if ( diagnostics )
				diagnostics->clear();
			return true;
		}
	}
	auto parsed = parseFile( contents, sourcePath, source, std::move( defaultScope ) );
	if ( diagnostics )
		*diagnostics = std::move( parsed.diagnostics );
	if ( !parsed.valid )
		return false;
	Lock lock( mMutex );
	mFiles[sourcePath] = { source, hash, std::move( parsed.snippets ) };
	rebuildSnapshot();
	return true;
}

bool UserSnippetStore::removeFile( std::string_view sourcePath ) {
	Lock lock( mMutex );
	auto found = mFiles.find( std::string( sourcePath ) );
	if ( found == mFiles.end() )
		return false;
	mFiles.erase( found );
	rebuildSnapshot();
	return true;
}

void UserSnippetStore::removeSource( UserSnippetSource source ) {
	Lock lock( mMutex );
	bool changed = false;
	for ( auto it = mFiles.begin(); it != mFiles.end(); ) {
		if ( it->second.source == source ) {
			it = mFiles.erase( it );
			changed = true;
		} else {
			++it;
		}
	}
	if ( changed )
		rebuildSnapshot();
}

void UserSnippetStore::clear() {
	Lock lock( mMutex );
	mFiles.clear();
	mSnapshot = std::make_shared<Snapshot>();
}

void UserSnippetStore::rebuildSnapshot() {
	auto snapshot = std::make_shared<Snapshot>();
	size_t count = 0;
	for ( const auto& file : mFiles )
		count += file.second.snippets.size();
	snapshot->snippets.reserve( count );
	for ( const auto& file : mFiles ) {
		for ( const auto& snippet : file.second.snippets ) {
			const size_t index = snapshot->snippets.size();
			snapshot->snippets.emplace_back( snippet );
			if ( snippet.scopes.empty() ) {
				snapshot->global.emplace_back( index );
			} else {
				for ( const auto& scope : snippet.scopes )
					snapshot->byLanguage[scope].emplace_back( index );
			}
		}
	}
	mSnapshot = std::move( snapshot );
}

std::vector<UserSnippetMatch> UserSnippetStore::find( std::string_view language,
													  std::string_view pattern, size_t maxResults,
													  std::string_view filePath ) const {
	if ( maxResults == 0 )
		return {};
	std::shared_ptr<const Snapshot> snapshot;
	{
		Lock lock( mMutex );
		snapshot = mSnapshot;
	}
	std::string normalizedLanguage( language );
	std::string normalizedPath( filePath );
	String::replaceAll( normalizedPath, "\\", "/" );
	String::toLowerInPlace( normalizedLanguage );
	std::vector<size_t> candidates;
	candidates.reserve( snapshot->global.size() + 32 );
	candidates.insert( candidates.end(), snapshot->global.begin(), snapshot->global.end() );
	auto languageIt = snapshot->byLanguage.find( normalizedLanguage );
	if ( languageIt != snapshot->byLanguage.end() )
		candidates.insert( candidates.end(), languageIt->second.begin(), languageIt->second.end() );

	SmallVector<std::string, 16> inputs;
	if ( !pattern.empty() ) {
		for ( size_t offset = 0; offset < pattern.size(); ++offset ) {
			const Uint8 current = static_cast<Uint8>( pattern[offset] );
			if ( offset > 0 && ( current & 0xC0 ) == 0x80 )
				continue;
			if ( offset > 0 ) {
				const Uint8 previous = static_cast<Uint8>( pattern[offset - 1] );
				if ( previous >= 0x80 || std::isalnum( previous ) || previous == '_' )
					continue;
			}
			inputs.emplace_back( pattern.substr( offset ) );
		}
	}
	static constexpr size_t NO_INPUT = std::numeric_limits<size_t>::max();
	struct Candidate {
		size_t snippetIndex;
		size_t prefixIndex;
		size_t inputIndex;
		int score;
	};
	std::vector<Candidate> matchedCandidates;
	matchedCandidates.reserve( eemin( maxResults, candidates.size() ) );
	for ( size_t index : candidates ) {
		const auto& snippet = snapshot->snippets[index];
		if ( !matchesFile( snippet, normalizedPath ) )
			continue;
		int bestScore = std::numeric_limits<int>::min();
		size_t bestPrefix = NO_INPUT;
		size_t bestInput = NO_INPUT;
		for ( size_t prefixIndex = 0; prefixIndex < snippet.prefixes.size(); ++prefixIndex ) {
			const auto& prefix = snippet.prefixes[prefixIndex];
			if ( pattern.empty() ) {
				if ( bestPrefix == NO_INPUT ) {
					bestScore = 0;
					bestPrefix = prefixIndex;
				}
				continue;
			}
			for ( size_t inputIndex = 0; inputIndex < inputs.size(); ++inputIndex ) {
				const auto& input = inputs[inputIndex];
				const int score = String::fuzzyMatchSimple( input, prefix, false, true );
				if ( score <= 0 )
					continue;
				const int weightedScore =
					score + static_cast<int>( String::utf8Length( input ) * 1000 );
				if ( weightedScore > bestScore ) {
					bestScore = weightedScore;
					bestPrefix = prefixIndex;
					bestInput = inputIndex;
				}
				break;
			}
		}
		if ( bestPrefix == NO_INPUT || ( !pattern.empty() && bestScore <= 0 ) )
			continue;
		matchedCandidates.push_back( { index, bestPrefix, bestInput, bestScore } );
	}
	std::sort( matchedCandidates.begin(), matchedCandidates.end(),
			   [&]( const auto& left, const auto& right ) {
				   if ( left.score != right.score )
					   return left.score > right.score;
				   const auto& leftSnippet = snapshot->snippets[left.snippetIndex];
				   const auto& rightSnippet = snapshot->snippets[right.snippetIndex];
				   const int leftPriority = sourcePriority( leftSnippet.source );
				   const int rightPriority = sourcePriority( rightSnippet.source );
				   if ( leftPriority != rightPriority )
					   return leftPriority > rightPriority;
				   return leftSnippet.name < rightSnippet.name;
			   } );
	if ( matchedCandidates.size() > maxResults )
		matchedCandidates.resize( maxResults );
	std::vector<UserSnippetMatch> matches;
	matches.reserve( matchedCandidates.size() );
	for ( const auto& candidate : matchedCandidates ) {
		const auto& snippet = snapshot->snippets[candidate.snippetIndex];
		matches.push_back(
			{ snippet, snippet.prefixes[candidate.prefixIndex],
			  candidate.inputIndex != NO_INPUT ? inputs[candidate.inputIndex] : std::string{},
			  candidate.score } );
	}
	return matches;
}

std::vector<UserSnippetMatch> UserSnippetStore::findForLocator( std::string_view language,
																std::string_view pattern,
																size_t maxResults,
																std::string_view filePath ) const {
	if ( maxResults == 0 )
		return {};
	std::shared_ptr<const Snapshot> snapshot;
	{
		Lock lock( mMutex );
		snapshot = mSnapshot;
	}
	std::string normalizedLanguage( language );
	const std::string query( pattern );
	std::string normalizedPath( filePath );
	String::replaceAll( normalizedPath, "\\", "/" );
	String::toLowerInPlace( normalizedLanguage );
	std::vector<size_t> candidates;
	candidates.reserve( snapshot->global.size() + 32 );
	candidates.insert( candidates.end(), snapshot->global.begin(), snapshot->global.end() );
	auto languageIt = snapshot->byLanguage.find( normalizedLanguage );
	if ( languageIt != snapshot->byLanguage.end() )
		candidates.insert( candidates.end(), languageIt->second.begin(), languageIt->second.end() );

	struct Candidate {
		size_t snippetIndex;
		int score;
	};
	std::vector<Candidate> matches;
	matches.reserve( eemin( maxResults, candidates.size() ) );
	const auto matchScore = [&query]( const std::string& value ) {
		const int score = String::fuzzyMatchSimple( query, value, false, true );
		return score > 0 ? score : String::icontains( value, query ) ? 1 : 0;
	};
	for ( size_t index : candidates ) {
		const auto& snippet = snapshot->snippets[index];
		if ( !matchesFile( snippet, normalizedPath ) )
			continue;
		int score = query.empty() ? 0 : matchScore( snippet.name );
		if ( !pattern.empty() ) {
			for ( const auto& prefix : snippet.prefixes )
				score = eemax( score, matchScore( prefix ) );
			if ( !snippet.description.empty() )
				score = eemax( score, matchScore( snippet.description ) );
		}
		if ( pattern.empty() || score > 0 )
			matches.push_back( { index, score } );
	}
	std::sort( matches.begin(), matches.end(),
			   [&]( const Candidate& left, const Candidate& right ) {
				   if ( left.score != right.score )
					   return left.score > right.score;
				   const auto& leftSnippet = snapshot->snippets[left.snippetIndex];
				   const auto& rightSnippet = snapshot->snippets[right.snippetIndex];
				   const int leftPriority = sourcePriority( leftSnippet.source );
				   const int rightPriority = sourcePriority( rightSnippet.source );
				   if ( leftPriority != rightPriority )
					   return leftPriority > rightPriority;
				   return leftSnippet.name < rightSnippet.name;
			   } );
	if ( matches.size() > maxResults )
		matches.resize( maxResults );
	std::vector<UserSnippetMatch> results;
	results.reserve( matches.size() );
	for ( const auto& match : matches ) {
		const auto& snippet = snapshot->snippets[match.snippetIndex];
		results.push_back( { snippet, snippet.prefixes.front(), {}, match.score } );
	}
	return results;
}

size_t UserSnippetStore::size() const {
	Lock lock( mMutex );
	return mSnapshot->snippets.size();
}

} // namespace ecode
