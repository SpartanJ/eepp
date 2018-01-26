#ifndef EE_UI_ACTION_ROTATE_HPP
#define EE_UI_ACTION_ROTATE_HPP

#include <eepp/ui/uiaction.hpp>
#include <eepp/ui/actions/actioninterpolation1d.hpp>

namespace EE { namespace UI { namespace Action {

class EE_API Rotate : public ActionInterpolation1d {
	public:
		static Rotate * New( const Float& start, const Float& end, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

		UIAction * clone() const override;

		UIAction * reverse() const override;
	protected:
		Rotate( const Float & start, const Float & end, const Time & duration, const Ease::Interpolation & type );

		void onStart() override;

		void onUpdate( const Time& time ) override;
	private:
		Rotate();
};

}}}

#endif

