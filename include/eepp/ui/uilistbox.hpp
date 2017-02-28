#ifndef EE_UICUILISTBOX_HPP
#define EE_UICUILISTBOX_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/ui/uilistboxitem.hpp>

namespace EE { namespace UI {

class EE_API UIListBox : public UIComplexControl {
	public:
		static UIListBox * New();

		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					RowHeight( 0 ),
					SmoothScroll( true ),
					VScrollMode( UI_SCROLLBAR_AUTO ),
					HScrollMode( UI_SCROLLBAR_AUTO ),
					PaddingContainer(),
					TouchDragDeceleration( 0.01f )
				{
					fontStyleConfig = UIThemeManager::instance()->getDefaultFontStyleConfig();
				}

				inline ~CreateParams() {}

				FontStyleConfig		fontStyleConfig;
				Uint32				RowHeight;
				bool				SmoothScroll;
				UI_SCROLLBAR_MODE	VScrollMode;
				UI_SCROLLBAR_MODE	HScrollMode;
				Recti				PaddingContainer;
				Recti				HScrollPadding;
				Recti				VScrollPadding;
				Float				TouchDragDeceleration;
		};

		UIListBox( UIListBox::CreateParams& Params );

		UIListBox();

		virtual ~UIListBox();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		void clear();

		void addListBoxItems( std::vector<String> Texts );

		Uint32 addListBoxItem( const String& Text );

		Uint32 addListBoxItem( UIListBoxItem * Item );

		Uint32 removeListBoxItem( const String& Text );

		Uint32 removeListBoxItem( UIListBoxItem * Item );

		Uint32 removeListBoxItem( Uint32 ItemIndex );

		void removeListBoxItems( std::vector<Uint32> ItemsIndex );

		virtual void setTheme( UITheme * Theme );

		bool isMultiSelect() const;

		UIScrollBar * getVerticalScrollBar() const;

		UIScrollBar * getHorizontalScrollBar() const;

		UIListBoxItem * getItem( const Uint32& Index ) const;

		Uint32 getItemIndex( UIListBoxItem * Item );

		Uint32 getItemIndex( const String& Text );

		UIListBoxItem * getItemSelected();

		String getItemSelectedText() const;

		Uint32 getItemSelectedIndex() const;

		std::list<Uint32> getItemsSelectedIndex() const;

		std::list<UIListBoxItem *> getItemsSelected();

		void setFontColor( const ColorA& Color );

		const ColorA& getFontColor() const;

		void setFontOverColor( const ColorA& Color );

		const ColorA& getFontOverColor() const;

		void setFontSelectedColor( const ColorA& Color );

		const ColorA& getFontSelectedColor() const;

		void setFont( Graphics::Font * font );

		Graphics::Font * getFont() const;

		void setContainerPadding( const Recti& Padding );

		const Recti& getContainerPadding() const;

		void setSmoothScroll( const bool& soft );

		const bool& isSmoothScroll() const;

		void setRowHeight( const Uint32& height );

		const Uint32& getRowHeight() const;

		Uint32 getCount();

		void setSelected( Uint32 Index );

		void setSelected( const String& Text );

		void selectPrev();

		void selectNext();

		void setVerticalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& getVerticalScrollMode();

		void setHorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& getHorizontalScrollMode();

		virtual void update();

		bool isTouchDragEnabled() const;

		void setTouchDragEnabled( const bool& enable );

		bool isTouchDragging() const;

		void setTouchDragging( const bool& dragging );

		Float getTouchDragDeceleration() const;

		void setTouchDragDeceleration(const Float & touchDragDeceleration);

		FontStyleConfig getFontStyleConfig() const;

		void setFontStyleConfig(const FontStyleConfig & fontStyleConfig);
	protected:
		friend class UIListBoxItem;
		friend class UIItemContainer<UIListBox>;
		friend class UIDropDownList;

		FontStyleConfig		mFontStyleConfig;
		Uint32 				mRowHeight;
		UI_SCROLLBAR_MODE	mVScrollMode;
		UI_SCROLLBAR_MODE	mHScrollMode;
		bool 				mSmoothScroll;
		Recti				mPaddingContainer;
		Recti				mHScrollPadding;
		Recti				mVScrollPadding;
		UIItemContainer<UIListBox> * mContainer;
		UIScrollBar * 		mVScrollBar;
		UIScrollBar * 		mHScrollBar;
		Uint32 				mLastPos;
		Uint32 				mMaxTextWidth;
		Int32 				mHScrollInit;
		Int32 				mItemsNotVisible;
		Uint32				mLastTickMove;

		Uint32				mVisibleFirst;
		Uint32				mVisibleLast;

		Vector2i			mTouchDragPoint;
		Float				mTouchDragAcceleration;
		Float				mTouchDragDeceleration;

		std::list<Uint32>				mSelected;
		std::vector<UIListBoxItem *> 	mItems;
		std::vector<String>				mTexts;

		void updateScroll( bool FromScrollChange = false );

		void onScrollValueChange( const UIEvent * Event );

		void onHScrollValueChange( const UIEvent * Event );

		virtual void onSizeChange();

		void setRowHeight();

		void updateListBoxItemsSize();

		Uint32 getListBoxItemIndex( const String& Name );

		Uint32 getListBoxItemIndex( UIListBoxItem * Item );

		void itemClicked( UIListBoxItem * Item );

		void resetItemsStates();

		virtual Uint32 onSelected();

		void containerResize();

		void itemUpdateSize( UIListBoxItem * Item );

		void autoPadding();

		void findMaxWidth();

		UIListBoxItem * createListBoxItem( const String& Name );

		void createItemIndex( const Uint32& i );

		virtual void onAlphaChange();

		virtual Uint32 onMessage( const UIMessage * Msg );

		virtual Uint32 onKeyDown( const UIEventKey &Event );

		void itemKeyEvent( const UIEventKey &Event );

		void setHScrollStep();
};

}}

#endif
