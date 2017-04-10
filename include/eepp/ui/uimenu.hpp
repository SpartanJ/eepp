#ifndef EE_UICUIMENU_HPP
#define EE_UICUIMENU_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimenusubmenu.hpp>
#include <eepp/ui/uimenuseparator.hpp>
#include <deque>

namespace EE { namespace UI {

class EE_API UIMenu : public UIWidget {
	public:
		static UIMenu * New();

		static void fixMenuPos( Vector2i& position, UIMenu * Menu, UIMenu * parent = NULL, UIMenuSubMenu * SubMenu = NULL );

		UIMenu();

		virtual ~UIMenu();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		Uint32 add( const String& Text, Drawable * Icon = NULL );

		Uint32 add( UIControl * Control );

		Uint32 addSeparator();

		Uint32 addCheckBox( const String& Text, const bool& Active = false );

		Uint32 addSubMenu( const String& Text, Drawable * Icon = NULL, UIMenu * SubMenu = NULL );

		UIControl * getItem( const Uint32& Index );
		
		UIControl * getItem( const String& Text );

		Uint32 getItemIndex( UIControl * Item );

		Uint32 getCount() const;

		void remove( const Uint32& Index );

		void remove( UIControl * Ctrl );

		void removeAll();

		void insert( const String& Text, Drawable * Icon, const Uint32& Index );

		void insert( UIControl * Control, const Uint32& Index );

		virtual void setTheme( UITheme * Theme );

		virtual bool show();

		virtual bool hide();

		const Recti& getPadding() const;

		Uint32 getMinRightMargin() const;

		void setMinRightMargin(const Uint32 & minRightMargin);

		UITooltipStyleConfig getFontStyleConfig() const;

		void setFontStyleConfig(const UITooltipStyleConfig & fontStyleConfig);

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		friend class UIMenuItem;
		friend class UIMenuCheckBox;
		friend class UIMenuSubMenu;

		std::deque<UIControl *> mItems;
		UIMenuStyleConfig		mStyleConfig;
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
		
		UIMenuItem * createMenuItem( const String& Text, Drawable * Icon );

		UIMenuCheckBox * createMenuCheckBox( const String& Text, const bool& Active );

		UIMenuSubMenu * createSubMenu( const String& Text, Drawable * Icon, UIMenu * SubMenu );
		
		void onThemeLoaded();

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
