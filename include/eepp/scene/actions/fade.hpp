#ifndef EE_SCENE_ACTION_FADE_HPP
#define EE_SCENE_ACTION_FADE_HPP

#include <eepp/scene/action.hpp>
#include <eepp/scene/actions/actioninterpolation1d.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Fade : public ActionInterpolation1d {
  public:
	static Fade* New( const Float& start, const Float& end, const Time& duration,
					  const Ease::Interpolation& type = Ease::Linear,
					  const bool& alphaChilds = true );

	Action* clone() const override;

	Action* reverse() const override;

  protected:
	Fade();

	Fade( const Float& start, const Float& end, const Time& duration,
		  const Ease::Interpolation& type, const bool& alphaChilds );

	void onStart() override;

	void onUpdate( const Time& time ) override;

	bool mAffectChilds;
};

class EE_API FadeIn : public Fade {
  public:
	static FadeIn* New( const Time& duration, const Ease::Interpolation& type = Ease::Linear,
						const bool& alphaChilds = true );

	void start() override;

	Action* clone() const override;

	Action* reverse() const override;

  protected:
	Time mDuration;
	Ease::Interpolation mType;

	FadeIn();

	FadeIn( const Time& duration, const Ease::Interpolation& type, const bool& alphaChilds );
};

class EE_API FadeOut : public Fade {
  public:
	static FadeOut* New( const Time& duration, const Ease::Interpolation& type = Ease::Linear,
						 const bool& alphaChilds = true );

	void start() override;

	Action* clone() const override;

	Action* reverse() const override;

  protected:
	Time mDuration;
	Ease::Interpolation mType;

	FadeOut();

	FadeOut( const Time& duration, const Ease::Interpolation& type, const bool& alphaChilds );
};

}}} // namespace EE::Scene::Actions

#endif
