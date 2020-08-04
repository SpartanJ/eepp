#ifndef PROJECTSEARCH_HPP
#define PROJECTSEARCH_HPP

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

class ProjectSearch {
  public:
	struct ResultData {
		struct Result {
			Result( const String& line, const TextPosition& pos ) : line( line ), position( pos ) {}
			std::string line;
			TextPosition position;
		};
		std::string file;
		std::vector<Result> results;
	};

	typedef std::vector<ResultData> Result;
	typedef std::function<void( const Result& )> ResultCb;

	class ResultModel : public Model {
	  public:
		enum Column { FileOrPosition, Line, ColumnPosition };

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
			if ( parent.data() )
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

		Variant data( const ModelIndex& index, Role role = Role::Display ) const {
			static const char* EMPTY = "";
			if ( role == Role::Display ) {
				if ( index.internalId() == -1 ) {
					if ( index.column() == FileOrPosition ) {
						return Variant( String::format( "%s (%zu)",
														mResult[index.row()].file.c_str(),
														mResult[index.row()].results.size() ) );
					}
				} else {
					switch ( index.column() ) {
						case FileOrPosition:
							return Variant( String::format(
								"%6lld      %s",
								mResult[index.internalId()].results[index.row()].position.line(),
								mResult[index.internalId()].results[index.row()].line.c_str() ) );
					}
				}
			} else if ( role == Role::Custom ) {
				if ( index.internalId() != -1 ) {
					switch ( index.column() ) {
						case FileOrPosition:
							return Variant(
								mResult[index.internalId()].results[index.row()].position.line() );
						case Line:
							return Variant(
								mResult[index.internalId()].results[index.row()].line.c_str() );
						case ColumnPosition:
							return Variant( mResult[index.internalId()]
												.results[index.row()]
												.position.column() );
					}
				}
			}
			return Variant( EMPTY );
		}

		virtual void update() { onModelUpdate(); }

	  protected:
		Result mResult;
	};

	static std::shared_ptr<ResultModel> asModel( const Result& result ) {
		return std::make_shared<ResultModel>( result );
	}

	static void find( const std::vector<std::string> files, const std::string& string,
					  ResultCb result, bool caseSensitive );

	static void find( const std::vector<std::string> files, const std::string& string,
					  std::shared_ptr<ThreadPool> pool, ResultCb result, bool caseSensitive );
};

#endif // PROJECTSEARCH_HPP
