#ifndef EE_UIEVENTDISPATCHER_HPP
#define EE_UIEVENTDISPATCHER_HPP

#include <eepp/scene/eventdispatcher.hpp>
using namespace EE::Scene;

namespace EE { namespace UI {

class EE_API UIEventDispatcher : public EventDispatcher {
  public:
	static UIEventDispatcher* New( SceneNode* sceneNode );

	UIEventDispatcher( SceneNode* sceneNode );

	const bool& justGainedFocus() const;

  protected:
	bool mJustGainedFocus;

	void inputCallback( InputEvent* Event );

	void checkTabPress( const Uint32& KeyCode );
};

}} // namespace EE::UI

#endif
