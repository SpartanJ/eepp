#ifndef ECODE_PROJECTSEARCH_HPP
#define ECODE_PROJECTSEARCH_HPP

#include <eepp/core/string.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/models/model.hpp>
#include <functional>
#include <memory>
#include <string>

using namespace EE;
using namespace EE::System;
using namespace EE::UI::Doc;
using namespace EE::UI::Models;

namespace ecode {

using GlobMatch = std::pair<std::string, bool>; // where string is the glob and bool true
												// indicates that it's inverted / negated

class ProjectSearch {
  public:
	struct SearchConfig {
		std::string searchString;
		bool caseSensitive;
		bool wholeWord;
		TextDocument::FindReplaceType type;
	};

	struct ResultData {
		struct Result {
			Result( const String& line, const TextRange& pos, Int64 s, Int64 e ) :
				line( line ), position( pos ), start( s ), end( e ) {}
			Result() {}
			String line;
			TextRange position;
			Int64 start{ 0 };
			Int64 end{ 0 };
			bool selected{ true };
			std::vector<std::string> captures;
		};
		std::string file;
		std::vector<Result> results;
		std::shared_ptr<TextDocument> openDoc{ nullptr };
		bool selected{ true };

		void setResultsSelected( bool selected ) {
			this->selected = selected;
			for ( auto& res : results )
				res.selected = selected;
		}

		bool allResultsSelected() {
			for ( const auto& res : results ) {
				if ( !res.selected )
					return false;
			}
			return true;
		}

		bool noneResultsSelected() {
			for ( const auto& res : results ) {
				if ( res.selected )
					return false;
			}
			return true;
		}
	};

	using Result = std::vector<ResultData>;
	using ConsolidatedResult = std::pair<SearchConfig, Result>;
	using ResultCb = std::function<void( const ConsolidatedResult& )>;

	class ResultModel : public Model {
	  public:
		enum Column {
			FileOrPosition,
			Line,
			LineEnd,
			ColumnStart,
			ColumnEnd,
			Selected,
			Data,
			ChildCount
		};

		ResultModel( const Result& result ) : mResult( result ) {}

		virtual size_t treeColumn() const { return Column::FileOrPosition; }

		ModelIndex parentIndex( const ModelIndex& index ) const {
			if ( !index.isValid() || index.internalId() == -1 )
				return {};
			return createIndex( index.internalId(), index.column(), &mResult[index.internalId()],
								-1 );
		}

		ModelIndex index( int row, int column, const ModelIndex& parent ) const {
			if ( row < 0 || column < 0 )
				return {};
			if ( !parent.isValid() )
				return createIndex( row, column, &mResult[row], -1 );
			if ( parent.internalData() )
				return createIndex( row, column, &mResult[parent.row()].results[row],
									parent.row() );
			return {};
		}

		size_t rowCount( const ModelIndex& index ) const {
			if ( !index.isValid() )
				return mResult.size();
			if ( index.internalId() == -1 )
				return mResult[index.row()].results.size();
			return 0;
		}

		size_t columnCount( const ModelIndex& ) const { return 2; }

		std::string columnName( const size_t& colIndex ) const {
			return colIndex == 0 ? "File" : "Line";
		}

		size_t resultCount() {
			size_t count = 0;
			for ( const auto& res : mResult )
				count += res.results.size();
			return count;
		}

		Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
			static const char* EMPTY = "";
			if ( role == ModelRole::Display ) {
				if ( index.internalId() == -1 ) {
					if ( index.column() == FileOrPosition ) {
						return Variant( String::format( "%s (%zu)",
														mResult[index.row()].file.c_str(),
														mResult[index.row()].results.size() ) );
					}
				} else {
					switch ( index.column() ) {
						case FileOrPosition:
							return Variant(
								String( String::format( "%6lld      ", mResult[index.internalId()]
																			   .results[index.row()]
																			   .position.start()
																			   .line() +
																		   1 ) ) +
								mResult[index.internalId()].results[index.row()].line );
					}
				}
			} else if ( role == ModelRole::Custom ) {
				if ( index.internalId() != -1 ) {
					switch ( index.column() ) {
						case FileOrPosition:
							return Variant( mResult[index.internalId()]
												.results[index.row()]
												.position.start()
												.line() );
						case Line:
							return Variant(
								mResult[index.internalId()].results[index.row()].line.c_str() );
						case ColumnStart:
							return Variant( mResult[index.internalId()]
												.results[index.row()]
												.position.start()
												.column() );
						case ColumnEnd:
							return Variant( mResult[index.internalId()]
												.results[index.row()]
												.position.end()
												.column() );
						case Selected:
							return Variant(
								mResult[index.internalId()].results[index.row()].selected );
						case Data:
							return Variant(
								(void*)&mResult[index.internalId()].results[index.row()] );
						case ChildCount:
							return Variant( (Int64)0 );
					}
				} else {
					switch ( index.column() ) {
						case FileOrPosition:
							return Variant( mResult[index.row()].file.c_str() );
						case Data:
							return Variant( (void*)&mResult[index.row()] );
						case ChildCount:
							return Variant( (Int64)mResult[index.row()].results.size() );
						case Selected:
							return Variant( (bool)mResult[index.row()].selected );
					}
				}
			}
			return Variant( EMPTY );
		}

		const Result& getResult() const { return mResult; }

		void removeLastNewLineCharacter();

		void setResultFromSymbolReference( bool ref ) { mResultFromSymbolReference = ref; }

		bool isResultFromSymbolReference() const { return mResultFromSymbolReference; }

		void setOpType( TextDocument::FindReplaceType opType ) { mOpType = opType; }

		bool isResultFromPatternMatch() const {
			return mOpType != TextDocument::FindReplaceType::Normal;
		}

	  protected:
		Result mResult;
		TextDocument::FindReplaceType mOpType{ TextDocument::FindReplaceType::Normal };
		bool mResultFromSymbolReference{ false };
		bool mResultFromLuaPattern{ false };
		bool mResultFromRegEx{ false };
	};

	static std::shared_ptr<ResultModel> asModel( const Result& result ) {
		return std::make_shared<ResultModel>( result );
	}

	static std::vector<ProjectSearch::ResultData::Result>
	fileResFromDoc( const std::string& string, bool caseSensitive, bool wholeWord,
					TextDocument::FindReplaceType type, std::shared_ptr<TextDocument> doc );

	static void
	find( const std::vector<std::string> files, std::string string,
		  std::shared_ptr<ThreadPool> pool, ResultCb result, bool caseSensitive,
		  bool wholeWord = false,
		  const TextDocument::FindReplaceType& type = TextDocument::FindReplaceType::Normal,
		  const std::vector<GlobMatch>& pathFilters = {}, std::string basePath = "",
		  std::vector<std::shared_ptr<TextDocument>> openDocs = {} );
};

} // namespace ecode

#endif // ECODE_PROJECTSEARCH_HPP
