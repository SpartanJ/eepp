#ifndef EE_SCENEMANAGER_HPP
#define EE_SCENEMANAGER_HPP

#include <eepp/system/clock.hpp>
#include <eepp/system/container.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/system/time.hpp>
using namespace EE::System;

namespace EE { namespace UI {
class UISceneNode;
}} // namespace EE::UI
using namespace EE::UI;

namespace EE { namespace Scene {

class SceneNode;

class EE_API SceneManager {
	SINGLETON_DECLARE_HEADERS( SceneManager )
  public:
	static bool isActive();

	SceneManager();

	~SceneManager();

	SceneNode* add( SceneNode* sceneNode );

	bool remove( SceneNode* sceneNode );

	size_t count() const;

	void draw();

	void update( const Time& elapsed );

	void update();

	bool isShuttingDown() const;

	UISceneNode* getUISceneNode();

	void setCurrentUISceneNode( UISceneNode* uiSceneNode );

	Time getElapsed() const;

  protected:
	Clock mClock;
	UISceneNode* mUISceneNode;
	bool mIsShuttingDown;
	std::vector<SceneNode*> mSceneNodes;
};

}} // namespace EE::Scene

#endif
