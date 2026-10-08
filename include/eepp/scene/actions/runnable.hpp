#ifndef EE_SCENE_ACTION_CALLBACK_HPP
#define EE_SCENE_ACTION_CALLBACK_HPP

#include <eepp/core/small_function.hpp>
#include <eepp/scene/actions/delay.hpp>
#include <eepp/system/mutex.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Runnable : public Delay {
  public:
	using RunnableFunc = SmallFunction<48>;

	static Runnable* New( RunnableFunc callback, const Time& time = Seconds( 0 ),
						  bool loop = false );

	void update( const Time& time ) override;

	bool isDone() override;

	Action* clone() const override;

	Action* reverse() const override;

	void setCallback( RunnableFunc&& callback );

  protected:
	RunnableFunc mCallback;
	Mutex mCallbackMutex;
	bool mCalled{ false };
	bool mLoop{ false };

	explicit Runnable( RunnableFunc callback, const Time& time = Seconds( 0 ), bool loop = false );

	void onStart() override;
};

}}} // namespace EE::Scene::Actions

#endif
