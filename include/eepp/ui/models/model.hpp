#ifndef EE_UI_MODEL_MODEL_HPP
#define EE_UI_MODEL_MODEL_HPP

#include <eepp/ui/models/modelindex.hpp>
#include <eepp/ui/models/variant.hpp>
#include <functional>
#include <string>
#include <unordered_set>

using namespace EE::Graphics;
using namespace EE::Math;

namespace EE { namespace UI { namespace Abstract {
class UIAbstractView;
}}} // namespace EE::UI::Abstract

using namespace EE::UI::Abstract;

namespace EE { namespace UI { namespace Models {

enum class SortOrder { None, Ascending, Descending };

class EE_API Model {
  public:
	enum UpdateFlag {
		DontInvalidateIndexes = 0,
		InvalidateAllIndexes = 1 << 0,
	};

	enum class Role {
		Display,
		Icon,
		Sort,
		Custom
	};

	virtual ~Model(){};

	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const = 0;

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const = 0;

	virtual std::string columnName( const size_t& /*column*/ ) const { return {}; }

	virtual Variant data( const ModelIndex&, Role = Role::Display ) const = 0;

	virtual void update() = 0;

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

	bool isValid( const ModelIndex& index ) const {
		auto parentIndex = this->parentIndex( index );
		return index.row() >= 0 && index.row() < (Int64)rowCount( parentIndex ) &&
			   index.column() >= 0 && index.column() < (Int64)columnCount( parentIndex );
	}

	virtual int keyColumn() const { return -1; }

	virtual SortOrder sortOrder() const { return SortOrder::None; }

	virtual void setKeyColumnAndSortOrder( const size_t& /*column*/, const SortOrder& /*order*/ ) {}

	void registerView( UIAbstractView* );

	void unregisterView( UIAbstractView* );

	void setOnUpdate( const std::function<void()>& onUpdate );

  protected:
	Model(){};

	void forEachView( std::function<void( UIAbstractView* )> );

	void onModelUpdate( unsigned flags = UpdateFlag::InvalidateAllIndexes );

	ModelIndex createIndex( int row, int column, const void* data = nullptr ) const;

  private:
	std::unordered_set<UIAbstractView*> mViews;
	std::function<void()> mOnUpdate;
};

inline ModelIndex ModelIndex::parent() const {
	return mModel ? mModel->parentIndex( *this ) : ModelIndex();
}

}}} // namespace EE::UI::Model

#endif // EE_UI_MODEL_MODEL_HPP
