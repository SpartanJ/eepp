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
					UITheme * Theme = UIThemeManager::instance()->DefaultTheme();

					if ( NULL != Theme ) {
						Font				= Theme->Font();
						FontColor			= Theme->FontColor();
						FontOverColor		= Theme->FontOverColor();
						FontSelectedColor	= Theme->FontSelectedColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->DefaultFont();
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

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		void Clear();

		void AddListBoxItems( std::vector<String> Texts );

		Uint32 AddListBoxItem( const String& Text );

		Uint32 AddListBoxItem( UIListBoxItem * Item );

		Uint32 RemoveListBoxItem( const String& Text );

		Uint32 RemoveListBoxItem( UIListBoxItem * Item );

		Uint32 RemoveListBoxItem( Uint32 ItemIndex );

		void RemoveListBoxItems( std::vector<Uint32> ItemsIndex );

		virtual void SetTheme( UITheme * Theme );

		bool IsMultiSelect() const;

		UIScrollBar * VerticalScrollBar() const;

		UIScrollBar * HorizontalScrollBar() const;

		UIListBoxItem * GetItem( const Uint32& Index ) const;

		Uint32 GetItemIndex( UIListBoxItem * Item );

		Uint32 GetItemIndex( const String& Text );

		UIListBoxItem * GetItemSelected();

		String GetItemSelectedText() const;

		Uint32 GetItemSelectedIndex() const;

		std::list<Uint32> GetItemsSelectedIndex() const;

		std::list<UIListBoxItem *> GetItemsSelected();

		void FontColor( const ColorA& Color );

		const ColorA& FontColor() const;

		void FontOverColor( const ColorA& Color );

		const ColorA& FontOverColor() const;

		void FontSelectedColor( const ColorA& Color );

		const ColorA& FontSelectedColor() const;

		void Font( Graphics::Font * Font );

		Graphics::Font * Font() const;

		void PaddingContainer( const Recti& Padding );

		const Recti& PaddingContainer() const;

		void SmoothScroll( const bool& soft );

		const bool& SmoothScroll() const;

		void ScrollAlwaysVisible( const bool& visible );

		const bool& ScrollAlwaysVisible() const;

		void RowHeight( const Uint32& height );

		const Uint32& RowHeight() const;

		Uint32 Count();

		void SetSelected( Uint32 Index );

		void SetSelected( const String& Text );

		void SelectPrev();

		void SelectNext();

		void VerticalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& VerticalScrollMode();

		void HorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& HorizontalScrollMode();

		virtual void Update();

		bool TouchDragEnable() const;

		void TouchDragEnable( const bool& enable );

		bool TouchDragging() const;

		void TouchDragging( const bool& dragging );
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

		void UpdateScroll( bool FromScrollChange = false );

		void OnScrollValueChange( const UIEvent * Event );

		void OnHScrollValueChange( const UIEvent * Event );

		virtual void OnSizeChange();

		void SetRowHeight();

		void UpdateListBoxItemsSize();

		Uint32 GetListBoxItemIndex( const String& Name );

		Uint32 GetListBoxItemIndex( UIListBoxItem * Item );

		void ItemClicked( UIListBoxItem * Item );

		void ResetItemsStates();

		virtual Uint32 OnSelected();

		void ContainerResize();

		void ItemUpdateSize( UIListBoxItem * Item );

		void AutoPadding();

		void FindMaxWidth();

		UIListBoxItem * CreateListBoxItem( const String& Name );

		void CreateItemIndex( const Uint32& i );

		virtual void OnAlphaChange();

		virtual Uint32 OnMessage( const UIMessage * Msg );

		virtual Uint32 OnKeyDown( const UIEventKey &Event );

		void ItemKeyEvent( const UIEventKey &Event );
};

}}

#endif
