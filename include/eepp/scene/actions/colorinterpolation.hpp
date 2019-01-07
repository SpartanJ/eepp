#ifndef EE_SCENE_ACTIONS_COLORINTERPOLATION_HPP
#define EE_SCENE_ACTIONS_COLORINTERPOLATION_HPP

#include <eepp/scene/action.hpp>

#include <eepp/math/interpolation1d.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/system/color.hpp>
using namespace EE::Math;

namespace EE { namespace Scene { namespace Actions {

class EE_API ColorInterpolation : public Action {
	public:
		enum ColorInterpolationType {
			Background,
			Foreground,
			Skin,
			Border,
			Text
		};

		static ColorInterpolation * New( const Color& start, const Color& end, const bool& interpolateAlpha, const Time& duration, const Ease::Interpolation& type = Ease::Linear, const ColorInterpolationType& colorInterpolationType = Background );

		void start() override;

		void stop() override;

		void update( const Time& time ) override;

		bool isDone() override;

		virtual Action * clone() const override;

		Action * reverse() const override;

		Interpolation1d getInterpolationR() const;

		void setInterpolationR(const Interpolation1d & interpolationR);

		Interpolation1d getInterpolationG() const;

		void setInterpolationG(const Interpolation1d & interpolationG);

		Interpolation1d getInterpolationB() const;

		void setInterpolationB(const Interpolation1d & interpolationB);

		Interpolation1d getInterpolationA() const;

		void setInterpolationA(const Interpolation1d & interpolationA);
	protected:
		ColorInterpolation( const Color& start, const Color& end, const bool& interpolateAlpha, const Time & duration, const Ease::Interpolation & type, const ColorInterpolationType& colorInterpolationType );

		void onStart() override;

		virtual void onUpdate( const Time& time ) override;

		ColorInterpolation();

		Interpolation1d mInterpolationR;
		Interpolation1d mInterpolationG;
		Interpolation1d mInterpolationB;
		Interpolation1d mInterpolationA;
		ColorInterpolationType mColorInterpolationType;
		bool mInterpolateAlpha;
};

}}}

#endif
