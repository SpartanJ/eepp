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

				inline CreateParams(
					cUIControl * parentCtrl,
					const eeVector2i& pos = eeVector2i( 0, 0 ),
					const eeSize& size = eeSize( -1, -1 ),
					const Uint32& flags = UI_HALIGN_LEFT | UI_VALIGN_CENTER,
					const EE_PRE_BLEND_FUNC& blend = ALPHA_NORMAL,
					const cUIBackground& Back = cUIBackground(),
					const cUIBorder& Bord = cUIBorder(),
					const eeSize& MinCtrlSize = eeSize(0,0)
				) :
					cUIControlAnim::CreateParams( parentCtrl, pos, size, flags, blend, Back, Bord ),
					MinControlSize( MinCtrlSize )
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

		void CalcDistToBorder();
};

}}

#endif
