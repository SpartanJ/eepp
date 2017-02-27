#ifndef EE_UICUIMENU_HPP
#define EE_UICUIMENU_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimenusubmenu.hpp>
#include <eepp/ui/uiseparator.hpp>
#include <deque>

namespace EE { namespace UI {

class EE_API UIMenu : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					PaddingContainer(),
					MinWidth( 0 ),
					MinSpaceForIcons( 0 ),
					MinRightMargin( 0 ),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					FontSelectedColor( 0, 0, 0, 255 )
				{
					UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

					if ( NULL != Theme ) {
						Font				= Theme->getFont();
						FontColor			= Theme->getFontColor();
						FontShadowColor		= Theme->getFontShadowColor();
						FontOverColor		= Theme->getFontOverColor();
						FontSelectedColor	= Theme->getFontSelectedColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->getDefaultFont();
				}

				inline ~CreateParams() {}

				Recti				PaddingContainer;
				Uint32				MinWidth;
				Uint32				MinSpaceForIcons;
				Uint32				MinRightMargin;
				Graphics::Font * 	Font;
				ColorA				FontColor;
				ColorA				FontShadowColor;
				ColorA				FontOverColor;
				ColorA				FontSelectedColor;

		};

		static void fixMenuPos( Vector2i& position, UIMenu * Menu, UIMenu * parent = NULL, UIMenuSubMenu * SubMenu = NULL );

		UIMenu( UIMenu::CreateParams& Params );

		UIMenu();

		virtual ~UIMenu();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		Uint32 add( const String& Text, SubTexture * Icon = NULL );

		Uint32 add( UIControl * Control );

		Uint32 addSeparator();

		Uint32 addCheckBox( const String& Text, const bool& Active = false );

		Uint32 addSubMenu( const String& Text, SubTexture * Icon = NULL, UIMenu * SubMenu = NULL );

		UIControl * getItem( const Uint32& Index );
		
		UIControl * getItem( const String& Text );

		Uint32 getItemIndex( UIControl * Item );

		Uint32 getCount() const;

		void remove( const Uint32& Index );

		void remove( UIControl * Ctrl );

		void removeAll();

		void insert( const String& Text, SubTexture * Icon, const Uint32& Index );

		void insert( UIControl * Control, const Uint32& Index );

		virtual void setTheme( UITheme * Theme );

		virtual bool show();

		virtual bool hide();

		const Recti& getPadding() const;

		Font * getFont() const;

		void setFont(Font * font);

		ColorA getFontColor() const;

		void setFontColor(const ColorA & fontColor);

		ColorA getFontShadowColor() const;

		void setFontShadowColor(const ColorA & fontShadowColor);

		ColorA getFontOverColor() const;

		void setFontOverColor(const ColorA & fontOverColor);

		ColorA getFontSelectedColor() const;

		void setFontSelectedColor(const ColorA & fontSelectedColor);

		Uint32 getMinRightMargin() const;

		void setMinRightMargin(const Uint32 & minRightMargin);

	protected:
		friend class UIMenuItem;
		friend class UIMenuCheckBox;
		friend class UIMenuSubMenu;

		std::deque<UIControl *> mItems;
		Recti				mPadding;
		Font *				mFont;
		ColorA				mFontColor;
		ColorA				mFontShadowColor;
		ColorA				mFontOverColor;
		ColorA				mFontSelectedColor;
		Uint32				mMinWidth;
		Uint32				mMinSpaceForIcons;
		Uint32				mMinRightMargin;
		Uint32				mMaxWidth;
		Uint32				mNextPosY;
		Uint32				mBiggestIcon;
		UIControl *			mItemSelected;
		Uint32				mItemSelectedIndex;
		bool				mClickHide;
		Uint32				mLastTickMove;

		virtual void onSizeChange();

		void autoPadding();

		virtual Uint32 onMessage( const UIMessage * Msg );

		void setControlSize( UIControl * Control, const Uint32& position );
		
		void resizeControls();
		
		void rePosControls();
		
		void resizeMe();
		
		UIMenuItem * createMenuItem( const String& Text, SubTexture * Icon );

		UIMenuCheckBox * createMenuCheckBox( const String& Text, const bool& Active );

		UIMenuSubMenu * createSubMenu( const String& Text, SubTexture * Icon, UIMenu * SubMenu );
		
		void doAftersetTheme();

		bool checkControlSize( UIControl * Control, const bool& Resize = true );

		bool isSubMenu( UIControl * Ctrl );

		void setItemSelected( UIControl * Item );

		virtual Uint32 onKeyDown( const UIEventKey& Event );

		void prevSel();

		void nextSel();

		void trySelect( UIControl * Ctrl, bool Up );
};

}}

#endif
