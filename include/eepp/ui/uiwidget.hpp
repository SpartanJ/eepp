#ifndef EE_UIUIWIDGET_HPP
#define EE_UIUIWIDGET_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uitooltip.hpp>

namespace pugi {
class xml_node;
}

namespace EE { namespace UI {

class EE_API UIWidget : public UIControlAnim {
	public:
		static UIWidget * New();

		UIWidget();

		virtual ~UIWidget();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void update();

		virtual UIControl * setSize( const Sizei& size );

		virtual UIControl * setFlags( const Uint32& flags );

		virtual UIControl * unsetFlags( const Uint32& flags );

		virtual UIWidget * setAnchors( const Uint32& flags );

		virtual void setTheme( UITheme * Theme );

		virtual UIControl * setThemeSkin( const std::string& skinName );

		virtual UIControl * setThemeSkin( UITheme * Theme, const std::string& skinName );

		UIControl * setSize( const Int32& Width, const Int32& Height );

		const Sizei& getSize();

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
	protected:
		friend class UIManager;

		UITheme *	mTheme;
		UITooltip *	mTooltip;
		Sizei		mMinControlSize;
		Rect		mDistToBorder;
		Rect		mLayoutMargin;
		Rect		mRealMargin;
		Float		mLayoutWeight;
		Uint32		mLayoutGravity;
		LayoutSizeRules mLayoutWidthRules;
		LayoutSizeRules mLayoutHeightRules;
		LayoutPositionRules mLayoutPositionRule;
		UIWidget * mLayoutPositionRuleWidget;
		int	mPropertiesTransactionCount;

		void createTooltip();

		virtual void onParentSizeChange( const Vector2i& SizeChange );

		virtual void onPositionChange();

		virtual void onVisibilityChange();

		virtual void onAutoSize();

		virtual void onWidgetCreated();

		void beginPropertiesTransaction();

		void endPropertiesTransaction();

		void updateAnchors( const Vector2i & SizeChange );

		void alignAgainstLayout();
};

}}

#endif
