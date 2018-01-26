#ifndef EE_UI_ACTION_SCALE_HPP
#define EE_UI_ACTION_SCALE_HPP

#include <eepp/ui/uiaction.hpp>
#include <eepp/ui/actions/actioninterpolation2d.hpp>

namespace EE { namespace UI { namespace Action {

class EE_API Scale : public ActionInterpolation2d {
	public:
		static Scale * New( const Vector2f& start, const Vector2f& end, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

		UIAction * clone() const override;

		UIAction * reverse() const override;
	protected:
		Scale( const Vector2f& start, const Vector2f& end, const Time& duration, const Ease::Interpolation& type );

		void onStart() override;

		void onUpdate( const Time& time ) override;
	private:
		Scale();
};

}}}

#endif

