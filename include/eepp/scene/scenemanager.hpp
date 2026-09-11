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

	SceneNode* add( SceneNode* sceneNode );

	bool remove( SceneNode* sceneNode );

	size_t count() const;

	void draw();

	/** Draws only scene nodes associated with @p window using its graphics context. */
	void draw( EE::Window::Window* window );

	void update( const Time& elapsed );

	void update();

	UISceneNode* getUISceneNode();

	/** Returns the UI scene associated with @p window, or nullptr if none is registered. */
	UISceneNode* getUISceneNode( EE::Window::Window* window );

	void setCurrentUISceneNode( UISceneNode* uiSceneNode );

	/** Removes and destroys all registered scene nodes associated with @p window. */
	void destroyScenes( EE::Window::Window* window );

	Time getElapsed() const;

  protected:
	friend class EE::UI::UISceneNode;

	/** Installs the ambient UI scene for the current scope and returns the previously scoped scene.
	 * UI scene and graphics-context switching is restricted to the application's UI thread. */
	UISceneNode* setScopedUISceneNode( UISceneNode* uiSceneNode );

	Clock mClock;
	UISceneNode* mUISceneNode;
	UISceneNode* mScopedUISceneNode;
	std::vector<SceneNode*> mSceneNodes;
};

}} // namespace EE::Scene

#endif
