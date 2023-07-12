#ifndef EE_SCENE_ACTION_CALLBACK_HPP
#define EE_SCENE_ACTION_CALLBACK_HPP

#include <eepp/scene/actions/delay.hpp>
#include <functional>

namespace EE { namespace Scene { namespace Actions {

class EE_API Runnable : public Delay {
  public:
	typedef std::function<void()> RunnableFunc;

	static Runnable* New( RunnableFunc callback, const Time& time = Seconds( 0 ),
						  bool loop = false );

	void update( const Time& time ) override;

	bool isDone() override;

	Action* clone() const override;

	Action* reverse() const override;

  protected:
	RunnableFunc mCallback;
	bool mCalled{ false };
	bool mLoop{ false };

	explicit Runnable( RunnableFunc callback, const Time& time = Seconds( 0 ), bool loop = false );

	void onStart() override;
};

}}} // namespace EE::Scene::Actions

#endif
