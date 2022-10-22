#ifndef EE_SCENE_ACTION_DISABLE_HPP
#define EE_SCENE_ACTION_DISABLE_HPP

#include <eepp/scene/actions/enable.hpp>

namespace EE { namespace Scene { namespace Actions {

class EE_API Disable : public Enable {
  public:
	static Disable* New( const Time& time = Seconds( 0 ) );

	Action* clone() const override;

  protected:
	explicit Disable( const Time& time );
};

}}} // namespace EE::Scene::Actions

#endif
