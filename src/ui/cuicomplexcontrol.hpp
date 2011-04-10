#ifndef EE_UICUICOMPLEXCONTROL_HPP
#define EE_UICUICOMPLEXCONTROL_HPP

#include "cuicontrolanim.hpp"
#include "cuitooltip.hpp"

namespace EE { namespace UI {

class EE_API cUIComplexControl : public cUIControlAnim {
	public:
		class CreateParams : public cUIControlAnim::CreateParams {
			public:
				inline CreateParams() :
					cUIControlAnim::CreateParams(),
					MinControlSize( 0, 0 )
				{
				}

				inline ~CreateParams() {}

				String	TooltipText;
				eeSize	MinControlSize;
		};

		cUIComplexControl( const cUIComplexControl::CreateParams& Params );

		~cUIComplexControl();

		virtual void Update();

		virtual void Size( const eeSize &Size );

		void Size( const Int32& Width, const Int32& Height );

		const eeSize& Size();

		cUITooltip * Tooltip();

		void TooltipRemove();

		void TooltipText( const String& Text );

		String TooltipText();
	protected:
		cUITooltip *	mTooltip;
		eeSize			mMinControlSize;
		eeVector2i		mDistToBorder;

		void CreateTooltip();

		virtual void OnParentSizeChange();
};

}}

#endif
