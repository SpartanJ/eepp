#ifndef EE_UI_MODELS_MODEL_HPP
#define EE_UI_MODELS_MODEL_HPP

#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/ui/models/modelindex.hpp>
#include <eepp/ui/models/modelrole.hpp>
#include <eepp/ui/models/variant.hpp>
#include <functional>
#include <memory>
#include <stack>
#include <string>
#include <unordered_set>

using namespace EE::Graphics;
using namespace EE::Math;

namespace EE { namespace UI { namespace Abstract {
class UIAbstractView;
}}} // namespace EE::UI::Abstract

using namespace EE::UI::Abstract;

namespace EE { namespace UI { namespace Models {

class PersistentHandle;

enum class SortOrder { None, Ascending, Descending };

class EE_API Model {
  public:
	using ModelStyler = std::function<Variant( const ModelIndex&, const void* data )>;

	class EE_API Client {
	  public:
		virtual ~Client() {}

		virtual void onModelUpdated( unsigned flags ) = 0;

		virtual void modelDidInsertRows( [[maybe_unused]] ModelIndex const& parent,
										 [[maybe_unused]] int first, [[maybe_unused]] int last ) {}
		virtual void modelDidInsertColumns( [[maybe_unused]] ModelIndex const& parent,
											[[maybe_unused]] int first,
											[[maybe_unused]] int last ) {}
		virtual void modelDidMoveRows( [[maybe_unused]] ModelIndex const& source_parent,
									   [[maybe_unused]] int first, [[maybe_unused]] int last,
									   [[maybe_unused]] ModelIndex const& target_parent,
									   [[maybe_unused]] int target_index ) {}
		virtual void modeldidMoveColumns( [[maybe_unused]] ModelIndex const& source_parent,
										  [[maybe_unused]] int first, [[maybe_unused]] int last,
										  [[maybe_unused]] ModelIndex const& target_parent,
										  [[maybe_unused]] int target_index ) {}
		virtual void modelDidDeleteRows( [[maybe_unused]] ModelIndex const& parent,
										 [[maybe_unused]] int first, [[maybe_unused]] int last ) {}
		virtual void modelDidDeleteColumns( [[maybe_unused]] ModelIndex const& parent,
											[[maybe_unused]] int first,
											[[maybe_unused]] int last ) {}
	};

	enum UpdateFlag {
		DontInvalidateIndexes = 0,
		InvalidateAllIndexes = 1 << 0,
	};

	virtual ~Model(){};

	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const = 0;

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const = 0;

	virtual std::string columnName( const size_t& /*column*/ ) const { return {}; }

	virtual std::string rowName( const size_t& /*row*/ ) const { return {}; }

	virtual Variant data( const ModelIndex&, ModelRole = ModelRole::Display ) const = 0;

	virtual void update() { onModelUpdate(); }

	virtual ModelIndex parentIndex( const ModelIndex& ) const { return {}; }

	virtual ModelIndex index( int row, int column = 0, const ModelIndex& = ModelIndex() ) const {
		return createIndex( row, column );
	}

	virtual ModelIndex sibling( int row, int column, const ModelIndex& parent ) const;

	virtual void setData( const ModelIndex&, const Variant& ) {}

	virtual size_t treeColumn() const { return 0; }

	virtual bool acceptsDrag( const ModelIndex&, const std::string& dataType );

	virtual bool isColumnSortable( const size_t& /*columnIndex*/ ) const { return true; }

	virtual std::string dragDataType() const { return {}; }

	virtual bool isEditable( const ModelIndex& ) const { return false; }

	bool isValid( const ModelIndex& index ) const {
		auto parentIndex = this->parentIndex( index );
		return index.row() >= 0 && index.row() < (Int64)rowCount( parentIndex ) &&
			   index.column() >= 0 && index.column() < (Int64)columnCount( parentIndex );
	}

	virtual int keyColumn() const { return -1; }

	virtual SortOrder sortOrder() const { return SortOrder::None; }

	virtual bool isSortable() { return false; }

	virtual void sort( const size_t& /*column*/, const SortOrder& /*order*/ ) {}

	virtual bool classModelRoleEnabled() { return false; }

	void registerView( UIAbstractView* );

	void unregisterView( UIAbstractView* );

	void registerClient( Client* );

	void unregisterClient( Client* );

	void refreshView();

	void setOnUpdate( const std::function<void()>& onUpdate );

	void invalidate( unsigned int flags = Model::UpdateFlag::InvalidateAllIndexes );

	std::weak_ptr<PersistentHandle> registerPersistentIndex( ModelIndex const& );

	void beginInsertRows( ModelIndex const& parent, int first, int last );
	void beginInsertColumns( ModelIndex const& parent, int first, int last );
	void beginMoveRows( ModelIndex const& sourceParent, int first, int last,
						ModelIndex const& targetParent, int target_index );
	void beginMoveColumns( ModelIndex const& sourceParent, int first, int last,
						   ModelIndex const& targetParent, int target_index );
	bool beginDeleteRows( ModelIndex const& parent, int first, int last );
	bool beginDeleteColumns( ModelIndex const& parent, int first, int last );

	void endInsertRows();
	void endInsertColumns();
	void endMoveRows();
	void endMoveColumns();
	void endDeleteRows();
	void endDeleteColumns();

	Mutex& resourceMutex();

	void acquireResourceMutex();

	void releaseResourceMutex();

	Uint32 subsribeModelStyler( const ModelStyler& styler );

	void unsubsribeModelStyler( Uint32 id );

  protected:
	Model(){};

	void forEachView( std::function<void( UIAbstractView* )> );

	void onModelUpdate( unsigned flags = UpdateFlag::InvalidateAllIndexes );

	ModelIndex createIndex( int row, int column, const void* data = nullptr,
							const Int64& internalId = 0 ) const;

	enum class OperationType { Invalid = 0, Insert, Move, Delete, Reset };
	enum class Direction { Row, Column };

	struct Operation {
		OperationType type{ OperationType::Invalid };
		Direction direction{ Direction::Row };
		ModelIndex sourceParent;
		int first{ 0 };
		int last{ 0 };
		ModelIndex targetParent;
		int target{ 0 };

		Operation( OperationType type ) : type( type ) {}

		Operation( OperationType type, Direction direction, ModelIndex const& parent, int first,
				   int last ) :
			type( type ),
			direction( direction ),
			sourceParent( parent ),
			first( first ),
			last( last ) {}

		Operation( OperationType type, Direction direction, ModelIndex const& sourceParent,
				   int first, int last, ModelIndex const& targetParent, int target ) :
			type( type ),
			direction( direction ),
			sourceParent( sourceParent ),
			first( first ),
			last( last ),
			targetParent( targetParent ),
			target( target ) {}
	};

	void handleInsert( Operation const& );
	void handleMove( Operation const& );
	void handleDelete( Operation const& );

	template <bool IsRow> void saveDeletedIndices( ModelIndex const& parent, int first, int last );

	Variant stylizeModel( const ModelIndex& index, const void* data = nullptr ) const;

	UnorderedMap<ModelIndex, std::shared_ptr<PersistentHandle>> mPersistentHandles;
	std::stack<Operation> mOperationStack;
	// NOTE: We need to save which indices have been deleted before the delete
	// actually happens, because we can't figure out which persistent handles
	// belong to us in end_delete_rows/columns (because accessing the parents of
	// the indices might be impossible).
	std::stack<std::vector<ModelIndex>> mDeletedIndicesStack;
	std::unordered_set<UIAbstractView*> mViews;
	std::unordered_set<Client*> mClients;
	std::function<void()> mOnUpdate;
	Mutex mResourceLock;
	Uint32 mLastStylerId{ 0 };
	std::unordered_map<Uint32, ModelStyler> mStylers;
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_MODEL_HPP
