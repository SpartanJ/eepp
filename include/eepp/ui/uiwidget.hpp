#ifndef EE_UIUIWIDGET_HPP
#define EE_UIUIWIDGET_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uitooltip.hpp>

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

		UIControl * setSize( const Int32& Width, const Int32& Height );

		const Sizei& getSize();

		UITooltip * getTooltip();

		void tooltipRemove();

		UIWidget * setTooltipText( const String& Text );

		String getTooltipText();

		void updateAnchorsDistances();

		Recti getLayoutMargin() const;

		UIWidget * setLayoutMargin(const Recti & margin);

		Float getLayoutWeight() const;

		UIWidget * setLayoutWeight(const Float & weight);

		Uint32 getLayoutGravity() const;

		UIWidget * setLayoutGravity(const Uint32 & layoutGravity);

		LayoutSizeRules getLayoutWidthRules() const;

		UIWidget * setLayoutWidthRules(const LayoutSizeRules & layoutWidthRules);

		LayoutSizeRules getLayoutHeightRules() const;

		UIWidget * setLayoutHeightRules(const LayoutSizeRules & layoutHeightRules);

		UIWidget * setLayoutSizeRules( const LayoutSizeRules & layoutWidthRules, const LayoutSizeRules & layoutHeightRules );
	protected:
		friend class UILinearLayout;

		UITooltip *	mTooltip;
		Sizei		mMinControlSize;
		Recti		mDistToBorder;
		Recti		mLayoutMargin;
		Recti		mRealMargin;
		Float		mLayoutWeight;
		Uint32		mLayoutGravity;
		LayoutSizeRules mLayoutWidthRules;
		LayoutSizeRules mLayoutHeightRules;

		void createTooltip();

		virtual void onParentSizeChange( const Vector2i& SizeChange );

		virtual void onPositionChange();

		virtual void onAutoSize();

		void updateAnchors( const Vector2i & SizeChange );

		void alignAgainstLayout();
};

}}

#endif
