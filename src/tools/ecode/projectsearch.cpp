#include "projectsearch.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/luapattern.hpp>

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

static String textLine( const std::string& fileText, const size_t& fromPos, size_t& relCol ) {
	size_t start = 0;
	size_t end = 0;
	const char* stringStartPtr = fileText.c_str();
	const char* startPtr = fileText.c_str() + fromPos;
	const char* ptr = startPtr;
	const char* nlStartPtr = stringStartPtr;
	if ( stringStartPtr != ptr ) {
		while ( stringStartPtr != ptr && *--ptr != '\n' ) {
		}
		nlStartPtr = ptr + 1;
		start = ptr - stringStartPtr + 1;
	}
	ptr = startPtr;
	while ( ++ptr && *ptr != '\0' && *ptr != '\n' ) {
	}
	end = ptr - stringStartPtr;
	relCol = String( fileText.substr( start, startPtr - nlStartPtr ) ).size();
	// if the line to substract is massive we only get the fist kilobyte of that line, since the
	// line is only shared for visual aid.
	return fileText.substr( start, end - start > EE_1KB ? EE_1KB : end - start );
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
				lSearchRes = searchRes;
				searchRes += text.size();
				continue;
			}
			size_t relCol;
			totNl += countNewLines( fileText, lSearchRes, searchRes );
			String str(
				textLine( caseSensitive ? fileText : fileTextOriginal, searchRes, relCol ) );
			res.push_back( { str,
							 { { (Int64)totNl, (Int64)relCol },
							   { (Int64)totNl, (Int64)( relCol + text.size() ) } },
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
	std::vector<ProjectSearch::ResultData::Result> res;
	size_t totNl = 0;
	bool matched = false;
	Int64 searchRes = 0;
	std::string fileTextOriginal;

	if ( !caseSensitive ) {
		fileTextOriginal = fileText;
		String::toLowerInPlace( fileText );
	}

	do {
		int start, end = 0;
		if ( ( matched = pattern.find( fileText, start, end, searchRes ) ) ) {
			if ( wholeWord &&
				 !String::isWholeWord( fileText, fileText.substr( start, end - start ), start ) ) {
				searchRes = end;
				continue;
			}
			size_t relCol;
			totNl += countNewLines( fileText, searchRes, start );
			String str( textLine( caseSensitive ? fileText : fileTextOriginal, start, relCol ) );
			int len = end - start;
			res.push_back(
				{ str,
				  { { (Int64)totNl, (Int64)relCol }, { (Int64)totNl, (Int64)( relCol + len ) } },
				  start,
				  end } );
			searchRes = end;
		}
	} while ( matched );

	return res;
}

void ProjectSearch::find( const std::vector<std::string> files, const std::string& string,
						  ResultCb result, bool caseSensitive, bool wholeWord,
						  const TextDocument::FindReplaceType& type ) {
	Result res;
	const auto occ =
		type == TextDocument::FindReplaceType::Normal
			? String::BMH::createOccTable( (const unsigned char*)string.c_str(), string.size() )
			: std::vector<size_t>();
	for ( auto& file : files ) {
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
						  bool wholeWord, const TextDocument::FindReplaceType& type ) {
	if ( files.empty() )
		result( {} );
	FindData* findData = eeNew( FindData, () );
	findData->resCount = files.size();
	if ( !caseSensitive )
		String::toLowerInPlace( string );
	const auto occ =
		type == TextDocument::FindReplaceType::Normal
			? String::BMH::createOccTable( (const unsigned char*)string.c_str(), string.size() )
			: std::vector<size_t>();
	for ( auto& file : files ) {
		pool->run(
			[findData, file, string, caseSensitive, wholeWord, occ, type] {
				auto fileRes =
					type == TextDocument::FindReplaceType::Normal
						? searchInFileHorspool( file, string, caseSensitive, wholeWord, occ )
						: searchInFileLuaPattern( file, string, caseSensitive, wholeWord );
				if ( !fileRes.empty() ) {
					Lock l( findData->resMutex );
					findData->res.push_back( { file, fileRes } );
				}
			},
			[result, findData] {
				int count;
				{
					Lock l( findData->countMutex );
					findData->resCount--;
					count = findData->resCount;
				}
				if ( count == 0 ) {
					result( findData->res );
					eeDelete( findData );
				}
			} );
	}
}

}
