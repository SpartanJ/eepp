#ifndef EE_UICUILISTBOX_HPP
#define EE_UICUILISTBOX_HPP

#include "cuicontrolanim.hpp"
#include "cuiscrollbar.hpp"
#include "cuilistboxitem.hpp"

namespace EE { namespace UI {

class EE_API cUIListBox : public cUIControlAnim {
	public:
		class CreateParams : public cUIControlAnim::CreateParams {
			public:
				inline CreateParams() :
					cUIControl::CreateParams(),
					RowHeight( 0 ),
					ScrollAlwaysVisible( false ),
					SoftScroll( true ),
					AllowHorizontalScroll( true ),
					PaddingContainer(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					FontSelectedColor( 0, 0, 0, 255 )
				{
				}

				inline ~CreateParams() {}

				Uint32		RowHeight;
				bool		ScrollAlwaysVisible;
				bool		SoftScroll;
				bool		AllowHorizontalScroll;
				eeRecti		PaddingContainer;
				cFont * 	Font;
				eeColorA 	FontColor;
				eeColorA 	FontOverColor;
				eeColorA	FontSelectedColor;
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

		cUIScrollBar * ScrollBar() const;

		cUIScrollBar * HScrollBar() const;

		cUIListBoxItem * GetItem( const Uint32& Index ) const;

		Uint32 GetItemIndex( cUIListBoxItem * Item );

		cUIListBoxItem * GetItemSelected();

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

		void SoftScroll( const bool& soft );

		const bool& SoftScroll() const;

		void ScrollAlwaysVisible( const bool& visible );

		const bool& ScrollAlwaysVisible() const;

		void RowHeight( const Uint32& height );

		const Uint32& RowHeight() const;

		void AllowHorizontalScroll( const bool& allow );

		const bool& AllowHorizontalScroll() const;
	protected:
		friend class cUIListBoxItem;

		Uint32 				mRowHeight;
		bool 				mScrollAlwaysVisible;
		bool 				mSoftScroll;
		eeRecti 			mPaddingContainer;
		cUIControl * 		mContainer;
		cUIScrollBar * 		mScrollBar;
		cUIScrollBar * 		mHScrollBar;
		cFont * 			mFont;
		eeColorA 			mFontColor;
		eeColorA 			mFontOverColor;
		eeColorA 			mFontSelectedColor;
		Uint32 				mLastPos;
		bool 				mDisableScrollUpdate;
		Uint32 				mMaxTextWidth;
		bool 				mAllowHorizontalScroll;
		Int32 				mHScrollInit;
		Int32 				mItemsNotVisible;
		std::list<Uint32>	mSelected;
		std::vector<cUIListBoxItem *> 	mItems;

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
};

}}

#endif
