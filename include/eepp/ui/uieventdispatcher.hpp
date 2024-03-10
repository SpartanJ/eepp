#ifndef EE_UIEVENTDISPATCHER_HPP
#define EE_UIEVENTDISPATCHER_HPP

#include <eepp/scene/eventdispatcher.hpp>
using namespace EE::Scene;

namespace EE { namespace UI {

class EE_API UIEventDispatcher : public EventDispatcher {
  public:
	static UIEventDispatcher* New( SceneNode* sceneNode );

	explicit UIEventDispatcher( SceneNode* sceneNode );

	bool justGainedFocus() const;

  protected:
	bool mJustGainedFocus{ false };
	bool mJustLostFocus{ false };

	void inputCallback( InputEvent* Event );

	void checkTabPress( const Uint32& KeyCode, const Uint32& mod );
};

}} // namespace EE::UI

#endif
