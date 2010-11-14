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

		cUIListBoxItem * GetItem( const Uint32& Index ) const;

		Uint32 GetItemIndex( cUIListBoxItem * Item );

		cUIListBoxItem * GetItemSelected();

		Uint32 GetItemSelectedIndex() const;

		std::list<Uint32> GetItemsSelectedIndex() const;

		std::list<cUIListBoxItem *> GetItemsSelected();
	protected:
		friend class cUIListBoxItem;

		Uint32 mRowHeight;
		bool mScrollAlwaysVisible;
		bool mSoftScroll;
		eeRecti mPaddingContainer;
		cUIControl * mContainer;
		cUIScrollBar * mScrollBar;
		cFont * mFont;
		eeColorA mFontColor;
		eeColorA mFontOverColor;
		eeColorA mFontSelectedColor;

		std::vector<cUIListBoxItem *> 	mItems;
		std::list<Uint32>				mSelected;

		Uint32 mLastPos;

		bool mDisableScrollUpdate;

		void UpdateScroll( bool FromScrollChange = false );
		void OnScrollValueChange( const cUIEvent * Event );
		virtual void OnSizeChange();

		void SetRowHeight();

		void UpdateListBoxItemsSize();

		Uint32 GetListBoxItemIndex( const std::wstring& Name );

		Uint32 GetListBoxItemIndex( cUIListBoxItem * Item );

		void ItemClicked( cUIListBoxItem * Item );

		void ResetItemsStates();
};

}}

#endif
