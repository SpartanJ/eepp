#include <algorithm>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/tools/uiinspector.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiwebview.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace UI {

UIInspector::UIInspector() = default;

UIInspector::~UIInspector() = default;

void UIInspector::setDefaultScene( UISceneNode* scene ) {
	mDefaultScene = scene;
	if ( scene )
		sceneHandle( scene );
}

UISceneNode* UIInspector::defaultScene() const {
	return mDefaultScene;
}

std::string UIInspector::registerNode( Scene::Node* node, const char* prefix, NodeHandles& handles,
									   ReverseNodeHandles& reverse ) {
	if ( !node )
		return {};
	auto found = reverse.find( node );
	if ( found != reverse.end() )
		return found->second;
	Uint64& next = prefix[0] == 'w' ? mNextWidget : mNextScene;
	std::string handle = std::string( prefix ) + std::to_string( next++ );
	NodeEntry entry;
	entry.node = node;
	entry.close = node->connect( Scene::Event::OnClose,
								 [this, node, handle, &handles, &reverse]( const Scene::Event* ) {
									 auto it = handles.find( handle );
									 if ( it != handles.end() )
										 it->second.node = nullptr;
									 reverse.erase( node );
									 if ( mDefaultScene == node )
										 mDefaultScene = nullptr;
								 } );
	handles.emplace( handle, std::move( entry ) );
	reverse.emplace( node, handle );
	return handle;
}

std::string UIInspector::windowHandle( EE::Window::Window* window ) {
	if ( !window )
		return {};
	auto found = mWindowReverse.find( window );
	if ( found != mWindowReverse.end() )
		return found->second;
	std::string handle = "win:" + std::to_string( mNextWindow++ );
	mWindowReverse.emplace( window, handle );
	mWindows.emplace( handle, window );
	return handle;
}

std::string UIInspector::sceneHandle( UISceneNode* scene ) {
	return registerNode( scene, "scene:", mScenes, mSceneReverse );
}

std::string UIInspector::widgetHandle( UIWidget* widget ) {
	return registerNode( widget, "w:", mWidgets, mWidgetReverse );
}

EE::Window::Window* UIInspector::window( const std::string& handle ) const {
	auto it = mWindows.find( handle );
	if ( it == mWindows.end() )
		return nullptr;
	bool live = false;
	EE::Window::Engine::instance()->forEachWindow(
		[&]( EE::Window::Window* candidate ) { live |= candidate == it->second; } );
	return live ? it->second : nullptr;
}

void UIInspector::invalidateWindow( EE::Window::Window* window ) {
	auto it = mWindowReverse.find( window );
	if ( it != mWindowReverse.end() ) {
		mWindows.erase( it->second );
		mWindowReverse.erase( it );
	}
}

UISceneNode* UIInspector::scene( const std::string& handle ) const {
	auto it = mScenes.find( handle );
	return it == mScenes.end() ? nullptr : static_cast<UISceneNode*>( it->second.node );
}

UIWidget* UIInspector::widget( const std::string& handle ) const {
	auto it = mWidgets.find( handle );
	return it == mWidgets.end() ? nullptr : static_cast<UIWidget*>( it->second.node );
}

SmallVector<UISceneNode*, 4> UIInspector::topLevelScenes() const {
	SmallVector<UISceneNode*, 4> scenes;
	Scene::SceneManager::instance()->forEachSceneNode( [&]( Scene::SceneNode* node ) {
		if ( node->isUISceneNode() )
			scenes.push_back( node->asType<UISceneNode>() );
	} );
	return scenes;
}

WidgetQueryResult UIInspector::query( UISceneNode* scene, const std::string& selector ) const {
	if ( !scene || !scene->getRoot() )
		return {};
	WidgetQueryResult widgets = scene->getRoot()->querySelectorAll( selector );
	widgets.erase(
		std::remove_if( widgets.begin(), widgets.end(),
						[scene]( UIWidget* widget ) { return widget->getUISceneNode() != scene; } ),
		widgets.end() );
	return widgets;
}

std::string UIInspector::property( UIWidget* widget, const std::string& name ) const {
	if ( !widget || !CSS::StyleSheetSpecification::instance()->getProperty( name ) )
		return {};
	return widget->getPropertyString( name );
}

bool UIInspector::isSecret( UIWidget* widget ) const {
	if ( !widget )
		return false;
	if ( widget->isType( UI_TYPE_TEXTINPUT ) &&
		 widget->asType<UITextInput>()->getMode() == UITextInput::TextInputMode::Password )
		return true;
	if ( widget->isType( UI_TYPE_HTML_INPUT ) &&
		 widget->asType<UIHTMLInput>()->getInputType() == "password" )
		return true;
	return false;
}

}} // namespace EE::UI
