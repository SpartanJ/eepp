#ifndef EE_UI_TOOLS_UIINSPECTOR_HPP
#define EE_UI_TOOLS_UIINSPECTOR_HPP

#include <eepp/core/containers.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <string>

namespace EE { namespace UI {

/** UI-thread-only inspection domain. Handles are monotonic and invalidated on Node::OnClose. */
class EE_API UIInspector {
  public:
	UIInspector();

	~UIInspector();

	UIInspector( const UIInspector& ) = delete;

	UIInspector& operator=( const UIInspector& ) = delete;

	void setDefaultScene( UISceneNode* scene );

	UISceneNode* defaultScene() const;

	std::string windowHandle( EE::Window::Window* window );

	std::string sceneHandle( UISceneNode* scene );

	std::string widgetHandle( UIWidget* widget );

	EE::Window::Window* window( const std::string& handle ) const;

	void invalidateWindow( EE::Window::Window* window );

	UISceneNode* scene( const std::string& handle ) const;

	UIWidget* widget( const std::string& handle ) const;

	SmallVector<UISceneNode*, 4> topLevelScenes() const;

	WidgetQueryResult query( UISceneNode* scene, const std::string& selector ) const;

	std::string property( UIWidget* widget, const std::string& name ) const;

	bool isSecret( UIWidget* widget ) const;

  private:
	struct NodeEntry {
		Scene::Node* node{ nullptr };
		Scene::EventConnection close;
	};
	using NodeHandles = UnorderedMap<std::string, NodeEntry>;
	using ReverseNodeHandles = UnorderedMap<Scene::Node*, std::string>;

	std::string registerNode( Scene::Node* node, const char* prefix, NodeHandles& handles,
							  ReverseNodeHandles& reverse );

	Uint64 mNextWindow{ 1 };
	Uint64 mNextScene{ 1 };
	Uint64 mNextWidget{ 1 };
	UISceneNode* mDefaultScene{ nullptr };
	UnorderedMap<EE::Window::Window*, std::string> mWindowReverse;
	UnorderedMap<std::string, EE::Window::Window*> mWindows;
	ReverseNodeHandles mSceneReverse;
	NodeHandles mScenes;
	ReverseNodeHandles mWidgetReverse;
	NodeHandles mWidgets;
};

}} // namespace EE::UI

#endif
