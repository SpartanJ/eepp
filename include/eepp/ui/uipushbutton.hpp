#ifndef EE_UICUIPUSHBUTTON_HPP
#define EE_UICUIPUSHBUTTON_HPP

#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIPushButton : public UIWidget {
  public:
	class StyleConfig {
	  public:
		Int32 IconHorizontalMargin = 4;
		bool IconAutoMargin = true;
		Sizei IconMinSize;
	};

	static UIPushButton* New();

	UIPushButton();

	virtual ~UIPushButton();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual UIPushButton* setIcon( Drawable* icon );

	virtual UIImage* getIcon() const;

	virtual UIPushButton* setText( const String& text );

	virtual const String& getText();

	void setIconHorizontalMargin( Int32 margin );

	const Int32& getIconHorizontalMargin() const;

	UITextView* getTextBox() const;

	const StyleConfig& getStyleConfig() const;

	void setIconMinimumSize( const Sizei& minIconSize );

	void setStyleConfig( const StyleConfig& styleConfig );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

  protected:
	StyleConfig mStyleConfig;
	UIImage* mIcon;
	UITextView* mTextBox;

	explicit UIPushButton( const std::string& tag );

	virtual void onSizeChange();

	virtual void onAlphaChange();

	virtual void onStateChange();

	virtual void onAlignChange();

	virtual void onThemeLoaded();

	virtual void onAutoSize();

	virtual void onPaddingChange();

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual Uint32 onKeyUp( const KeyEvent& Event );

	void autoIconHorizontalMargin();
};

}} // namespace EE::UI

#endif
