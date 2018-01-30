#ifndef EE_UICUILISTBOX_HPP
#define EE_UICUILISTBOX_HPP

#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uitouchdragablewidget.hpp>

namespace EE { namespace UI {

class EE_API UIListBox : public UITouchDragableWidget {
	public:
		static UIListBox * New();

		UIListBox();

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

		UIScrollBar * getVerticalScrollBar() const;

		UIScrollBar * getHorizontalScrollBar() const;

		UIListBoxItem * getItem( const Uint32& Index ) const;

		Uint32 getItemIndex( UIListBoxItem * Item );

		Uint32 getItemIndex( const String& Text );

		UIListBoxItem * getItemSelected();

		String getItemSelectedText() const;

		Uint32 getItemSelectedIndex() const;

		std::list<Uint32> getItemsSelectedIndex() const;

		std::list<UIListBoxItem *> getItemsSelected();

		void setFontColor( const Color& Color );

		const Color& getFontColor() const;

		void setFontOverColor( const Color& Color );

		const Color& getFontOverColor() const;

		void setFontSelectedColor( const Color& Color );

		const Color& getFontSelectedColor() const;

		void setFont( Graphics::Font * font );

		Graphics::Font * getFont() const;

		void setContainerPadding( const Rectf& Padding );

		const Rectf& getContainerPadding() const;

		void setSmoothScroll( const bool& soft );

		const bool& isSmoothScroll() const;

		void setRowHeight( const Uint32& height );

		const Uint32& getRowHeight() const;

		Uint32 getCount();

		void setSelected( Uint32 Index );

		void setSelected( const String& Text );

		void selectPrev();

		void selectNext();

		void setVerticalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& getVerticalScrollMode();

		void setHorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& getHorizontalScrollMode();

		UIFontStyleConfig getFontStyleConfig() const;

		void setFontStyleConfig(const UIFontStyleConfig & fontStyleConfig);

		void loadFromXmlNode(const pugi::xml_node & node);
	protected:
		friend class UIListBoxItem;
		friend class UIItemContainer<UIListBox>;
		friend class UIDropDownList;

		UIFontStyleConfig		mFontStyleConfig;
		Uint32 				mRowHeight;
		UI_SCROLLBAR_MODE	mVScrollMode;
		UI_SCROLLBAR_MODE	mHScrollMode;
		Rectf				mContainerPadding;
		Rectf				mHScrollPadding;
		Rectf				mVScrollPadding;
		UIItemContainer<UIListBox> * mContainer;
		UIScrollBar * 		mVScrollBar;
		UIScrollBar * 		mHScrollBar;
		Uint32 				mLastPos;
		Uint32 				mMaxTextWidth;
		Int32 				mHScrollInit;
		Int32 				mItemsNotVisible;
		Uint32				mLastTickMove;

		Uint32				mVisibleFirst;
		Uint32				mVisibleLast;

		bool 				mSmoothScroll;

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

		void setHScrollStep();

		virtual void onTouchDragValueChange( Vector2f diff );

		virtual bool isTouchOverAllowedChilds();
};

}}

#endif
