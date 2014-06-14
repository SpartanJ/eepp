#ifndef EE_UICUILISTBOX_HPP
#define EE_UICUILISTBOX_HPP

#include <eepp/ui/cuicontrolanim.hpp>
#include <eepp/ui/cuiscrollbar.hpp>
#include <eepp/ui/tuiitemcontainer.hpp>
#include <eepp/ui/cuilistboxitem.hpp>

namespace EE { namespace UI {

class EE_API cUIListBox : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
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
					cUITheme * Theme = cUIThemeManager::instance()->DefaultTheme();

					if ( NULL != Theme ) {
						Font				= Theme->Font();
						FontColor			= Theme->FontColor();
						FontOverColor		= Theme->FontOverColor();
						FontSelectedColor	= Theme->FontSelectedColor();
					}

					if ( NULL == Font )
						Font = cUIThemeManager::instance()->DefaultFont();
				}

				inline ~CreateParams() {}

				Uint32				RowHeight;
				bool				SmoothScroll;
				UI_SCROLLBAR_MODE	VScrollMode;
				UI_SCROLLBAR_MODE	HScrollMode;
				eeRecti				PaddingContainer;
				eeRecti				HScrollPadding;
				eeRecti				VScrollPadding;
				cFont *				Font;
				ColorA			FontColor;
				ColorA			FontOverColor;
				ColorA			FontSelectedColor;
				Float				TouchDragDeceleration;
		};

		cUIListBox( cUIListBox::CreateParams& Params );

		virtual ~cUIListBox();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		void Clear();

		void AddListBoxItems( std::vector<String> Texts );

		Uint32 AddListBoxItem( const String& Text );

		Uint32 AddListBoxItem( cUIListBoxItem * Item );

		Uint32 RemoveListBoxItem( const String& Text );

		Uint32 RemoveListBoxItem( cUIListBoxItem * Item );

		Uint32 RemoveListBoxItem( Uint32 ItemIndex );

		void RemoveListBoxItems( std::vector<Uint32> ItemsIndex );

		virtual void SetTheme( cUITheme * Theme );

		bool IsMultiSelect() const;

		cUIScrollBar * VerticalScrollBar() const;

		cUIScrollBar * HorizontalScrollBar() const;

		cUIListBoxItem * GetItem( const Uint32& Index ) const;

		Uint32 GetItemIndex( cUIListBoxItem * Item );

		Uint32 GetItemIndex( const String& Text );

		cUIListBoxItem * GetItemSelected();

		String GetItemSelectedText() const;

		Uint32 GetItemSelectedIndex() const;

		std::list<Uint32> GetItemsSelectedIndex() const;

		std::list<cUIListBoxItem *> GetItemsSelected();

		void FontColor( const ColorA& Color );

		const ColorA& FontColor() const;

		void FontOverColor( const ColorA& Color );

		const ColorA& FontOverColor() const;

		void FontSelectedColor( const ColorA& Color );

		const ColorA& FontSelectedColor() const;

		void Font( cFont * Font );

		cFont * Font() const;

		void PaddingContainer( const eeRecti& Padding );

		const eeRecti& PaddingContainer() const;

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
		friend class cUIListBoxItem;
		friend class tUIItemContainer<cUIListBox>;
		friend class cUIDropDownList;

		Uint32 				mRowHeight;
		UI_SCROLLBAR_MODE	mVScrollMode;
		UI_SCROLLBAR_MODE	mHScrollMode;
		bool 				mSmoothScroll;
		eeRecti 			mPaddingContainer;
		eeRecti				mHScrollPadding;
		eeRecti				mVScrollPadding;
		tUIItemContainer<cUIListBox> * mContainer;
		cUIScrollBar * 		mVScrollBar;
		cUIScrollBar * 		mHScrollBar;
		cFont * 			mFont;
		ColorA 			mFontColor;
		ColorA 			mFontOverColor;
		ColorA 			mFontSelectedColor;
		Uint32 				mLastPos;
		Uint32 				mMaxTextWidth;
		Int32 				mHScrollInit;
		Int32 				mItemsNotVisible;
		Uint32				mLastTickMove;

		Uint32				mVisibleFirst;
		Uint32				mVisibleLast;

		eeVector2i			mTouchDragPoint;
		Float				mTouchDragAcceleration;
		Float				mTouchDragDeceleration;

		std::list<Uint32>				mSelected;
		std::vector<cUIListBoxItem *> 	mItems;
		std::vector<String>				mTexts;

		void UpdateScroll( bool FromScrollChange = false );

		void OnScrollValueChange( const cUIEvent * Event );

		void OnHScrollValueChange( const cUIEvent * Event );

		virtual void OnSizeChange();

		void SetRowHeight();

		void UpdateListBoxItemsSize();

		Uint32 GetListBoxItemIndex( const String& Name );

		Uint32 GetListBoxItemIndex( cUIListBoxItem * Item );

		void ItemClicked( cUIListBoxItem * Item );

		void ResetItemsStates();

		virtual Uint32 OnSelected();

		void ContainerResize();

		void ItemUpdateSize( cUIListBoxItem * Item );

		void AutoPadding();

		void FindMaxWidth();

		cUIListBoxItem * CreateListBoxItem( const String& Name );

		void CreateItemIndex( const Uint32& i );

		virtual void OnAlphaChange();

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		virtual Uint32 OnKeyDown( const cUIEventKey &Event );

		void ItemKeyEvent( const cUIEventKey &Event );
};

}}

#endif
