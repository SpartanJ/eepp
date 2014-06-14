#ifndef EE_UICUICOMPLEXCONTROL_HPP
#define EE_UICUICOMPLEXCONTROL_HPP

#include <eepp/ui/cuicontrolanim.hpp>
#include <eepp/ui/cuitooltip.hpp>

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
					const Vector2i& pos = Vector2i( 0, 0 ),
					const Sizei& size = Sizei( -1, -1 ),
					const Uint32& flags = UI_CONTROL_DEFAULT_FLAGS,
					const EE_BLEND_MODE& blend = ALPHA_NORMAL,
					const cUIBackground& Back = cUIBackground(),
					const cUIBorder& Bord = cUIBorder(),
					const Sizei& MinCtrlSize = Sizei(0,0)
				) :
					cUIControlAnim::CreateParams( parentCtrl, pos, size, flags, blend, Back, Bord ),
					MinControlSize( MinCtrlSize )
				{
				}

				inline ~CreateParams() {}

				String	TooltipText;
				Sizei	MinControlSize;
		};

		cUIComplexControl( const cUIComplexControl::CreateParams& Params );

		virtual ~cUIComplexControl();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void Update();

		virtual void Size( const Sizei &Size );

		void Size( const Int32& Width, const Int32& Height );

		const Sizei& Size();

		cUITooltip * Tooltip();

		void TooltipRemove();

		void TooltipText( const String& Text );

		String TooltipText();

		void UpdateAnchorsDistances();
	protected:
		cUITooltip *	mTooltip;
		Sizei			mMinControlSize;
		Recti			mDistToBorder;

		void CreateTooltip();

		virtual void OnParentSizeChange( const Vector2i& SizeChange );
};

}}

#endif
