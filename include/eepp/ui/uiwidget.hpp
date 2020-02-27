#ifndef EE_UIUIWIDGET_HPP
#define EE_UIUIWIDGET_HPP

#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/uinode.hpp>

namespace pugi {
class xml_node;
}

namespace EE { namespace UI { namespace CSS {
class PropertyDefinition;
}}} // namespace EE::UI::CSS

using namespace EE::UI::CSS;

namespace EE { namespace UI {

class UITooltip;
class UIStyle;

class EE_API UIWidget : public UINode {
  public:
	static UIWidget* New();

	static UIWidget* NewWithTag( const std::string& tag );

	UIWidget();

	virtual ~UIWidget();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual Node* setSize( const Sizef& size );

	virtual UINode* setFlags( const Uint32& flags );

	virtual UINode* unsetFlags( const Uint32& flags );

	virtual UIWidget* setAnchors( const Uint32& flags );

	virtual void setTheme( UITheme* Theme );

	virtual UINode* setThemeSkin( const std::string& skinName );

	virtual UINode* setThemeSkin( UITheme* Theme, const std::string& skinName );

	virtual Node* setSize( const Float& Width, const Float& Height );

	virtual Node* setId( const std::string& id );

	const Sizef& getSize() const;

	UITooltip* getTooltip();

	void tooltipRemove();

	UIWidget* setTooltipText( const String& Text );

	String getTooltipText();

	void updateAnchorsDistances();

	Rect getLayoutMargin() const;

	UIWidget* setLayoutMargin( const Rect& margin );

	UIWidget* setLayoutMarginLeft( const Float& marginLeft );

	UIWidget* setLayoutMarginRight( const Float& marginRight );

	UIWidget* setLayoutMarginTop( const Float& marginTop );

	UIWidget* setLayoutMarginBottom( const Float& marginBottom );

	Float getLayoutWeight() const;

	UIWidget* setLayoutWeight( const Float& weight );

	Uint32 getLayoutGravity() const;

	UIWidget* setLayoutGravity( const Uint32& layoutGravity );

	LayoutSizeRule getLayoutWidthRule() const;

	UIWidget* setLayoutWidthRule( const LayoutSizeRule& layoutWidthRules );

	LayoutSizeRule getLayoutHeightRule() const;

	UIWidget* setLayoutHeightRule( const LayoutSizeRule& layoutHeightRules );

	UIWidget* setLayoutSizeRules( const LayoutSizeRule& layoutWidthRules,
								  const LayoutSizeRule& layoutHeightRules );

	UIWidget* setLayoutPositionRule( const LayoutPositionRule& layoutPositionRule, UIWidget* of );

	UIWidget* getLayoutPositionRuleWidget() const;

	LayoutPositionRule getLayoutPositionRule() const;

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	void notifyLayoutAttrChange();

	void notifyLayoutAttrChangeParent();

	void setStyleSheetInlineProperty( const std::string& name, const std::string& value,
									  const Uint32& specificity = UINT32_MAX -
																  1 /*SpecificityInline*/ );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	const Rectf& getPadding() const;

	UIWidget* setPadding( const Rectf& padding );

	UIWidget* setPaddingLeft( const Float& paddingLeft );

	UIWidget* setPaddingRight( const Float& paddingRight );

	UIWidget* setPaddingTop( const Float& paddingTop );

	UIWidget* setPaddingBottom( const Float& paddingBottom );

	const std::string& getStyleSheetTag() const;

	const std::string& getStyleSheetId() const;

	const std::vector<std::string>& getStyleSheetClasses() const;

	UIWidget* getStyleSheetParentElement() const;

	UIWidget* getStyleSheetPreviousSiblingElement() const;

	UIWidget* getStyleSheetNextSiblingElement() const;

	const std::vector<std::string>& getStyleSheetPseudoClasses() const;

	void addClass( const std::string& cls );

	void addClasses( const std::vector<std::string>& classes );

	void removeClass( const std::string& cls );

	void removeClasses( const std::vector<std::string>& classes );

	bool hasClass( const std::string& cls ) const;

	void setElementTag( const std::string& tag );

	const std::string& getElementTag() const;

	virtual void pushState( const Uint32& State, bool emitEvent = true );

	virtual void popState( const Uint32& State, bool emitEvent = true );

	UIStyle* getUIStyle() const;

	void reloadStyle( const bool& reloadChilds = true );

	void beginAttributesTransaction();

	void endAttributesTransaction();

	const Uint32& getStyleState() const;

	const Uint32& getStylePreviousState() const;

	std::vector<UIWidget*> findAllByClass( const std::string& className );

	std::vector<UIWidget*> findAllByTag( const std::string& tag );

	UIWidget* findByClass( const std::string& className );

	template <typename T> T* findByClass( const std::string& className ) {
		return reinterpret_cast<T*>( findByClass( className ) );
	}

	UIWidget* findByTag( const std::string& tag );

	template <typename T> T* findByTag( const std::string& tag ) {
		return reinterpret_cast<T*>( findByTag( tag ) );
	}

	UIWidget* querySelector( const CSS::StyleSheetSelector& selector );

	UIWidget* querySelector( const std::string& selector );

	template <typename T> T* querySelector( const std::string& selector ) {
		return reinterpret_cast<T*>( querySelector( selector ) );
	}

	std::vector<UIWidget*> querySelectorAll( const CSS::StyleSheetSelector& selector );

	std::vector<UIWidget*> querySelectorAll( const std::string& selector );

	std::string getPropertyString( const std::string& property );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

	bool isSceneNodeLoading() const;

	Float
	getPropertyRelativeTargetContainerLength( const CSS::PropertyRelativeTarget& relativeTarget,
											  const Float& defaultValue = 0,
											  const Uint32& propertyIndex = 0 );

	Float lengthFromValue( const std::string& value,
						   const CSS::PropertyRelativeTarget& relativeTarget,
						   const Float& defaultValue = 0, const Float& defaultContainerValue = 0,
						   const Uint32& propertyIndex = 0 );

	Float lengthFromValue( const StyleSheetProperty& property, const Float& defaultValue = 0,
						   const Float& defaultContainerValue = 0 );

	Float lengthFromValueAsDp( const std::string& value,
							   const CSS::PropertyRelativeTarget& relativeTarget,
							   const Float& defaultValue = 0,
							   const Float& defaultContainerValue = 0,
							   const Uint32& propertyIndex = 0 );

	Float lengthFromValueAsDp( const StyleSheetProperty& property, const Float& defaultValue = 0,
							   const Float& defaultContainerValue = 0 );

  protected:
	friend class UIManager;
	friend class UISceneNode;

	std::string mTag;
	UITheme* mTheme;
	UIStyle* mStyle;
	UITooltip* mTooltip;
	Sizef mMinControlSize;
	Rect mDistToBorder;
	Rect mLayoutMargin;
	Rectf mPadding;
	Rectf mRealPadding;
	Float mLayoutWeight;
	Uint32 mLayoutGravity;
	LayoutSizeRule mLayoutWidthRule;
	LayoutSizeRule mLayoutHeightRule;
	LayoutPositionRule mLayoutPositionRule;
	UIWidget* mLayoutPositionRuleWidget;
	int mAttributesTransactionCount;
	std::string mSkinName;
	std::vector<std::string> mClasses;
	std::vector<std::string> mPseudoClasses;

	explicit UIWidget( const std::string& tag );

	void updatePseudoClasses();

	void createTooltip();

	virtual Uint32 onMouseMove( const Vector2i& Pos, const Uint32& Flags );

	virtual Uint32 onMouseLeave( const Vector2i& Pos, const Uint32& Flags );

	virtual void onParentSizeChange( const Vector2f& SizeChange );

	virtual void onPositionChange();

	virtual void onVisibilityChange();

	virtual void onSizeChange();

	virtual void onAutoSize();

	virtual void onWidgetCreated();

	virtual void onPaddingChange();

	virtual void onThemeLoaded();

	virtual void onParentChange();

	void updateAnchors( const Vector2f& SizeChange );

	void alignAgainstLayout();

	void reportStyleStateChange();

	std::string getLayoutWidthRulesString() const;

	std::string getLayoutHeightRulesString() const;

	std::string getLayoutGravityString() const;

	std::string getGravityString() const;

	std::string getFlagsString() const;

	bool checkPropertyDefinition( const StyleSheetProperty& property );
};

}} // namespace EE::UI

#endif
