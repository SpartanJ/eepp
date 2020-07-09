#ifndef EE_UI_UITREEVIEW_HPP
#define EE_UI_UITREEVIEW_HPP

#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <memory>
#include <unordered_map>

using namespace EE::UI::Abstract;

namespace EE { namespace UI {

class EE_API UITreeView : public UIAbstractTableView {
  public:
	static UITreeView* New();

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	const Float& getIndentWidth() const;

	void setIndentWidth( const Float& indentWidth );

	virtual Sizef getContentSize() const;

	virtual void drawChilds();

	virtual Node* overFind( const Vector2f& point );

	bool isExpanded( const ModelIndex& index ) const;

	Drawable* getExpandIcon() const;

	void setExpandedIcon( Drawable* expandIcon );

	Drawable* getContractIcon() const;

	void setContractedIcon( Drawable* contractIcon );

  protected:
	enum class IterationDecision {
		Continue,
		Break,
		Stop,
	};

	Float mIndentWidth;
	Sizef mContentSize;
	Drawable* mExpandIcon{nullptr};
	Drawable* mContractIcon{nullptr};

	UITreeView();

	virtual void createOrUpdateColumns();

	struct MetadataForIndex;

	template <typename Callback> void traverseTree( Callback ) const;

	mutable std::unordered_map<void*, std::unique_ptr<MetadataForIndex>> mViewMetadata;
	mutable std::unordered_map<int, std::unordered_map<void*, UIPushButton*>> mWidgets;
	mutable std::unordered_map<void*, UIWidget*> mRows;

	virtual size_t getItemCount() const;

	UITreeView::MetadataForIndex& getIndexMetadata( const ModelIndex& index ) const;

	EE::UI::UIPushButton* getIndexWidget( const int& column, void* data );

	virtual void onColumnSizeChange( const size_t& colIndex );

	virtual UIPushButton* updateCell( const ModelIndex& index, const size_t& col,
									  const size_t& indentLevel, const Float& yOffset );

	virtual UIWidget* updateRow( const ModelIndex& index, const Float& yOffset );

	virtual void onScrollChange();

	void updateContentSize();
};

}} // namespace EE::UI

#endif // EE_UI_UITREEVIEW_HPP
