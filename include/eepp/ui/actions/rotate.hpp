#ifndef EE_UI_ACTION_ROTATE_HPP
#define EE_UI_ACTION_ROTATE_HPP

#include <eepp/ui/uiaction.hpp>
#include <eepp/ui/actions/actioninterpolation1d.hpp>

namespace EE { namespace UI { namespace Action {

class Rotate : public ActionInterpolation1d {
	public:
		static Rotate * New( const Float& start, const Float& end, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

		UIAction * clone() const;

		UIAction * reverse() const;
	protected:
		Rotate( const Float & start, const Float & end, const Time & duration, const Ease::Interpolation & type );

		void onStart();

		void onUpdate( const Time& time );
	private:
		Rotate();
};

}}}

#endif

