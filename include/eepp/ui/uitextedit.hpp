#ifndef EE_UICUITEXTEDIT_HPP
#define EE_UICUITEXTEDIT_HPP

#include <eepp/graphics/text.hpp>
#include <eepp/ui/uicodeeditor.hpp>

namespace EE { namespace UI {

class EE_API UITextEdit : public UICodeEditor {
  public:
	static UITextEdit* New();

	UITextEdit();

	virtual ~UITextEdit();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	String getText() const;

	void setText( const String& text );

	void setWordWrap( bool enabled );

  protected:
	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual void drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							 const TextPosition& cursor );
};

}} // namespace EE::UI

#endif
