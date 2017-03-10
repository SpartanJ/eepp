#ifndef EE_UICUIDROPDOWNLIST_HPP
#define EE_UICUIDROPDOWNLIST_HPP

#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uilistbox.hpp>

namespace EE { namespace UI {

class EE_API UIDropDownList : public UITextInput {
	public:
		static UIDropDownList * New();

		UIDropDownList();

		virtual ~UIDropDownList();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		UIListBox * getListBox() const;

		virtual void update();

		void showList();

		bool getPopUpToMainControl() const;

		void setPopUpToMainControl(bool popUpToMainControl);

		Uint32 getMaxNumVisibleItems() const;

		void setMaxNumVisibleItems(const Uint32 & maxNumVisibleItems);

		DropDownListStyleConfig getStyleConfig() const;

		void setStyleConfig(const DropDownListStyleConfig & styleConfig);

		void loadFromXmlNode(const pugi::xml_node & node);
	protected:
		friend class UIComboBox;

		DropDownListStyleConfig mStyleConfig;
		UIListBox *		mListBox;
		UIControl *		mFriendCtrl;

		void onListBoxFocusLoss( const UIEvent * Event );

		virtual void onItemSelected( const UIEvent * Event );

		virtual void show();

		virtual void hide();

		Uint32 onMouseClick( const Vector2i& position, const Uint32 flags );

		virtual void onItemClicked( const UIEvent * Event );

		virtual void onItemKeyDown( const UIEvent * Event );

		virtual void onControlClear( const UIEvent * Event );

		Uint32 onKeyDown( const UIEventKey &Event );

		virtual void onSizeChange();

		virtual void onAutoSize();

		virtual void autoSizeControl();

		virtual void onThemeLoaded();

		void setFriendControl( UIControl * friendCtrl );

		void destroyListBox();
};

}}

#endif
