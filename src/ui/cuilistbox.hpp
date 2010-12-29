#ifndef EE_UICUILISTBOX_HPP
#define EE_UICUILISTBOX_HPP

#include "cuicontrolanim.hpp"
#include "cuiscrollbar.hpp"
#include "tuiitemcontainer.hpp"
#include "cuilistboxitem.hpp"

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
					FontSelectedColor( 0, 0, 0, 255 )
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
				cFont *				Font;
				eeColorA			FontColor;
				eeColorA			FontOverColor;
				eeColorA			FontSelectedColor;
		};

		cUIListBox( cUIListBox::CreateParams& Params );

		virtual ~cUIListBox();

		void AddListBoxItems( std::vector<std::wstring> Texts );

		Uint32 AddListBoxItem( const std::string& Text );

		Uint32 AddListBoxItem( const std::wstring& Text );

		Uint32 AddListBoxItem( cUIListBoxItem * Item );

		Uint32 RemoveListBoxItem( const std::wstring& Text );

		Uint32 RemoveListBoxItem( cUIListBoxItem * Item );

		Uint32 RemoveListBoxItem( Uint32 ItemIndex );

		void RemoveListBoxItems( std::vector<Uint32> ItemsIndex );

		virtual void SetTheme( cUITheme * Theme );

		bool IsMultiSelect() const;

		cUIScrollBar * VerticalScrollBar() const;

		cUIScrollBar * HorizontalScrollBar() const;

		cUIListBoxItem * GetItem( const Uint32& Index ) const;

		Uint32 GetItemIndex( cUIListBoxItem * Item );

		cUIListBoxItem * GetItemSelected();

		std::wstring GetItemSelectedText() const;

		Uint32 GetItemSelectedIndex() const;

		std::list<Uint32> GetItemsSelectedIndex() const;

		std::list<cUIListBoxItem *> GetItemsSelected();

		void FontColor( const eeColorA& Color );

		const eeColorA& FontColor() const;

		void FontOverColor( const eeColorA& Color );

		const eeColorA& FontOverColor() const;

		void FontSelectedColor( const eeColorA& Color );

		const eeColorA& FontSelectedColor() const;

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

		void SelectPrev();

		void SelectNext();

		void VerticalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& VerticalScrollMode();

		void HorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& HorizontalScrollMode();
	protected:
		friend class cUIListBoxItem;
		friend class tUIItemContainer<cUIListBox>;
		friend class cUIDropDownList;

		Uint32 				mRowHeight;
		UI_SCROLLBAR_MODE	mVScrollMode;
		UI_SCROLLBAR_MODE	mHScrollMode;
		bool 				mSmoothScroll;
		eeRecti 			mPaddingContainer;
		tUIItemContainer<cUIListBox> * mContainer;
		cUIScrollBar * 		mVScrollBar;
		cUIScrollBar * 		mHScrollBar;
		cFont * 			mFont;
		eeColorA 			mFontColor;
		eeColorA 			mFontOverColor;
		eeColorA 			mFontSelectedColor;
		Uint32 				mLastPos;
		Uint32 				mMaxTextWidth;
		Int32 				mHScrollInit;
		Int32 				mItemsNotVisible;
		Uint32				mLastTickMove;

		Uint32				mVisibleFirst;
		Uint32				mVisibleLast;

		std::list<Uint32>				mSelected;
		std::vector<cUIListBoxItem *> 	mItems;
		std::vector<std::wstring>		mTexts;

		void UpdateScroll( bool FromScrollChange = false );

		void OnScrollValueChange( const cUIEvent * Event );

		void OnHScrollValueChange( const cUIEvent * Event );

		virtual void OnSizeChange();

		void SetRowHeight();

		void UpdateListBoxItemsSize();

		Uint32 GetListBoxItemIndex( const std::wstring& Name );

		Uint32 GetListBoxItemIndex( cUIListBoxItem * Item );

		void ItemClicked( cUIListBoxItem * Item );

		void ResetItemsStates();

		virtual Uint32 OnSelected();

		void ContainerResize();

		void ItemUpdateSize( cUIListBoxItem * Item );

		void AutoPadding();

		void FindMaxWidth();

		void ManageKeyboard();

		cUIListBoxItem * CreateListBoxItem( const std::wstring& Name );

		void CreateItemIndex( const Uint32& i );

		virtual void OnAlphaChange();

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		virtual Uint32 OnKeyDown( const cUIEventKey &Event );

		void ItemKeyEvent( const cUIEventKey &Event );
};

}}

#endif
