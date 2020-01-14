#ifndef EE_SCENE_ACTION_MARGINMOVE_HPP
#define EE_SCENE_ACTION_MARGINMOVE_HPP

#include <eepp/scene/action.hpp>

#include <eepp/math/interpolation1d.hpp>
#include <eepp/math/rect.hpp>
using namespace EE::Math;

namespace EE { namespace Scene { namespace Actions {

class EE_API MarginMove : public Action {
  public:
	enum InterpolateFlag {
		Left = 1 << 0,
		Top = 1 << 1,
		Right = 1 << 2,
		Bottom = 1 << 3,
		All = Left | Top | Right | Bottom
	};

	static MarginMove* New( const Rect& start, const Rect& end, const Time& duration,
							const Ease::Interpolation& type = Ease::Linear,
							const Uint32& interpolateFlag = InterpolateFlag::All );

	void start() override;

	void stop() override;

	void update( const Time& time ) override;

	bool isDone() override;

	virtual Action* clone() const override;

	Action* reverse() const override;

	Float getCurrentProgress() override;

	Interpolation1d getInterpolationLeft() const;

	void setInterpolationLeft( const Interpolation1d& interpolationLeft );

	Interpolation1d getInterpolationRight() const;

	void setInterpolationRight( const Interpolation1d& interpolationRight );

	Interpolation1d getInterpolationTop() const;

	void setInterpolationTop( const Interpolation1d& interpolationTop );

	Interpolation1d getInterpolationBottom() const;

	void setInterpolationBottom( const Interpolation1d& interpolationBottom );

  protected:
	MarginMove( const Rect& start, const Rect& end, const Time& duration,
				const Ease::Interpolation& type, const Uint32& interpolateFlag );

	void onStart() override;

	virtual void onUpdate( const Time& time ) override;

	MarginMove();

	Uint32 mFlags;
	Interpolation1d mInterpolationLeft;
	Interpolation1d mInterpolationRight;
	Interpolation1d mInterpolationTop;
	Interpolation1d mInterpolationBottom;
};

}}} // namespace EE::Scene::Actions

#endif
