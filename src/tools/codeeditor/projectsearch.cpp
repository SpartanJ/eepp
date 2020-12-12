#include "projectsearch.hpp"
#include <eepp/system/filesystem.hpp>

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

static std::string textLine( const std::string& fileText, const size_t& fromPos, size_t& relCol ) {
	size_t start = 0;
	size_t end = 0;
	const char* stringStartPtr = fileText.c_str();
	const char* startPtr = fileText.c_str() + fromPos;
	const char* ptr = startPtr;
	while ( stringStartPtr != ptr && *--ptr != '\n' ) {
	}
	const char* nlStartPtr = ptr + 1;
	start = ptr - stringStartPtr + 1;
	ptr = startPtr;
	while ( ++ptr && *ptr != '\0' && *ptr != '\n' ) {
	}
	end = ptr - stringStartPtr;
	relCol = startPtr - nlStartPtr;
	return fileText.substr( start, end - start );
}

static std::vector<ProjectSearch::ResultData::Result>
searchInFileHorspool( const std::string& file, const std::string& text, const bool& caseSensitive,
					  const String::BMH::OccTable& occ ) {
	std::vector<ProjectSearch::ResultData::Result> res;
	std::string fileText;
	Int64 lSearchRes = 0;
	Int64 searchRes = 0;
	size_t totNl = 0;
	FileSystem::fileGet( file, fileText );
	if ( !caseSensitive ) {
		std::string fileTextOriginal( fileText );
		String::toLowerInPlace( fileText );
		do {
			searchRes = String::BMH::find( fileText, text, searchRes, occ );
			if ( searchRes != -1 ) {
				TextPosition pos;
				size_t relCol;
				totNl += countNewLines( fileText, lSearchRes, searchRes );
				std::string str = textLine( fileTextOriginal, searchRes, relCol );
				pos.setLine( totNl );
				pos.setColumn( relCol );
				res.push_back( { str, pos } );
				lSearchRes = searchRes;
				searchRes += text.size();
			}
		} while ( searchRes != -1 );
	} else {
		do {
			searchRes = String::BMH::find( fileText, text, searchRes, occ );
			if ( searchRes != -1 ) {
				TextPosition pos;
				size_t relCol;
				totNl += countNewLines( fileText, lSearchRes, searchRes );
				std::string str = textLine( fileText, searchRes, relCol );
				pos.setLine( totNl );
				pos.setColumn( relCol );
				res.push_back( { str, pos } );
				lSearchRes = searchRes;
				searchRes += text.size();
			}
		} while ( searchRes != -1 );
	}
	return res;
}

static std::vector<ProjectSearch::ResultData::Result>
searchInFile( const std::string& file, const std::string& text, const bool& caseSensitive ) {
	std::vector<ProjectSearch::ResultData::Result> res;
	TextDocument doc( false );
	TextPosition pos{ 0, 0 };
	String searchText( text );
	if ( doc.loadFromFile( file ) ) {
		do {
			pos = doc.find( searchText, pos, caseSensitive );
			if ( pos.isValid() ) {
				const auto& l = doc.line( pos.line() ).getText();
				res.push_back( { l.substr( 0, l.size() - 1 ).toUtf8(), pos } );
				pos = doc.positionOffset( pos, searchText.size() );
			}
		} while ( pos.isValid() );
	}
	return res;
}

void ProjectSearch::find( const std::vector<std::string> files, const std::string& string,
						  ResultCb result, bool caseSensitive ) {
	Result res;
	for ( auto& file : files ) {
		auto fileRes = searchInFile( file, string, caseSensitive );
		if ( !fileRes.empty() )
			res.push_back( { file, fileRes } );
	}
	result( res );
}

void ProjectSearch::findHorspool( const std::vector<std::string> files, const std::string& string,
								  ResultCb result, bool caseSensitive ) {
	Result res;
	const auto occ =
		String::BMH::createOccTable( (const unsigned char*)string.c_str(), string.size() );
	for ( auto& file : files ) {
		auto fileRes = searchInFileHorspool( file, string, caseSensitive, occ );
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
						  std::shared_ptr<ThreadPool> pool, ResultCb result, bool caseSensitive ) {
	if ( files.empty() )
		result( {} );
	FindData* findData = eeNew( FindData, () );
	findData->resCount = files.size();
	for ( auto& file : files ) {
		pool->run(
			[findData, file, string, caseSensitive] {
				auto fileRes = searchInFile( file, string, caseSensitive );
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

void ProjectSearch::findHorspool( const std::vector<std::string> files, std::string string,
								  std::shared_ptr<ThreadPool> pool, ResultCb result,
								  bool caseSensitive ) {
	if ( files.empty() )
		result( {} );
	FindData* findData = eeNew( FindData, () );
	findData->resCount = files.size();
	if ( !caseSensitive )
		String::toLowerInPlace( string );
	const auto occ =
		String::BMH::createOccTable( (const unsigned char*)string.c_str(), string.size() );
	for ( auto& file : files ) {
		pool->run(
			[findData, file, string, caseSensitive, occ] {
				auto fileRes = searchInFileHorspool( file, string, caseSensitive, occ );
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
