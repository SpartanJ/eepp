#ifndef EE_UICUITEXTEDIT_HPP
#define EE_UICUITEXTEDIT_HPP

#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

class UIScrollBar;

class EE_API UITextEdit : public UIWidget {
  public:
	static UITextEdit* New();

	UITextEdit();

	virtual ~UITextEdit();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	const String& getText() const;

	void setText( const String& Txt );

	UITextInput* getTextInput() const;

	UIScrollBar* getHScrollBar() const;

	UIScrollBar* getVScrollBar() const;

	void setAllowEditing( const bool& allow );

	const bool& isEditingAllowed() const;

	void setVerticalScrollMode( const ScrollBarMode& Mode );

	const ScrollBarMode& getVerticalScrollMode();

	void setHorizontalScrollMode( const ScrollBarMode& Mode );

	const ScrollBarMode& getHorizontalScrollMode();

	UIFontStyleConfig getFontStyleConfig() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

  protected:
	UITextInput* mTextInput;
	UIScrollBar* mHScrollBar;
	UIScrollBar* mVScrollBar;
	ScrollBarMode mHScrollBarMode;
	ScrollBarMode mVScrollBarMode;
	bool mSkipValueChange;
	Rectf mContainerPadding;

	virtual void onSizeChange();

	virtual void onAlphaChange();

	virtual void onParentSizeChange( const Vector2f& SizeChange );

	virtual void onPaddingChange();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void onVScrollValueChange( const Event* Event );

	void onHScrollValueChange( const Event* Event );

	void onInputSizeChange( const Event* Event = NULL );

	void onCursorPosChange( const Event* Event );

	void autoPadding();

	void scrollbarsSet();

	void fixScroll();

	void fixScrollToCursor();

	void shrinkText( const Float& width );

	void textInputTryResize( Sizef aSize, Sizef iSize );

	Sizef getAvailableSize();

	Sizef getInputSize();
};

}} // namespace EE::UI

#endif
