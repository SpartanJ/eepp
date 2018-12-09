#ifndef EE_UIUIWIDGET_HPP
#define EE_UIUIWIDGET_HPP

#include <eepp/scene/nodeattribute.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uitooltip.hpp>

namespace pugi {
class xml_node;
}

namespace EE { namespace UI {

class EE_API UIWidget : public UINode {
	public:
		static UIWidget * New();

		UIWidget();

		virtual ~UIWidget();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void update( const Time& time );

		virtual Node * setSize( const Sizef& size );

		virtual UINode * setFlags( const Uint32& flags );

		virtual UINode * unsetFlags( const Uint32& flags );

		virtual UIWidget * setAnchors( const Uint32& flags );

		virtual void setTheme( UITheme * Theme );

		virtual UINode * setThemeSkin( const std::string& skinName );

		virtual UINode * setThemeSkin( UITheme * Theme, const std::string& skinName );

		virtual Node * setSize( const Float& Width, const Float& Height );

		const Sizef& getSize();

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

		void setAttribute( const std::string& name, const std::string& value );

		virtual void setAttribute( const NodeAttribute& attribute );

		const Rectf& getPadding() const;

		UIWidget * setPadding(const Rectf& padding);
	protected:
		friend class UIManager;
		friend class UISceneNode;

		UITheme *	mTheme;
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

		void createTooltip();

		virtual void onParentSizeChange( const Vector2f& SizeChange );

		virtual void onPositionChange();

		virtual void onVisibilityChange();

		virtual void onAutoSize();

		virtual void onWidgetCreated();

		virtual void onPaddingChange();

		void beginAttributesTransaction();

		void endAttributesTransaction();

		void updateAnchors( const Vector2f & SizeChange );

		void alignAgainstLayout();
};

}}

#endif
