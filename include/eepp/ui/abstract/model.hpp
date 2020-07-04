#ifndef EE_UI_MODEL_MODEL_HPP
#define EE_UI_MODEL_MODEL_HPP

#include <eepp/ui/abstract/modelindex.hpp>
#include <functional>
#include <string>
#include <unordered_set>

namespace EE { namespace UI {
class UIAbstractView;
}} // namespace EE::UI

namespace EE { namespace UI { namespace Abstract {

enum class SortOrder { None, Ascending, Descending };

class Variant;

class EE_API Model {
  public:
	enum UpdateFlag {
		DontInvalidateIndexes = 0,
		InvalidateAllIndexes = 1 << 0,
	};

	enum class Role {
		Display,
		Sort,
		Custom,
		ForegroundColor,
		BackgroundColor,
		Icon,
		Font,
		DragData,
		TextAlignment,
	};

	virtual ~Model(){};

	virtual int rowCount( const ModelIndex& = ModelIndex() ) const = 0;
	virtual int columnCount( const ModelIndex& = ModelIndex() ) const = 0;
	virtual std::string columnName( int ) const { return {}; }
	virtual Variant data( const ModelIndex&, Role = Role::Display ) const = 0;
	virtual void update() = 0;
	virtual ModelIndex parentIndex( const ModelIndex& ) const { return {}; }
	virtual ModelIndex index( int row, int column = 0, const ModelIndex& = ModelIndex() ) const {
		return createIndex( row, column );
	}
	virtual ModelIndex sibling( int row, int column, const ModelIndex& parent ) const;
	virtual void setData( const ModelIndex&, const Variant& ) {}
	virtual int treeColumn() const { return 0; }
	virtual bool acceptsDrag( const ModelIndex&, const std::string& dataType );

	virtual bool isColumnSortable( int /*columnIndex*/ ) const { return true; }

	virtual std::string dragDataType() const { return {}; }

	bool isValid( const ModelIndex& index ) const {
		auto parentIndex = this->parentIndex( index );
		return index.row() >= 0 && index.row() < rowCount( parentIndex ) && index.column() >= 0 &&
			   index.column() < columnCount( parentIndex );
	}

	virtual int keyColumn() const { return -1; }
	virtual SortOrder sortOrder() const { return SortOrder::None; }
	virtual void setKeyColumnAndSortOrder( int, SortOrder ) {}

	void registerView( UIAbstractView* );
	void unregisterView( UIAbstractView* );

	std::function<void()> onUpdate;

  protected:
	Model(){};

	void forEachView( std::function<void( UIAbstractView* )> );
	void didUpdate( unsigned flags = UpdateFlag::InvalidateAllIndexes );

	ModelIndex createIndex( int row, int column, const void* data = nullptr ) const;

  private:
	std::unordered_set<UIAbstractView*> mViews;
};

inline ModelIndex ModelIndex::parent() const {
	return mModel ? mModel->parentIndex( *this ) : ModelIndex();
}

}}} // namespace EE::UI::Abstract

#endif // EE_UI_MODEL_MODEL_HPP
