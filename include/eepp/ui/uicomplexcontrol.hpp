#ifndef EE_UICUICOMPLEXCONTROL_HPP
#define EE_UICUICOMPLEXCONTROL_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uitooltip.hpp>

namespace EE { namespace UI {

class EE_API UIComplexControl : public UIControlAnim {
	public:
		static UIComplexControl * New();

		UIComplexControl();

		virtual ~UIComplexControl();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void update();

		virtual UIControl * setSize( const Sizei& size );

		virtual UIControl * setFlags( const Uint32& flags );

		virtual UIControl * unsetFlags( const Uint32& flags );

		virtual UIComplexControl * setAnchors( const Uint32& flags );

		UIControl * setSize( const Int32& Width, const Int32& Height );

		const Sizei& getSize();

		UITooltip * getTooltip();

		void tooltipRemove();

		void setTooltipText( const String& Text );

		String getTooltipText();

		void updateAnchorsDistances();
	protected:
		UITooltip *	mTooltip;
		Sizei		mMinControlSize;
		Recti		mDistToBorder;

		void createTooltip();

		virtual void onParentSizeChange( const Vector2i& SizeChange );

		virtual void onPositionChange();

		virtual void onAutoSize();
};

}}

#endif
