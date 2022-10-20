#ifndef EE_UICPROGRESSBAR_HPP
#define EE_UICPROGRESSBAR_HPP

#include <eepp/graphics/scrollparallax.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uitextview.hpp>

namespace EE { namespace UI {

class UIProgressBarFiller;

class EE_API UIProgressBar : public UIWidget {
  public:
	class StyleConfig {
	  public:
		bool DisplayPercent = false;
		bool VerticalExpand = true;
		Vector2f MovementSpeed = Vector2f( 0.f, 0 );
	};

	static UIProgressBar* New();

	UIProgressBar();

	virtual ~UIProgressBar();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual void setProgress( Float Val );

	const Float& getProgress() const;

	virtual void setTotalSteps( const Float& Steps );

	const Float& getTotalSteps() const;

	virtual void scheduledUpdate( const Time& time );

	void setMovementSpeed( const Vector2f& Speed );

	const Vector2f& getMovementSpeed() const;

	void setVerticalExpand( const bool& verticalExpand );

	const bool& getVerticalExpand() const;

	void setDisplayPercent( const bool& displayPercent );

	const bool& getDisplayPercent() const;

	UITextView* getTextBox() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	const StyleConfig& getStyleConfig() const;

  protected:
	friend class UIProgressBarFiller;
	StyleConfig mStyleConfig;
	Float mProgress;
	Float mTotalSteps;
	UITextView* mTextBox;
	Vector2f mOffset;
	UIProgressBarFiller* mFiller;

	virtual Uint32 onValueChange();

	virtual void onSizeChange();

	virtual void onPaddingChange();

	virtual void onThemeLoaded();

	void updateTextBox();

	virtual void onAlphaChange();
};

}} // namespace EE::UI

#endif
