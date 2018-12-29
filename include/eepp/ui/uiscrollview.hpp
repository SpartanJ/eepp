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

		void setVerticalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& getVerticalScrollMode();

		void setHorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& getHorizontalScrollMode();

		const ScrollViewType& getViewType() const;

		void setViewType( const ScrollViewType& viewType );

		UIScrollBar * getVerticalScrollBar() const;

		UIScrollBar * getHorizontalScrollBar() const;

		UINode * getContainer() const;

		virtual bool setAttribute( const NodeAttribute& attribute, const Uint32& state = UIState::StateFlagNormal );
	protected:
		ScrollViewType mViewType;
		UI_SCROLLBAR_MODE mVScrollMode;
		UI_SCROLLBAR_MODE mHScrollMode;
		UIScrollBar * mVScroll;
		UIScrollBar * mHScroll;
		UINode * mContainer;
		Node * mScrollView;
		Uint32 mSizeChangeCb;

		virtual void onSizeChange();

		virtual void onAlphaChange();

		virtual void onChildCountChange();

		virtual void onPaddingChange();

		void onValueChangeCb( const Event * Event );

		void onScrollViewSizeChange( const Event * Event );

		void containerUpdate();

		void updateScroll();

		virtual void onTouchDragValueChange( Vector2f diff );

		virtual bool isTouchOverAllowedChilds();
};

}}

#endif
