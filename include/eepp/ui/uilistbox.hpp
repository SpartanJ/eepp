#ifndef EE_UICUILISTBOX_HPP
#define EE_UICUILISTBOX_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/ui/uilistboxitem.hpp>

namespace EE { namespace UI {

class EE_API UIListBox : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					RowHeight( 0 ),
					SmoothScroll( true ),
					VScrollMode( UI_SCROLLBAR_AUTO ),
					HScrollMode( UI_SCROLLBAR_AUTO ),
					PaddingContainer(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					FontSelectedColor( 0, 0, 0, 255 ),
					TouchDragDeceleration( 0.01f )
				{
					UITheme * Theme = UIThemeManager::instance()->defaultTheme();

					if ( NULL != Theme ) {
						Font				= Theme->font();
						FontColor			= Theme->fontColor();
						FontOverColor		= Theme->fontOverColor();
						FontSelectedColor	= Theme->fontSelectedColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->defaultFont();
				}

				inline ~CreateParams() {}

				Uint32				RowHeight;
				bool				SmoothScroll;
				UI_SCROLLBAR_MODE	VScrollMode;
				UI_SCROLLBAR_MODE	HScrollMode;
				Recti				PaddingContainer;
				Recti				HScrollPadding;
				Recti				VScrollPadding;
				Graphics::Font *	Font;
				ColorA				FontColor;
				ColorA				FontOverColor;
				ColorA				FontSelectedColor;
				Float				TouchDragDeceleration;
		};

		UIListBox( UIListBox::CreateParams& Params );

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

		UIScrollBar * verticalScrollBar() const;

		UIScrollBar * horizontalScrollBar() const;

		UIListBoxItem * getItem( const Uint32& Index ) const;

		Uint32 getItemIndex( UIListBoxItem * Item );

		Uint32 getItemIndex( const String& Text );

		UIListBoxItem * getItemSelected();

		String getItemSelectedText() const;

		Uint32 getItemSelectedIndex() const;

		std::list<Uint32> getItemsSelectedIndex() const;

		std::list<UIListBoxItem *> getItemsSelected();

		void fontColor( const ColorA& Color );

		const ColorA& fontColor() const;

		void fontOverColor( const ColorA& Color );

		const ColorA& fontOverColor() const;

		void fontSelectedColor( const ColorA& Color );

		const ColorA& fontSelectedColor() const;

		void font( Graphics::Font * font );

		Graphics::Font * font() const;

		void paddingContainer( const Recti& Padding );

		const Recti& paddingContainer() const;

		void smoothScroll( const bool& soft );

		const bool& smoothScroll() const;

		void scrollAlwaysVisible( const bool& visible );

		const bool& scrollAlwaysVisible() const;

		void rowHeight( const Uint32& height );

		const Uint32& rowHeight() const;

		Uint32 count();

		void setSelected( Uint32 Index );

		void setSelected( const String& Text );

		void selectPrev();

		void selectNext();

		void verticalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& verticalScrollMode();

		void horizontalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& horizontalScrollMode();

		virtual void update();

		bool touchDragEnable() const;

		void touchDragEnable( const bool& enable );

		bool touchDragging() const;

		void touchDragging( const bool& dragging );
	protected:
		friend class UIListBoxItem;
		friend class UIItemContainer<UIListBox>;
		friend class UIDropDownList;

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
		Graphics::Font * 	mFont;
		ColorA				mFontColor;
		ColorA				mFontOverColor;
		ColorA				mFontSelectedColor;
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
};

}}

#endif
