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
					cUIControlAnim::CreateParams()
				{
				}

				inline ~CreateParams() {}

				String TooltipText;
		};

		cUIComplexControl( const cUIComplexControl::CreateParams& Params );

		~cUIComplexControl();

		virtual void Update();

		cUITooltip * Tooltip();

		void TooltipRemove();

		void TooltipText( const String& Text );

		String TooltipText();
	protected:
		cUITooltip *	mTooltip;

		void CreateTooltip();
};

}}

#endif
