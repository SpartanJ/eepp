#ifndef EE_UI_TOOLS_UIINSPECTORSERVER_HPP
#define EE_UI_TOOLS_UIINSPECTORSERVER_HPP

#include <eepp/config.hpp>
#include <memory>
#include <string>

namespace EE { namespace Window {
class Window;
}} // namespace EE::Window

namespace EE { namespace UI {
class UISceneNode;

struct UIInspectorServerSettings {
	std::string address{ "127.0.0.1" };
	Uint16 port{ 0 };
	bool readOnly{ false };
	std::string token;
};

/** Explicitly enabled TCP inspector. Public methods must be called from the UI thread. */
class EE_API UIInspectorServer {
  public:
	static UIInspectorServer* instance();

	static bool start( UISceneNode* defaultScene, UIInspectorServerSettings settings = {} );

	static void startFromEnvironment( UISceneNode* defaultScene );

	static void stop();

	static void pump();

	static void notifyWindowDestroyed( EE::Window::Window* window );

	bool isRunning() const;

	const std::string& getAddress() const;

	Uint16 getPort() const;

	const std::string& getToken() const;

	~UIInspectorServer();

  private:
	UIInspectorServer();
	struct Impl;
	std::unique_ptr<Impl> mImpl;
	static UIInspectorServer* sInstance;
};

}} // namespace EE::UI

#endif
