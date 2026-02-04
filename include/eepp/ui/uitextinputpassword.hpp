#ifndef EE_UICUITEXTINPUTPASSWORD_HPP
#define EE_UICUITEXTINPUTPASSWORD_HPP

#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

class EE_API UITextInputPassword : public UITextInput {
  public:
	static UITextInputPassword* New();

	virtual ~UITextInputPassword();

	virtual void draw();

	virtual const String& getText() const;

	virtual UITextView* setText( const String& text );

	const Text& getPassCache() const;

	const String& getBulletCharacter() const;

	void setBulletCharacter( const String& bulletCharacter );

  protected:
	UITextInputPassword();

	Text mPassCache;
	Vector2f mHintAlignOffset;
	String mBulletCharacter;

	void updateText();

	void updatePass( const String& pass );

	void updateFontStyleConfig();

	virtual void onStateChange();

	virtual void onFontChanged();

	virtual void onFontStyleChanged();

	virtual Text& getVisibleTextCache();
};

}} // namespace EE::UI

#endif
