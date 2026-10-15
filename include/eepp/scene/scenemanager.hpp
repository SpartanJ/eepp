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
namespace EE { namespace Window {
class Window;
}} // namespace EE::Window
using namespace EE::UI;

namespace EE { namespace Scene {

class SceneNode;

class EE_API SceneManager {
	SINGLETON_DECLARE_HEADERS( SceneManager )
  public:
	static bool isActive();

	SceneManager();

	~SceneManager();

	/** Registers a scene and makes it current if no scene is currently bound. */
	SceneNode* add( SceneNode* sceneNode );

	bool remove( SceneNode* sceneNode );

	size_t count() const;

	void draw();

	/** Draws only scene nodes associated with @p window using its graphics context. */
	void draw( EE::Window::Window* window );

	void update( const Time& elapsed );

	void update();

	/** Returns the currently bound scene, or nullptr if none is bound. Scoped contexts temporarily
	 * override this persistent binding. Scene binding is restricted to the application thread. */
	SceneNode* getCurrentSceneNode() const;

	/** Returns the current scene if it is a UI scene, otherwise nullptr. */
	UISceneNode* getUISceneNode();

	/** Returns the UI scene associated with @p window, or nullptr if none is registered. */
	UISceneNode* getUISceneNode( EE::Window::Window* window );

	/** Enumerates registered top-level scenes on the UI thread. */
	void forEachSceneNode( const std::function<void( SceneNode* )>& callback ) const;

	/** Sets the persistent scene binding and returns the previous scene. Does not switch native
	 * windows; use SceneNode::makeCurrent() for scoped scene and graphics-context binding. */
	SceneNode* setCurrentSceneNode( SceneNode* sceneNode );

	/** Removes and destroys all registered scene nodes associated with @p window. */
	void destroyScenes( EE::Window::Window* window );

	Time getElapsed() const;

  protected:
	Clock mClock;
	SceneNode* mCurrentSceneNode;
	std::vector<SceneNode*> mSceneNodes;
};

}} // namespace EE::Scene

#endif
