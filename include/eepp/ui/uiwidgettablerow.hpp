#ifndef EE_UI_UIWIDGETTABLEROW_HPP
#define EE_UI_UIWIDGETTABLEROW_HPP

#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class UIWidgetTable;

class EE_API UIWidgetTableRow : public UIWidget {
  public:
	static UIWidgetTableRow* New();

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	virtual ~UIWidgetTableRow();

	virtual void setTheme( UITheme* Theme );

	void setColumn( const Uint32& columnIndex, UINode* node );

	UINode* getColumn( const Uint32& columnIndex ) const;

	bool isSelected() const;

	void unselect();

	void select();

	virtual Uint32 onMessage( const NodeMessage* Msg );

  protected:
	friend class UIItemContainer<UIWidgetTable>;
	friend class UIWidgetTable;

	std::vector<UINode*> mCells;

	UIWidgetTableRow();

	UIWidgetTable* gridParent() const;

	void updateRow();

	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	virtual void onStateChange();

	virtual void onParentChange();

	virtual void onAlphaChange();

	virtual void onAutoSize();
};

}} // namespace EE::UI

#endif
