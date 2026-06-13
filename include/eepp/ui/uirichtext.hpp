#ifndef EE_UI_UIRICHTEXT_HPP
#define EE_UI_UIRICHTEXT_HPP

#include <eepp/graphics/richtext.hpp>
#include <eepp/graphics/texttransform.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UIRichText : public UIHTMLWidget {
  public:
	enum class IntrinsicMode { None, Min, Max };

	enum class WhiteSpaceCollapse {
		Collapse,
		Preserve,
		PreserveBreaks,
		PreserveSpaces,
		BreakSpaces,
		Discard
	};

	static WhiteSpaceCollapse toWhiteSpaceCollapse( std::string_view val );

	static std::string fromWhiteSpaceCollapse( WhiteSpaceCollapse val );

	static std::string fromWhiteSpace( WhiteSpaceCollapse collapse, bool lineWrap );

	static String collapseInternalWhitespace( const String& s );

	static void rebuildRichText( UILayout* container, RichText& richText,
								 IntrinsicMode mode = IntrinsicMode::None );

	static void setUseCodeEditorForPreCodeBlocks( bool enabled );

	static bool getUseCodeEditorForPreCodeBlocks();

	static UIRichText* New();

	static UIRichText* NewWithTag( const std::string& tag );

	static UIRichText* NewParagraph() { return UIRichText::NewWithTag( "p" ); };

	static UIRichText* NewH1() { return UIRichText::NewWithTag( "h1" ); };

	static UIRichText* NewH2() { return UIRichText::NewWithTag( "h2" ); };

	static UIRichText* NewH3() { return UIRichText::NewWithTag( "h3" ); };

	static UIRichText* NewH4() { return UIRichText::NewWithTag( "h4" ); };

	static UIRichText* NewH5() { return UIRichText::NewWithTag( "h5" ); };

	static UIRichText* NewH6() { return UIRichText::NewWithTag( "h6" ); };

	static UIRichText* NewBr();

	static UIRichText* NewHr();

	static UIRichText* NewHtml();

	static UIRichText* NewBody();

	static UIRichText* NewDiv() { return UIRichText::NewWithTag( "div" ); };

	static UIRichText* NewPre() { return UIRichText::NewWithTag( "pre" ); };

	static UIRichText* NewBlockquote() { return UIRichText::NewWithTag( "blockquote" ); };

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual Float getMinIntrinsicWidth() const;

	virtual Float getMaxIntrinsicWidth() const;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	const Graphics::RichText& getRichText();

	Graphics::Font* getFont() const;

	UIRichText* setFont( Graphics::Font* font );

	Uint32 getFontSize() const;

	UIRichText* setFontSize( const Uint32& characterSize );

	const Uint32& getFontStyle() const;

	Uint32 getTextDecoration() const;

	UIRichText* setTextDecoration( const Uint32& textDecoration );

	UIRichText* setFontStyle( const Uint32& fontStyle );

	FontWeight getFontWeight() const;

	UIRichText* setFontWeight( const FontWeight& weight );

	const Color& getFontColor() const;

	UIRichText* setFontColor( const Color& color );

	const Color& getFontBackgroundColor() const;

	UIRichText* setFontBackgroundColor( const Color& color );

	const Color& getFontShadowColor() const;

	UIRichText* setFontShadowColor( const Color& color );

	const Vector2f& getFontShadowOffset() const;

	UIRichText* setFontShadowOffset( const Vector2f& offset );

	const Float& getOutlineThickness() const;

	UIRichText* setOutlineThickness( const Float& outlineThickness );

	const Color& getOutlineColor() const;

	UIRichText* setOutlineColor( const Color& outlineColor );

	Uint32 getTextAlign() const;

	UIRichText* setTextAlign( const Uint32& align );

	WhiteSpaceCollapse getWhiteSpaceCollapse() const;

	void setWhiteSpaceCollapse( WhiteSpaceCollapse collapse );

	bool getLineWrap() const;

	void setLineWrap( bool lineWrap );

	void applyWhiteSpace( std::string_view val );

	Float getLineHeightPx() const;

	UIRichText* setLineHeightEq( const std::string& eq );

	Float getTextIndentPx() const;

	UIRichText* setTextIndentEq( const std::string& eq );

	Uint32 getTabSize() const;

	UIRichText* setTabSize( Uint32 tabSize );

	bool isTextSelectionEnabled() const;

	void setTextSelectionEnabled( bool active );

	const TextTransform::Value& getTextTransform() const;

	void setTextTransform( const TextTransform::Value& textTransform );

	const Color& getSelectionBackColor() const;

	void setSelectionBackColor( const Color& color );

	const Color& getSelectionColor() const;

	void setSelectionColor( const Color& color );

	std::pair<Int64, Int64> getTextSelectionRange() const;

	void setTextSelectionRange( TextSelectionRange range );

	String getSelectionString() const;

	virtual RichText* getRichTextPtr() { return &mRichText; }

  protected:
	RichText mRichText;
	Int64 mSelCurInit{ 0 };
	Int64 mSelCurEnd{ 0 };
	bool mSelecting{ false };
	std::string mLineHeightEq;
	std::string mTextIndentEq;
	mutable Float mLineHeightPxCache{ 0 };
	mutable bool mLineHeightPxDirty{ true };
	mutable Float mTextIndentPxCache{ 0 };
	mutable bool mTextIndentPxDirty{ true };
	Uint32 mTabSize{ 8 };
	WhiteSpaceCollapse mWhiteSpaceCollapse{ WhiteSpaceCollapse::Collapse };
	bool mLineWrap{ true };
	TextTransform::Value mTextTransform{ TextTransform::None };

	LayoutInvalidationFlags mDeferredLayoutReasons{ 0 };

	explicit UIRichText( const std::string& tag = "richtext" );

	virtual Uint32 onMessage( const NodeMessage* Msg );
	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );
	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );
	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );
	virtual Uint32 onFocusLoss();

	virtual void onSizeChange();
	virtual void onPaddingChange();
	virtual void onChildCountChange( Node* child, const bool& removed );
	virtual void onFontChanged();
	virtual void onFontStyleChanged();
	virtual void onSelectionChange();
	virtual void onLayoutUpdate();

	void selCurInit( const Int64& init );
	void selCurEnd( const Int64& end );
	Int64 selCurInit() const { return mSelCurInit; }
	Int64 selCurEnd() const { return mSelCurEnd; }

	void rebuildRichText( RichText& richText, IntrinsicMode mode = IntrinsicMode::None );
	void updateDefaultSpansStyle();
};

class EE_API UIHTMLHtml : public UIRichText {
  public:
	static UIHTMLHtml* New( const std::string& tag );
	virtual Uint32 getType() const;
	bool isType( const Uint32& type ) const;
	bool applyProperty( const StyleSheetProperty& attribute );

  protected:
	UIHTMLHtml( const std::string& tag = "html" );
};

class EE_API UIHTMLBody : public UIRichText {
  public:
	static UIHTMLBody* New( const std::string& tag );
	virtual Uint32 getType() const;
	bool isType( const Uint32& type ) const;
	bool applyProperty( const StyleSheetProperty& attribute );
	virtual void updateLayout();
	void setDocumentViewportMinHeight( const Float& height );

  protected:
	bool mPropagatedBackground{ false };
	StyleSheetLength mMinHeightLocal;
	bool mSettingBodyHeight{ false };
	Float mDocumentViewportMinHeight{ 0 };
	Float mDocumentContentMinHeight{ 0 };

	UIHTMLBody( const std::string& tag = "body" );

	Float getLocalMinHeight() const;
	void setDocumentContentMinHeight( const Float& height );
	void updateDocumentMinHeight();
	void updateDocumentContentMinHeightFromChildren();
	virtual Uint32 onMessage( const NodeMessage* Msg );
};

class EE_API UIHTMLHead : public UIWidget {
  public:
	static UIHTMLHead* New();
	virtual Uint32 getType() const;
	bool isType( const Uint32& type ) const;

  protected:
	UIHTMLHead();
};

class EE_API UILineBreak : public UIRichText {
  public:
	static UILineBreak* New( const std::string& tag );

	virtual Uint32 getType() const;

	bool isType( const Uint32& type ) const;

  protected:
	UILineBreak( const std::string& tag = "br" );
};

}} // namespace EE::UI

#endif
