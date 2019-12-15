#ifndef EE_UISCROLLVIEW_HPP
#define EE_UISCROLLVIEW_HPP

#include <eepp/ui/uitouchdragablewidget.hpp>

namespace EE { namespace UI {

class UIScrollBar;

class EE_API UIScrollView : public UITouchDragableWidget {
	public:
		enum ScrollViewType {
			Inclusive,
			Exclusive
		};

		static UIScrollView * New();

		UIScrollView();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		void setVerticalScrollMode( const ScrollBarMode& Mode );

		const ScrollBarMode& getVerticalScrollMode();

		void setHorizontalScrollMode( const ScrollBarMode& Mode );

		const ScrollBarMode& getHorizontalScrollMode();

		const ScrollViewType& getViewType() const;

		void setViewType( const ScrollViewType& viewType );

		UIScrollBar * getVerticalScrollBar() const;

		UIScrollBar * getHorizontalScrollBar() const;

		UIWidget * getContainer() const;

		virtual bool applyProperty( const StyleSheetProperty& attribute );

		virtual std::string getPropertyString(const PropertyDefinition* propertyDef);
	protected:
		ScrollViewType mViewType;
		ScrollBarMode mVScrollMode;
		ScrollBarMode mHScrollMode;
		UIScrollBar * mVScroll;
		UIScrollBar * mHScroll;
		UIWidget * mContainer;
		Node * mScrollView;
		Uint32 mSizeChangeCb;
		Uint32 mPosChangeCb;

		virtual Uint32 onMessage( const NodeMessage * Msg );

		virtual void onSizeChange();

		virtual void onAlphaChange();

		virtual void onChildCountChange();

		virtual void onPaddingChange();

		void onValueChangeCb( const Event * Event );

		void onScrollViewSizeChange( const Event * Event );

		void onScrollViewPositionChange( const Event * Event );

		void containerUpdate();

		void updateScroll();

		virtual void onTouchDragValueChange( Vector2f diff );

		virtual bool isTouchOverAllowedChilds();
};

}}

#endif
