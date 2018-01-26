#ifndef EE_UI_ACTION_FADE_HPP
#define EE_UI_ACTION_FADE_HPP

#include <eepp/ui/uiaction.hpp>
#include <eepp/ui/actions/actioninterpolation1d.hpp>

namespace EE { namespace UI { namespace Action {

class EE_API Fade : public ActionInterpolation1d {
	public:
		static Fade * New( const Float& start, const Float& end, const Time& duration, const Ease::Interpolation& type = Ease::Linear, const bool& alphaChilds = true );

		UIAction * clone() const override;

		UIAction * reverse() const override;
	protected:
		Fade( const Float & start, const Float & end, const Time & duration, const Ease::Interpolation & type, const bool& alphaChilds );

		void onStart() override;

		void onUpdate( const Time& time ) override;

		bool mAffectChilds;
	private:
		Fade();
};

}}}

#endif

