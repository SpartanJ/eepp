#ifndef EE_UI_ACTION_FADE_HPP
#define EE_UI_ACTION_FADE_HPP

#include <eepp/ui/uiaction.hpp>
#include <eepp/ui/actions/actioninterpolation1d.hpp>

namespace EE { namespace UI { namespace Action {

class Fade : public ActionInterpolation1d {
	public:
		static Fade * New( const Float& start, const Float& end, const Time& duration, const Ease::Interpolation& type = Ease::Linear, const bool& alphaChilds = true );

		UIAction * clone() const;

		UIAction * reverse() const;
	protected:
		Fade( const Float & start, const Float & end, const Time & duration, const Ease::Interpolation & type, const bool& alphaChilds );

		void onStart();

		void onUpdate( const Time& time );

		bool mAffectChilds;
	private:
		Fade();
};

}}}

#endif

