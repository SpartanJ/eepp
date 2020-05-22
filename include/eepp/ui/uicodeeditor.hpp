#ifndef EE_UI_UICODEEDIT_HPP
#define EE_UI_UICODEEDIT_HPP

#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/uifontstyleconfig.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::Graphics;
using namespace EE::UI::Doc;

namespace EE { namespace Graphics {
class Font;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class EE_API UICodeEditor : public UIWidget, public TextDocument::Client {
  public:
	static UICodeEditor* New();

	UICodeEditor();

	virtual ~UICodeEditor();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual void draw();

	virtual void scheduledUpdate( const Time& time );

	void reset();

	void loadFromFile( const std::string& path );

	Font* getFont() const;

	const UIFontStyleConfig& getFontStyleConfig() const;

	UICodeEditor* setFont( Font* font );

	UICodeEditor* setFontSize( Float dpSize );

	UICodeEditor* setFontColor( const Color& color );

	UICodeEditor* setFontSelectedColor( const Color& color );

	UICodeEditor* setFontSelectionBackColor( const Color& color );

	const Uint32& getTabWidth() const;

	UICodeEditor* setTabWidth( const Uint32& tabWidth );

	const Float& getMouseWheelScroll() const;

	void setMouseWheelScroll( const Float& mouseWheelScroll );

  protected:
	Font* mFont;
	UIFontStyleConfig mFontStyleConfig;
	Doc::TextDocument mDoc;
	Vector2f mScrollPos;
	Clock mBlinkTimer;
	bool mDirtyEditor;
	bool mCursorVisible;
	bool mMouseDown;
	Uint32 mTabWidth;
	Int64 mLastColOffset;
	Vector2f mScroll;
	Float mMouseWheelScroll;

	void invalidateEditor();

	virtual Uint32 onFocusLoss();

	virtual Uint32 onTextInput( const TextInputEvent& event );

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseMove( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	virtual void onSizeChange();

	virtual void onPaddingChange();

	void updateEditor();

	void onDocumentTextChanged();

	void onDocumentCursorChange( const TextPosition& );

	void onDocumentSelectionChange( const TextRange& );

	std::pair<int, int> getVisibleLineRange();

	int getVisibleLinesCount();

	void scrollToMakeVisible( const TextPosition& position );

	Float getXOffsetCol( const TextPosition& position ) const;

	Int64 getColFromXOffset( Int64 line, const Float& offset ) const;

	Float getLineHeight() const;

	Float getCharacterSize() const;

	Float getGlyphWidth() const;

	void updateLastColumnOffset();

	void resetCursor();

	TextPosition resolveScreenPosition( const Vector2f& position ) const;

	Vector2f getViewPortLineCount() const;

	Sizef getMaxScroll() const;
};

}} // namespace EE::UI

#endif // EE_UI_UICODEEDIT_HPP
