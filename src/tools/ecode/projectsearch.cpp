#include "projectsearch.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/luapattern.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX
// For malloc_trim, which is a GNU extension
extern "C" {
#include <malloc.h>
}
#endif

namespace ecode {

static int countNewLines( const std::string& text, const size_t& start, const size_t& end ) {
	const char* startPtr = text.c_str() + start;
	const char* endPtr = text.c_str() + end;
	size_t count = 0;
	if ( startPtr != endPtr ) {
		count = *startPtr == '\n' ? 1 : 0;
		while ( ++startPtr && startPtr != endPtr ) {
			if ( '\n' == *startPtr )
				count++;
		};
	}
	return count;
}

static String textLine( const std::string& fileText, const size_t& fromPos, Int64& relCol ) {
	const char* stringStartPtr = fileText.c_str();
	const char* startPtr = fileText.c_str() + fromPos;
	const char* endPtr = startPtr;
	const char* nlStartPtr = startPtr;
	while ( nlStartPtr != stringStartPtr && *nlStartPtr != '\n' )
		--nlStartPtr;
	if ( *nlStartPtr == '\n' )
		nlStartPtr++;
	while ( ++endPtr && *endPtr != '\0' && *endPtr != '\n' ) {
	}
	relCol =
		String::utf8Length( fileText.substr( nlStartPtr - stringStartPtr, startPtr - nlStartPtr ) );
	// if the line to substract is massive we only get the fist kilobyte of that line, since the
	// line is only shared for visual aid.
	return fileText.substr( nlStartPtr - stringStartPtr,
							endPtr - nlStartPtr > EE_1KB ? EE_1KB : endPtr - nlStartPtr );
}

static std::vector<ProjectSearch::ResultData::Result>
searchInFileHorspool( const std::string& file, const std::string& text, const bool& caseSensitive,
					  const bool& wholeWord, const String::BMH::OccTable& occ ) {
	std::vector<ProjectSearch::ResultData::Result> res;
	std::string fileText;
	Int64 lSearchRes = 0;
	Int64 searchRes = 0;
	size_t totNl = 0;
	FileSystem::fileGet( file, fileText );
	std::string fileTextOriginal;

	if ( !caseSensitive ) {
		fileTextOriginal = fileText;
		String::toLowerInPlace( fileText );
	}

	do {
		searchRes = String::BMH::find( fileText, text, searchRes, occ );
		if ( searchRes != -1 ) {
			if ( wholeWord && !String::isWholeWord( fileText, text, searchRes ) ) {
				totNl += countNewLines( fileText, lSearchRes, searchRes );
				lSearchRes = searchRes;
				searchRes += text.size();
				continue;
			}
			Int64 relCol;
			totNl += countNewLines( fileText, lSearchRes, searchRes );
			String str(
				textLine( caseSensitive ? fileText : fileTextOriginal, searchRes, relCol ) );
			res.push_back( { str,
							 { { (Int64)totNl, (Int64)relCol },
							   { (Int64)totNl, (Int64)( relCol + String::utf8Length( text ) ) } },
							 searchRes,
							 static_cast<Int64>( searchRes + text.size() ) } );
			lSearchRes = searchRes;
			searchRes += text.size();
		}
	} while ( searchRes != -1 );

	return res;
}

static std::vector<ProjectSearch::ResultData::Result>
searchInFileLuaPattern( const std::string& file, const std::string& text, const bool& caseSensitive,
						const bool& wholeWord ) {
	std::string fileText;
	FileSystem::fileGet( file, fileText );
	LuaPattern pattern( text );
	std::vector<ProjectSearch::ResultData::Result> results;
	Int64 totNl = 0;
	bool matched = false;
	Int64 searchRes = 0;
	std::string fileTextOriginal;

	if ( !caseSensitive ) {
		fileTextOriginal = fileText;
		String::toLowerInPlace( fileText );
	}

	LuaPattern::Range matches[12];
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

void ProjectSearch::find( const std::vector<std::string> files, const std::string& string,
						  ResultCb result, bool caseSensitive, bool wholeWord,
						  const TextDocument::FindReplaceType& type,
						  const std::vector<GlobMatch>& pathFilters, std::string basePath,
						  std::vector<std::shared_ptr<TextDocument>> openDocs ) {
	Result res;
	const auto occ =
		type == TextDocument::FindReplaceType::Normal
			? String::BMH::createOccTable( (const unsigned char*)string.c_str(), string.size() )
			: std::vector<size_t>();
	for ( auto& file : files ) {
		bool skip = false;
		std::string_view fsv( file );
		if ( !basePath.empty() && String::startsWith( file, basePath ) )
			fsv = fsv.substr( basePath.size() );

		for ( const auto& filter : pathFilters ) {
			bool matches = String::globMatch( fsv, filter.first );
			if ( ( matches && filter.second ) || ( !matches && !filter.second ) ) {
				skip = true;
				break;
			}
		}
		if ( skip )
			continue;

		auto fileRes = type == TextDocument::FindReplaceType::Normal
						   ? searchInFileHorspool( file, string, caseSensitive, wholeWord, occ )
						   : searchInFileLuaPattern( file, string, caseSensitive, wholeWord );
		if ( !fileRes.empty() )
			res.push_back( { file, fileRes } );
	}
	result( res );
}

struct FindData {
	Mutex resMutex;
	Mutex countMutex;
	int resCount{ 0 };
	ProjectSearch::Result res;
};

void ProjectSearch::find( const std::vector<std::string> files, std::string string,
						  std::shared_ptr<ThreadPool> pool, ResultCb result, bool caseSensitive,
						  bool wholeWord, const TextDocument::FindReplaceType& type,
						  const std::vector<GlobMatch>& pathFilters, std::string basePath,
						  std::vector<std::shared_ptr<TextDocument>> openDocs ) {
	if ( files.empty() )
		result( {} );
	FileSystem::dirAddSlashAtEnd( basePath );
	pool->run( [files = std::move( files ), string = std::move( string ), pool = std::move( pool ),
				result = std::move( result ), caseSensitive, wholeWord, type,
				pathFilters = std::move( pathFilters ), basePath = std::move( basePath ),
				openDocs = std::move( openDocs )]() mutable {
		FindData* findData = eeNew( FindData, () );
		findData->resCount = files.size();
		if ( !caseSensitive )
			String::toLowerInPlace( string );
		const auto occ =
			type == TextDocument::FindReplaceType::Normal
				? String::BMH::createOccTable( (const unsigned char*)string.c_str(), string.size() )
				: std::vector<size_t>();
		std::vector<bool> search;
		search.resize( files.size() );
		size_t pos = 0;
		size_t count = 0;
		for ( const auto& file : files ) {
			bool skip = false;
			std::string_view fsv( file );
			if ( !basePath.empty() && String::startsWith( file, basePath ) )
				fsv = fsv.substr( basePath.size() );

			for ( const auto& filter : pathFilters ) {
				bool matches = String::globMatch( fsv, filter.first );
				if ( ( matches && filter.second ) || ( !matches && !filter.second ) ) {
					skip = true;
					break;
				}
			}
			if ( skip ) {
				search[pos++] = false;
				continue;
			}
			search[pos++] = true;
			count++;
		}

		findData->resCount = count;

		if ( count == 0 ) {
			result( findData->res );
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

			const auto onSearchEnd = [result, findData]( const auto& ) {
				int count;
				{
					Lock l( findData->countMutex );
					findData->resCount--;
					count = findData->resCount;
				}
				if ( count == 0 ) {
					result( findData->res );
					eeDelete( findData );
#if EE_PLATFORM == EE_PLATFORM_LINUX
					malloc_trim( 0 );
#endif
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
						auto res = doc->findAll( string, caseSensitive, wholeWord, type );
						std::vector<ProjectSearch::ResultData::Result> fileRes;
						for ( const auto& r : res ) {
							ProjectSearch::ResultData::Result f;
							f.openDoc = doc;
							f.position = r.result;
							const auto& line = doc->line( r.result.start().line() );
							if ( line.size() > EE_1KB )
								f.line = line.getText().substr( 0, EE_1KB );
							else
								f.line = line.getTextWithoutNewLine();
							f.start = r.result.start().column();
							f.end = r.result.end().column();
							std::vector<std::string> captures;
							for ( const auto& capture : r.captures )
								captures.emplace_back( doc->getText( capture ).toUtf8() );
							f.captures = std::move( captures );
							fileRes.emplace_back( std::move( f ) );
						}

						if ( !fileRes.empty() ) {
							Lock l( findData->resMutex );
							std::string file( doc->getFilePath() );
							findData->res.push_back( { std::move( file ), std::move( fileRes ) } );
						}
					},
					onSearchEnd );
			} else {
				pool->run(
					[findData, file, string, caseSensitive, wholeWord, occ, type] {
						auto fileRes =
							type == TextDocument::FindReplaceType::Normal
								? searchInFileHorspool( file, string, caseSensitive, wholeWord,
														occ )
								: searchInFileLuaPattern( file, string, caseSensitive, wholeWord );
						if ( !fileRes.empty() ) {
							Lock l( findData->resMutex );
							findData->res.push_back( { std::move( file ), std::move( fileRes ) } );
						}
					},
					onSearchEnd );
			}
		}
	} );
}

void ProjectSearch::ResultModel::removeLastNewLineCharacter() {
	for ( auto& r : mResult ) {
		for ( auto& r2 : r.results )
			if ( !r2.line.empty() && r2.line.back() == '\n' )
				r2.line.pop_back();
	}
}

} // namespace ecode
