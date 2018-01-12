#ifndef EE_UI_ACTION_MOVE_HPP
#define EE_UI_ACTION_MOVE_HPP

#include <eepp/ui/uiaction.hpp>
#include <eepp/ui/actions/actioninterpolation2d.hpp>

namespace EE { namespace UI { namespace Action {

class Move : public ActionInterpolation2d {
	public:
		static Move * New( const Vector2f& start, const Vector2f& end, const Time& duration, const Ease::Interpolation& type = Ease::Linear );

		UIAction * clone() const;

		UIAction * reverse() const;
	protected:
		Move( const Vector2f& start, const Vector2f& end, const Time& duration, const Ease::Interpolation& type );

		void onStart();

		void onUpdate( const Time& time );
	private:
		Move();
};

}}}

#endif

