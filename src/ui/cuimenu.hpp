#ifndef EE_UICUIMENU
#define EE_UICUIMENU

#include "cuicontrolanim.hpp"
#include "cuimenuitem.hpp"
#include "cuiseparator.hpp"

namespace EE { namespace UI {

class EE_API cUIMenu : public cUIControlAnim {
	public:
		class CreateParams : public cUIControlAnim::CreateParams {
			public:
				inline CreateParams() :
					cUIControl::CreateParams(),
					RowHeight( 0 ),
					PaddingContainer(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					MinWidth( 0 )
				{
					cUITheme * Theme = cUIThemeManager::instance()->DefaultTheme();

					if ( NULL != Theme ) {
						Font			= Theme->Font();

						if ( NULL == Font )
							Font = cUIThemeManager::instance()->DefaultFont();

						FontColor		= Theme->FontColor();
						FontShadowColor	= Theme->FontShadowColor();
						FontOverColor	= Theme->FontOverColor();
					}
				}

				inline ~CreateParams() {}

				Uint32		RowHeight;
				eeRecti		PaddingContainer;
				cFont * 	Font;
				eeColorA 	FontColor;
				eeColorA	FontShadowColor;
				eeColorA 	FontOverColor;
				Uint32		MinWidth;

		};

		cUIMenu( cUIMenu::CreateParams& Params );

		~cUIMenu();

		Uint32 Add( const std::wstring& Text, cShape * Icon = NULL );

		Uint32 Add( cUIMenuItem * Control );

		Uint32 AddSeparator();

		cUIControl * GetItem( const Uint32& Index );
		
		cUIControl * GetItem( const std::wstring& Text );

		Uint32 Count() const;

		void Remove( const Uint32& Index );

		void Remove( cUIControl * Ctrl );

		void RemoveAll();

		void Insert( const std::wstring& Text, cShape * Icon, const Uint32& Index );

		void Insert( cUIControl * Control, const Uint32& Index );

		virtual void SetTheme( cUITheme * Theme );
	protected:
		std::deque<cUIControl *> mItems;
		eeRecti				mPadding;
		cFont * 			mFont;
		eeColorA 			mFontColor;
		eeColorA			mFontShadowColor;
		eeColorA 			mFontOverColor;
		Uint32				mMinWidth;
		Uint32				mMaxWidth;
		Uint32				mRowHeight;
		Uint32				mNextPosY;
		Uint32				mBiggestIcon;

		virtual void OnSizeChange();

		void AutoPadding();

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		void SetControlSize( cUIControl * Control, const Uint32& Pos );
		
		void ResizeControls();
		
		void ReposControls();
		
		void ResizeMe();
		
		cUIMenuItem * CreateMenuItem( const std::wstring& Text, cShape * Icon );
		
		void DoAfterSetTheme();
};

}}

#endif
