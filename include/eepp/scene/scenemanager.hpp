#ifndef EE_SCENEMANAGER_HPP
#define EE_SCENEMANAGER_HPP

#include <eepp/system/singleton.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/time.hpp>
#include <eepp/system/container.hpp>
using namespace EE::System;

namespace EE { namespace UI {
class UISceneNode;
}}
using namespace EE::UI;

namespace EE { namespace Scene {

class SceneNode;

class SceneManager : public Container<SceneNode> {
	SINGLETON_DECLARE_HEADERS(SceneManager)
	public:
		SceneManager();

		~SceneManager();

		void draw();

		void update( const Time& elapsed );

		void update();

		bool isShootingDown() const;

		UISceneNode * getUISceneNode();
	protected:
		Clock mClock;
		UISceneNode * mUISceneNode;
		bool mIsShootingDown;
};

}}

#endif
