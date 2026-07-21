#include "projectsearch.hpp"
#include <eepp/system/filemapped.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/regex.hpp>

namespace ecode {

static constexpr size_t PROJECT_SEARCH_MMAP_THRESHOLD = 256 * 1024;

static int countNewLines( std::string_view text, const size_t& start, const size_t& end ) {
	if ( start >= end || start >= text.size() )
		return 0;

	const size_t realEnd = std::min( end, text.size() );
	const char* startPtr = text.data() + start;
	const char* endPtr = text.data() + realEnd;

	size_t count = *startPtr == '\n' ? 1 : 0;

	while ( ++startPtr != endPtr ) {
		if ( '\n' == *startPtr )
			count++;
	}

	return count;
}

static String textLine( std::string_view fileText, const size_t& fromPos, Int64& relCol ) {
	if ( fileText.empty() || fromPos >= fileText.size() ) {
		relCol = 0;
		return {};
	}

	const char* stringStartPtr = fileText.data();
	const char* stringEndPtr = fileText.data() + fileText.size();
	const char* startPtr = fileText.data() + fromPos;
	const char* endPtr = startPtr;
	const char* nlStartPtr = startPtr;

	while ( nlStartPtr != stringStartPtr && *( nlStartPtr - 1 ) != '\n' )
		--nlStartPtr;

	while ( endPtr != stringEndPtr && *endPtr != '\n' )
		++endPtr;

	relCol = String::utf8Length(
		std::string_view( nlStartPtr, static_cast<size_t>( startPtr - nlStartPtr ) ) );

	const size_t lineLen = static_cast<size_t>( endPtr - nlStartPtr );
	const size_t copyLen = lineLen > EE_1KB ? EE_1KB : lineLen;

	return String( std::string_view( nlStartPtr, copyLen ) );
}

static std::vector<ProjectSearch::ResultData::Result>
searchInTextHorspool( std::string_view fileText, const std::string& text, const bool& caseSensitive,
					  const bool& wholeWord, const String::BMH::OccTable& occ ) {
	std::vector<ProjectSearch::ResultData::Result> res;

	if ( fileText.empty() )
		return res;

	Int64 lSearchRes = 0;
	Int64 searchRes = 0;
	size_t totNl = 0;
	const bool caseInsensitive = !caseSensitive;

	do {
		searchRes = String::BMH::find( fileText, text, searchRes, occ, caseInsensitive );

		if ( searchRes != -1 ) {
			if ( wholeWord &&
				 !String::isWholeWord( fileText, std::string_view{ text }, searchRes ) ) {
				totNl += countNewLines( fileText, lSearchRes, searchRes );
				lSearchRes = searchRes;
				searchRes += text.size();
				continue;
			}

			Int64 relCol;
			totNl += countNewLines( fileText, lSearchRes, searchRes );

			String str( textLine( fileText, searchRes, relCol ) );

			res.push_back( { std::move( str ),
							 { { static_cast<Int64>( totNl ), static_cast<Int64>( relCol ) },
							   { static_cast<Int64>( totNl ),
								 static_cast<Int64>( relCol + String::utf8Length( text ) ) } },
							 searchRes,
							 static_cast<Int64>( searchRes + text.size() ) } );

			lSearchRes = searchRes;
			searchRes += text.size();
		}
	} while ( searchRes != -1 );

	return res;
}

static std::vector<ProjectSearch::ResultData::Result>
searchInFileHorspoolMapped( const std::string& file, const std::string& text,
							const bool& caseSensitive, const bool& wholeWord,
							const String::BMH::OccTable& occ ) {
	FileMapped mapped( file );
	if ( !mapped.isOpen() || mapped.empty() )
		return {};

	return searchInTextHorspool( mapped.view(), text, caseSensitive, wholeWord, occ );
}

static std::vector<ProjectSearch::ResultData::Result>
searchInFileHorspoolLoaded( const std::string& file, const std::string& text,
							const bool& caseSensitive, const bool& wholeWord,
							const String::BMH::OccTable& occ ) {
	thread_local std::string fileText;

	if ( !FileSystem::fileGet( file, fileText ) || fileText.empty() )
		return {};

	auto res = searchInTextHorspool( std::string_view{ fileText.data(), fileText.size() }, text,
									 caseSensitive, wholeWord, occ );

	return res;
}

static std::vector<ProjectSearch::ResultData::Result>
searchInFileHorspool( const std::string& file, const std::string& text, const bool& caseSensitive,
					  const bool& wholeWord, const String::BMH::OccTable& occ ) {
	const auto fileSize = FileSystem::fileSize( file );

	if ( fileSize >= 0 && static_cast<size_t>( fileSize ) <= PROJECT_SEARCH_MMAP_THRESHOLD )
		return searchInFileHorspoolLoaded( file, text, caseSensitive, wholeWord, occ );

	return searchInFileHorspoolMapped( file, text, caseSensitive, wholeWord, occ );
}

static std::vector<ProjectSearch::ResultData::Result>
searchInFilePatternMatch( const std::string& file, PatternMatcher& pattern,
						  const bool& caseSensitive, const bool& wholeWord ) {
	thread_local std::string fileText;
	fileText.clear();

	if ( !FileSystem::fileGet( file, fileText ) || fileText.empty() )
		return {};

	FileSystem::fileGet( file, fileText );
	std::vector<ProjectSearch::ResultData::Result> results;
	Int64 totNl = 0;
	bool matched = false;
	Int64 searchRes = 0;
	thread_local std::string fileTextOriginal;
	fileTextOriginal.clear();

	if ( !caseSensitive ) {
		fileTextOriginal = fileText;
		String::toLowerInPlace( fileText );
	}

	PatternMatcher::Range matches[12];
	do {
		int start, end = 0;

		if ( ( matched = pattern.matches( fileText, matches, searchRes ) ) ) {
			start = matches[0].start;
			end = matches[0].end;

			if ( wholeWord &&
				 !String::isWholeWord( fileText, fileText.substr( start, end - start ), start ) ) {
				searchRes = end;
				continue;
			}

			Int64 relCol;
			totNl += countNewLines( fileText, searchRes, start );
			String str( textLine( caseSensitive ? fileText : fileTextOriginal, start, relCol ) );
			int len = end - start;
			ProjectSearch::ResultData::Result res;
			res.line = std::move( str );
			res.position = { { totNl, (Int64)relCol }, { totNl, (Int64)( relCol + len ) } };
			res.start = start;
			res.end = end;
			for ( size_t c = 1; c < 12; c++ ) {
				if ( matches[c].isValid() ) {
					res.captures.push_back(
						fileText.substr( matches[c].start, matches[c].end - matches[c].start ) );
				} else {
					break;
				}
			}
			results.emplace_back( std::move( res ) );
			searchRes = end;
		}
	} while ( matched );

	return results;
}

static std::vector<ProjectSearch::ResultData::Result>
searchInFileLuaPattern( const std::string& file, const std::string& text, const bool& caseSensitive,
						const bool& wholeWord ) {
	LuaPattern pattern( text );
	return searchInFilePatternMatch( file, pattern, caseSensitive, wholeWord );
}

static std::vector<ProjectSearch::ResultData::Result> searchInFileRegEx( const std::string& file,
																		 const std::string& text,
																		 const bool& caseSensitive,
																		 const bool& wholeWord ) {
	RegEx pattern( text, static_cast<RegEx::Options>( RegEx::Options::Utf |
													  ( !caseSensitive ? RegEx::Options::Caseless
																	   : RegEx::Options::None ) ) );
	return searchInFilePatternMatch( file, pattern, caseSensitive, wholeWord );
}

std::vector<ProjectSearch::ResultData::Result>
ProjectSearch::fileResFromDoc( const std::string& string, bool caseSensitive, bool wholeWord,
							   TextDocument::FindReplaceType type,
							   std::shared_ptr<TextDocument> doc ) {
	auto res = doc->findAll( string, caseSensitive, wholeWord, type );
	std::vector<ProjectSearch::ResultData::Result> fileRes;
	for ( const auto& r : res ) {
		ProjectSearch::ResultData::Result f;
		f.position = r.result;
		auto lineLen = doc->getLineLength( r.result.start().line() );
		if ( lineLen > EE_1KB )
			f.line = doc->getLineTextSubStr( r.result.start().line(), 0, EE_1KB );
		else
			f.line = doc->getLineTextWithoutNewLine( r.result.start().line() );
		f.start = r.result.start().column();
		f.end = r.result.end().column();
		std::vector<std::string> captures;
		for ( const auto& capture : r.captures )
			captures.emplace_back( doc->getText( capture ).toUtf8() );
		f.captures = std::move( captures );
		fileRes.emplace_back( std::move( f ) );
	}

	return fileRes;
}

ProjectSearch::FindData*
ProjectSearch::find( const std::vector<std::string> files, std::string string,
					 std::shared_ptr<ThreadPool> pool, ResultCb result, bool caseSensitive,
					 bool wholeWord, const TextDocument::FindReplaceType& type,
					 const std::vector<GlobMatch>& pathFilters, std::string basePath,
					 std::vector<std::shared_ptr<TextDocument>> openDocs ) {
	static const std::string_view PROJECT_SEARCH_TASK_TAG = "ProjectSearchFindTag";
	static const Uint64 PROJECT_SEARCH_TASK_TAG_HASH =
		std::hash<std::string_view>()( PROJECT_SEARCH_TASK_TAG );

	if ( files.empty() ) {
		result( {} );
		return nullptr;
	}
	FindData* findData = eeNew( FindData, () );
	findData->taskTag = PROJECT_SEARCH_TASK_TAG_HASH;

	FileSystem::dirAddSlashAtEnd( basePath );
	pool->run( [findData, files = std::move( files ), string = std::move( string ),
				pool = std::move( pool ), result = std::move( result ), caseSensitive, wholeWord,
				type, pathFilters = std::move( pathFilters ), basePath = std::move( basePath ),
				openDocs = std::move( openDocs )]() mutable {
		SearchConfig searchConfig( string, caseSensitive, wholeWord, type );
		findData->resCount = files.size();
		if ( !caseSensitive && type != TextDocument::FindReplaceType::Normal )
			String::toLowerInPlace( string );
		const auto occ = type == TextDocument::FindReplaceType::Normal
							 ? String::BMH::createOccTable(
								   reinterpret_cast<const unsigned char*>( string.data() ),
								   string.size(), !caseSensitive )
							 : String::BMH::OccTable();
		std::vector<bool> search;
		search.resize( files.size() );
		size_t pos = 0;
		size_t count = 0;
		bool hasPositiveFilter =
			std::find_if( pathFilters.begin(), pathFilters.end(),
						  []( auto filter ) { return !filter.second; } ) != pathFilters.end();

		for ( const auto& file : files ) {
			bool skip = hasPositiveFilter;
			std::string_view fsv( file );
			if ( !basePath.empty() && String::startsWith( file, basePath ) )
				fsv = fsv.substr( basePath.size() );

			for ( const auto& filter : pathFilters ) {
				bool matches = String::globMatch( fsv, filter.first );

				// if it's inverted and the file matches with the glob it must be ignored/excluded
				if ( filter.second && matches ) {
					skip = true;
					break;
				}

				// if the file is not inverted and the file matches with the glob, then it must be
				// not skipped
				if ( !filter.second && matches ) {
					skip = false;
					break;
				}
			}

			if ( !findData->active ) {
				result( {} );
				eeDelete( findData );
				return;
			}

			if ( skip ) {
				search[pos++] = false;
				continue;
			}

			search[pos++] = true;
			count++;
		}

		findData->resCount = count;

		if ( count == 0 || !findData->active ) {
			result( { searchConfig, findData->res } );
			eeDelete( findData );
			return;
		}

		std::unordered_map<std::string, std::shared_ptr<TextDocument>> openPaths;
		for ( const auto& doc : openDocs )
			if ( doc->isDirty() )
				openPaths.insert( { doc->getFilePath(), doc } );

		pos = 0;
		for ( const auto& file : files ) {
			if ( !search[pos] ) {
				pos++;
				continue;
			}

			pos++;

			const auto onSearchEnd = [result, findData, searchConfig]( const auto& ) {
				int count;
				{
					Lock l( findData->countMutex );
					findData->resCount--;
					count = findData->resCount;
				}
				if ( count == 0 ) {
					result( { searchConfig, findData->res } );
					eeDelete( findData );
				}
			};

			auto openPath = openPaths.find( file );
			bool openDoc = openPath != openPaths.end();
			std::shared_ptr<TextDocument> doc = nullptr;
			if ( openDoc )
				doc = openPath->second;

			if ( openDoc && openPath->second->isDirty() ) {
				pool->run(
					[findData, doc, string, caseSensitive, wholeWord, type] {
						std::vector<ProjectSearch::ResultData::Result> fileRes =
							fileResFromDoc( string, caseSensitive, wholeWord, type, doc );

						if ( !fileRes.empty() ) {
							Lock l( findData->resMutex );
							std::string file( doc->getFilePath() );
							findData->res.emplace_back( std::move( file ), std::move( fileRes ),
														doc );
						}
					},
					onSearchEnd, PROJECT_SEARCH_TASK_TAG_HASH );
			} else {
				pool->run(
					[findData, file, string, caseSensitive, wholeWord, occ, type]() mutable {
						auto fileRes = type == TextDocument::FindReplaceType::Normal
										   ? searchInFileHorspool( file, string, caseSensitive,
																   wholeWord, occ )
										   : ( type == TextDocument::FindReplaceType::LuaPattern
												   ? searchInFileLuaPattern(
														 file, string, caseSensitive, wholeWord )
												   : searchInFileRegEx( file, string, caseSensitive,
																		wholeWord ) );
						if ( !fileRes.empty() ) {
							Lock l( findData->resMutex );
							findData->res.emplace_back( std::string( file ), std::move( fileRes ) );
						}
					},
					onSearchEnd, PROJECT_SEARCH_TASK_TAG_HASH );
			}
		}
	} );
	return findData;
}

void ProjectSearch::ResultModel::removeLastNewLineCharacter() {
	for ( auto& r : mResult ) {
		for ( auto& r2 : r.results )
			if ( !r2.line.empty() && r2.line.back() == '\n' )
				r2.line.pop_back();
	}
}

} // namespace ecode
