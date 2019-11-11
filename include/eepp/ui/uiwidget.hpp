#ifndef EE_UIUIWIDGET_HPP
#define EE_UIUIWIDGET_HPP

#include <eepp/scene/nodeattribute.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/css/stylesheetelement.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>

namespace pugi {
class xml_node;
}

namespace EE { namespace UI {

class UITooltip;
class UIStyle;

class EE_API UIWidget : public UINode, public CSS::StyleSheetElement {
	public:
		static UIWidget * New();

		static UIWidget * NewWithTag( const std::string& tag );

		UIWidget();

		virtual ~UIWidget();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual Node * setSize( const Sizef& size );

		virtual UINode * setFlags( const Uint32& flags );

		virtual UINode * unsetFlags( const Uint32& flags );

		virtual UIWidget * setAnchors( const Uint32& flags );

		virtual void setTheme( UITheme * Theme );

		virtual UINode * setThemeSkin( const std::string& skinName );

		virtual UINode * setThemeSkin( UITheme * Theme, const std::string& skinName );

		virtual Node * setSize( const Float& Width, const Float& Height );

		virtual Node * setId( const std::string & id );

		const Sizef& getSize() const;

		UITooltip * getTooltip();

		void tooltipRemove();

		UIWidget * setTooltipText( const String& Text );

		String getTooltipText();

		void updateAnchorsDistances();

		Rect getLayoutMargin() const;

		UIWidget * setLayoutMargin(const Rect & margin);

		Float getLayoutWeight() const;

		UIWidget * setLayoutWeight(const Float & weight);

		Uint32 getLayoutGravity() const;

		UIWidget * setLayoutGravity(const Uint32 & layoutGravity);

		LayoutSizeRules getLayoutWidthRules() const;

		UIWidget * setLayoutWidthRules(const LayoutSizeRules & layoutWidthRules);

		LayoutSizeRules getLayoutHeightRules() const;

		UIWidget * setLayoutHeightRules(const LayoutSizeRules & layoutHeightRules);

		UIWidget * setLayoutSizeRules( const LayoutSizeRules & layoutWidthRules, const LayoutSizeRules & layoutHeightRules );

		UIWidget * setLayoutPositionRule( const LayoutPositionRules& layoutPositionRule, UIWidget * of );

		UIWidget * getLayoutPositionRuleWidget() const;

		LayoutPositionRules getLayoutPositionRule() const;

		virtual void loadFromXmlNode( const pugi::xml_node& node );

		void notifyLayoutAttrChange();

		void notifyLayoutAttrChangeParent();

		void setStyleSheetProperty( const std::string& name, const std::string& value, const Uint32& specificity = UINT32_MAX - 1/*SpecificityInline*/ );

		virtual bool setAttribute( const NodeAttribute& attribute, const Uint32& state = UIState::StateFlagNormal );

		const Rectf& getPadding() const;

		UIWidget * setPadding(const Rectf& padding);

		const std::string& getStyleSheetTag() const;

		const std::string& getStyleSheetId() const;

		const std::vector<std::string>& getStyleSheetClasses() const;

		StyleSheetElement * getStyleSheetParentElement() const;

		StyleSheetElement * getStyleSheetPreviousSiblingElement() const;

		StyleSheetElement * getStyleSheetNextSiblingElement() const;

		const std::vector<std::string>& getStyleSheetPseudoClasses() const;

		void addClass( const std::string& cls );

		void addClasses( const std::vector<std::string>& classes );

		void removeClass( const std::string& cls );

		bool containsClass( const std::string& cls ) const;

		void setElementTag( const std::string& tag );

		const std::string& getElementTag() const;

		virtual void pushState( const Uint32& State, bool emitEvent = true );

		virtual void popState( const Uint32& State, bool emitEvent = true );

		UIStyle * getUIStyle() const;

		void reloadStyle( const bool& reloadChilds = true );

		void beginAttributesTransaction();

		void endAttributesTransaction();

		const Uint32& getStyleState() const;

		const Uint32& getStylePreviousState() const;

		std::vector<UIWidget*> findAllByClass( const std::string& className );

		std::vector<UIWidget*> findAllByTag( const std::string& tag );

		UIWidget * findByClass( const std::string& className );

		template<typename T>
		T * findByClass( const std::string& className ) {
			return reinterpret_cast<T*>( findByClass( className ) );
		}

		UIWidget * findByTag( const std::string& tag );

		template<typename T>
		T * findByTag( const std::string& tag ) {
			return reinterpret_cast<T*>( findByTag( tag ) );
		}

		UIWidget * querySelector( const std::string& selector );

		template<typename T>
		T * querySelector( const std::string& selector ) {
			return reinterpret_cast<T*>( querySelector( selector ) );
		}

		std::vector<UIWidget*> querySelectorAll( const std::string& selector );
	protected:
		friend class UIManager;
		friend class UISceneNode;

		std::string mTag;
		UITheme *	mTheme;
		UIStyle *	mStyle;
		UITooltip *	mTooltip;
		Sizef		mMinControlSize;
		Rect		mDistToBorder;
		Rect		mLayoutMargin;
		Rectf		mPadding;
		Rectf		mRealPadding;
		Float		mLayoutWeight;
		Uint32		mLayoutGravity;
		LayoutSizeRules mLayoutWidthRules;
		LayoutSizeRules mLayoutHeightRules;
		LayoutPositionRules mLayoutPositionRule;
		UIWidget * mLayoutPositionRuleWidget;
		int	mAttributesTransactionCount;
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

		void updateAnchors( const Vector2f & SizeChange );

		void alignAgainstLayout();

		void reportStyleStateChange();

		bool isSceneNodeLoading() const;

		std::string getLayoutWidthRulesString() const;

		std::string getLayoutHeightRulesString() const;

		std::string getLayoutGravityString() const;

		std::string getGravityString() const;

		std::string getFlagsString() const;

		bool drawablePropertySet( const std::string& propertyName, const std::string& value, std::function<void(Drawable * drawable, bool ownIt)> funcSet);

		UIWidget * querySelector( const CSS::StyleSheetSelector& selector );

		std::vector<UIWidget*> querySelectorAll( const CSS::StyleSheetSelector& selector );
};

}}

#endif
