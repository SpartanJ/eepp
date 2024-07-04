#ifndef EE_UI_UISCROLLABLEWIDGET_HPP
#define EE_UI_UISCROLLABLEWIDGET_HPP

#include <eepp/ui/uitouchdraggablewidget.hpp>

namespace EE { namespace UI {

class UIScrollBar;

class EE_API UIScrollableWidget : public UIWidget {
  public:
	enum ScrollViewType { Inclusive, Exclusive };

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	void setScrollMode( const ScrollBarMode& verticalMode, const ScrollBarMode& horizontalMode );

	void setVerticalScrollMode( const ScrollBarMode& Mode );

	const ScrollBarMode& getVerticalScrollMode() const;

	void setHorizontalScrollMode( const ScrollBarMode& Mode );

	const ScrollBarMode& getHorizontalScrollMode() const;

	const ScrollViewType& getViewType() const;

	void setScrollViewType( const ScrollViewType& viewType );

	UIScrollBar* getVerticalScrollBar() const;

	UIScrollBar* getHorizontalScrollBar() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual Sizef getContentSize() const = 0;

	void scrollToTop();

	void scrollToBottom();

	void scrollToPosition( const Rectf& pos, const bool& scrollVertically = true,
						   const bool& scrollHorizontally = false );

	Sizef getScrollableArea() const;

	Sizef getVisibleArea() const;

	virtual Rectf getVisibleRect() const;

	bool shouldVerticalScrollBeVisible() const;

	bool isAutoSetClipStep() const;

	void setAutoSetClipStep( bool setClipStep );

	virtual bool isScrollable() const;

  protected:
	ScrollViewType mScrollViewType;
	ScrollBarMode mVScrollMode;
	ScrollBarMode mHScrollMode;
	UIScrollBar* mVScroll;
	UIScrollBar* mHScroll;
	Uint32 mSizeChangeCb;
	Uint32 mPosChangeCb;
	Vector2f mScrollOffset;
	bool mAutoSetClipStep{ true };

	UIScrollableWidget( const std::string& tag );

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onSizeChange();

	virtual void onAlphaChange();

	virtual void onPaddingChange();

	void onValueChangeCb( const Event* Event );

	virtual void onContentSizeChange();

	virtual void updateScroll();

	virtual void onScrollChange();
};

}} // namespace EE::UI

#endif // EE_UI_UISCROLLABLEWIDGET_HPP
