#ifndef EE_UI_UITREEVIEW_HPP
#define EE_UI_UITREEVIEW_HPP

#include <eepp/ui/abstract/uiabstracttableview.hpp>
#include <eepp/ui/uitablerow.hpp>
#include <memory>
#include <unordered_map>

using namespace EE::UI::Abstract;

namespace EE { namespace UI {

class UITableRow;

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

	bool getExpandersAsIcons() const;

	void setExpandersAsIcons( bool expandersAsIcons );

	Float getMaxColumnContentWidth( const size_t& colIndex );

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
	bool mExpandersAsIcons{false};

	UITreeView();

	virtual void createOrUpdateColumns();

	struct MetadataForIndex {
		bool open{false};
	};

	template <typename Callback> void traverseTree( Callback ) const;

	mutable std::map<void*, MetadataForIndex> mViewMetadata;

	virtual size_t getItemCount() const;

	UITreeView::MetadataForIndex& getIndexMetadata( const ModelIndex& index ) const;

	virtual void onColumnSizeChange( const size_t& colIndex );

	virtual UIWidget* updateCell( const int& rowIndex, const ModelIndex& index,
								  const size_t& indentLevel, const Float& yOffset );

	virtual UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual void onOpenTreeModelIndex( const ModelIndex& index, bool open );

	void updateContentSize();
};

}} // namespace EE::UI

#endif // EE_UI_UITREEVIEW_HPP