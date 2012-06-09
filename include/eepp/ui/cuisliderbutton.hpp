#ifndef EE_UICUISLIDERBUTTON_HPP
#define EE_UICUISLIDERBUTTON_HPP

#include <eepp/ui/cuicontrolanim.hpp>

namespace EE { namespace UI { namespace Private {

class EE_API cUISliderButton : public cUIControlAnim {
	public:
		cUISliderButton( const cUIControlAnim::CreateParams& Params );

		virtual ~cUISliderButton();
	protected:
		virtual void OnPosChange();
};

}}}

#endif
