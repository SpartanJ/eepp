#include "projectsearch.hpp"

static std::vector<ProjectSearch::ResultData::Result>
searchInFile( const std::string& file, const std::string& text, const bool& caseSensitive ) {
	std::vector<ProjectSearch::ResultData::Result> res;
	TextDocument doc( false );
	TextPosition pos{0, 0};
	String searchText( text );
	if ( doc.loadFromFile( file ) ) {
		do {
			pos = doc.find( searchText, pos, caseSensitive );
			if ( pos.isValid() ) {
				const auto& l = doc.line( pos.line() ).getText();
				res.push_back( {l.substr( 0, l.size() - 1 ).toUtf8(), pos} );
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
			res.push_back( {file, fileRes} );
	}
	result( res );
}

struct FindData {
	Mutex resMutex;
	Mutex countMutex;
	int resCount{0};
	ProjectSearch::Result res;
};

void ProjectSearch::find( const std::vector<std::string> files, const std::string& string,
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
					findData->res.push_back( {file, fileRes} );
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
