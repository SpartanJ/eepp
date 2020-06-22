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

	virtual void shrinkText( const Float& maxWidth );

	String getText() const;

	void setText( const String& text );

  protected:
	struct TextLine {
		Text text;
		String::HashType hash{0};
	};
	std::map<size_t, TextLine> mLines;
	bool mIsMonoSpace;

	virtual void onFontChanged();

	virtual void onDocumentLineChanged( const Int64& lineIndex );

	virtual void drawLineText( const Int64& index, Vector2f position, const Float& fontSize );

	virtual Int64 getColFromXOffset( Int64 line, const Float& x ) const;

	virtual Float getColXOffset( TextPosition position );

	virtual Float getXOffsetCol( const TextPosition& position );

	virtual Float getLineWidth( const Int64& lineIndex );

	void ensureLineUpdated( const Int64& lineIndex );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual void drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							 const TextPosition& cursor );

	virtual void onDocumentChanged();

	void invalidateLinesCache();

	void updateLineCache( const Int64& lineIndex );

};

}} // namespace EE::UI

#endif
