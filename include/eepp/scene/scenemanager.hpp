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
	SceneManager();

	~SceneManager();

	SceneNode* add(SceneNode* sceneNode );

	bool remove( SceneNode* sceneNode );

	size_t count() const;

	void draw();

	void update( const Time& elapsed );

	void update();

	bool isShootingDown() const;

	UISceneNode* getUISceneNode();

	void setCurrentUISceneNode( UISceneNode* uiSceneNode );

  protected:
	Clock mClock;
	UISceneNode* mUISceneNode;
	bool mIsShootingDown;
	std::vector<SceneNode*> mSceneNodes;
};

}} // namespace EE::Scene

#endif
