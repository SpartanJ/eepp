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

	virtual void wrapText( const Float& maxWidth );

	String getText() const;

	void setText( const String& text );

  protected:
	virtual void onDocumentLineChanged( const Int64& lineIndex );

	virtual void drawLineText( const Int64& index, Vector2f position, const Float& fontSize,
							   const Float& lineHeight );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual void drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							 const TextPosition& cursor );
};

}} // namespace EE::UI

#endif
