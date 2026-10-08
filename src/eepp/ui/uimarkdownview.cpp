#include <eepp/system/filesystem.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/doc/markdownhelper.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uimarkdownview.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/window/input.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using namespace EE::UI::Doc;

namespace EE { namespace UI {

static URI markdownFileURI( const std::string& path ) {
	std::string absolutePath;
	if ( FileSystem::isRelativePath( path ) ) {
		absolutePath = FileSystem::getCurrentWorkingDirectory();
		FileSystem::dirAddSlashAtEnd( absolutePath );
		absolutePath += path;
	} else {
		absolutePath = path;
	}
#if EE_PLATFORM == EE_PLATFORM_WIN
	String::replaceAll( absolutePath, "\\", "/" );
	if ( absolutePath.size() > 1 && absolutePath[1] == ':' )
		absolutePath.insert( absolutePath.begin(), '/' );
#endif
	URI uri;
	uri.setScheme( "file" );
	uri.setPath( absolutePath );
	uri.normalize();
	return uri;
}

UIMarkdownView* UIMarkdownView::New() {
	return eeNew( UIMarkdownView, () );
}

UIMarkdownView::UIMarkdownView() : UILinearLayout( "markdownview", UIOrientation::Vertical ) {
	mTextSelectionController.setHost( this );
	mTextSelectionController.setSelectionRoot( this );
	mTextSelectionController.setLinkResolverCb(
		[this]( const std::string& href ) { return resolveLink( URI( href ) ).toString(); } );
	subscribeScheduledUpdate();
	mWidthPolicy = SizePolicy::MatchParent;
	mHeightPolicy = SizePolicy::WrapContent;
	getUISceneNode()->loadHTMLBasicCSS();
	updateNavigationInterceptor();
}

UIMarkdownView::~UIMarkdownView() {
	if ( mNavigationScene )
		mNavigationScene->setNavigationInterceptorCb( this, {} );
	if ( mNavigationLoadState )
		mNavigationLoadState->owner = nullptr;
	mTextSelectionController.onDocumentWillChange();
}

Uint32 UIMarkdownView::getType() const {
	return UI_TYPE_MARKDOWNVIEW;
}

bool UIMarkdownView::isType( const Uint32& type ) const {
	return UIMarkdownView::getType() == type || UILinearLayout::isType( type );
}

void UIMarkdownView::updateNavigationInterceptor() {
	auto* scene = getUISceneNode();
	if ( scene == mNavigationScene )
		return;
	if ( mNavigationScene )
		mNavigationScene->setNavigationInterceptorCb( this, {} );
	mNavigationScene = nullptr;
	if ( scene &&
		 scene->setNavigationInterceptorCb(
			 this, [this]( const NavigationRequest& request ) { return navigate( request ); } ) ) {
		mNavigationScene = scene;
	}
}

void UIMarkdownView::onSceneChange() {
	UILinearLayout::onSceneChange();
	updateNavigationInterceptor();
}

UITextSelectionController* UIMarkdownView::getTextSelectionController() {
	return &mTextSelectionController;
}

const UITextSelectionController* UIMarkdownView::getTextSelectionController() const {
	return &mTextSelectionController;
}

Uint32 UIMarkdownView::onKeyDown( const KeyEvent& event ) {
	if ( mTextSelectionController.onKeyDown( event ) )
		return 1;
	return UILinearLayout::onKeyDown( event );
}

Uint32 UIMarkdownView::onMessage( const NodeMessage* message ) {
	if ( message->getMsg() == NodeMessage::MouseUp &&
		 mTextSelectionController.onMouseUpMessage( message ) )
		return 1;
	return UILinearLayout::onMessage( message );
}

void UIMarkdownView::scheduledUpdate( const Time& time ) {
	UILinearLayout::scheduledUpdate( time );
	// Continue a captured drag when the pointer leaves a text owner or the view scrolls.
	mTextSelectionController.updateSelectionDrag();
}

void UIMarkdownView::loadFromString( std::string_view markdown ) {
	loadFromString( markdown, {} );
}

void UIMarkdownView::loadFromString( std::string_view markdown, const std::string& documentPath ) {
	loadDocument( markdown, documentPath.empty() ? std::string{}
												 : markdownFileURI( documentPath ).getFSPath() );
}

void UIMarkdownView::loadDocument( std::string_view markdown, std::string documentPath ) {
	if ( mNavigationLoadState )
		++mNavigationLoadState->generation;
	mDocumentPath = std::move( documentPath );
	mTextSelectionController.onDocumentWillChange();
	closeAllChildren();
	auto xhtml = Tools::HTMLFormatter::HTMLBodyToXML( Markdown::toXHTML( markdown ) );
	getUISceneNode()->loadLayoutFromString( xhtml, this );
	mTextSelectionController.onDocumentChanged();
	sendCommonEvent( Event::OnDocumentChanged );
}

void UIMarkdownView::loadFromFile( const std::string& path ) {
	if ( !mNavigationLoadState )
		mNavigationLoadState = std::make_shared<NavigationLoadState>( NavigationLoadState{ this } );
	const Uint64 generation = ++mNavigationLoadState->generation;
	std::weak_ptr<NavigationLoadState> state = mNavigationLoadState;
	auto* scene = getUISceneNode();
	auto resourceState = scene->getAsyncResourceLoadState();
	const Uint64 resourceGeneration = resourceState->generation.load( std::memory_order_acquire );
	std::string filePath = markdownFileURI( path ).getFSPath();
	auto load = [state, generation, resourceState, resourceGeneration,
				 filePath = std::move( filePath )]() mutable {
		std::string data;
		const bool success = FileSystem::fileGet( filePath, data );
		UISceneNode::runAsyncResourceOnMainThread(
			resourceState, resourceGeneration,
			[state, generation, filePath = std::move( filePath ), data = std::move( data ),
			 success]( UISceneNode* ) mutable {
				auto locked = state.lock();
				if ( !locked || !locked->owner || locked->generation != generation )
					return;
				auto* view = locked->owner;
				URI uri = markdownFileURI( filePath );
				if ( success ) {
					view->loadDocument( data, std::move( filePath ) );
					// Document observers may close the view or replace its content again.
					if ( locked->owner != view || locked->generation != generation + 1 )
						return;
				}
				NavigationEvent event(
					view, success ? Event::OnNavigationCompleted : Event::OnNavigationError,
					std::move( uri ), success );
				view->sendEvent( &event );
			} );
	};
	if ( auto pool = scene->getThreadPool() )
		pool->run( std::move( load ) );
	else
		load();
}

const std::string& UIMarkdownView::getDocumentPath() const {
	return mDocumentPath;
}

UIMarkdownView* UIMarkdownView::setFollowLocalLinks( bool follow ) {
	mFollowLocalLinks = follow;
	return this;
}

bool UIMarkdownView::getFollowLocalLinks() const {
	return mFollowLocalLinks;
}

URI UIMarkdownView::resolveLink( URI uri ) const {
	// file://docs/page.md is commonly used as a relative local path, although URI parsing
	// treats "docs" as an authority. Keep standard file:/// paths and localhost URLs absolute.
	if ( uri.getScheme() == "file" && !uri.getHost().empty() && uri.getHost() != "localhost" ) {
		std::string path = uri.getAuthority() + uri.getPath();
		uri.setAuthority( "" );
		uri.setScheme( "" );
		uri.setPath( path );
	}
	URI base =
		mDocumentPath.empty() ? getUISceneNode()->getURI() : markdownFileURI( mDocumentPath );
	return getUISceneNode()->solveRelativePath( std::move( uri ), std::move( base ) );
}

bool UIMarkdownView::navigate( const NavigationRequest& request ) {
	NavigationRequest resolved = request;
	resolved.uri = resolveLink( request.uri );
	if ( ( request.mouseButtons & EE_BUTTON_MMASK ) ||
		 ( ( request.mouseButtons & EE_BUTTON_LMASK ) &&
		   ( request.modifiers & KeyMod::getDefaultModifier() ) ) ) {
		resolved.target = NavigationRequest::Target::NewTab;
	}
	LinkOpenEvent event( this, std::move( resolved ) );
	sendEvent( &event );
	if ( event.handled )
		return true;
	const auto& uri = event.request.uri;
	if ( !mFollowLocalLinks || uri.getScheme() != "file" || request.method != "GET" )
		return false;
	const auto ext = FileSystem::fileExtension( uri.getPath() );
	if ( ext != "md" && ext != "markdown" )
		return false;
	loadFromFile( uri.getFSPath() );
	return true;
}

void UIMarkdownView::loadFromXmlNode( const pugi::xml_node& node ) {
	UILinearLayout::loadFromXmlNode( node );
	if ( !node.text().empty() )
		loadFromString( node.text().as_string() );
}

UIScrollableMarkdownView* UIScrollableMarkdownView::New() {
	return eeNew( UIScrollableMarkdownView, () );
}

UIScrollableMarkdownView::UIScrollableMarkdownView() :
	UIScrollView( "scrollablemarkdownview" ),
	WidgetCommandExecuter( getInput() ),
	mMarkdownView( UIMarkdownView::New() ) {
	mFlags |= UI_LOADS_ITS_CHILDREN;
	mMarkdownView->setParent( this );
	mMarkdownView->on( Event::OnDocumentChanged, [this]( const Event* ) { updateHistory(); } );
	mMarkdownView->on( Event::OnNavigationError,
					   [this]( const Event* ) { mPendingHistoryIndex = -1; } );
	mMarkdownView->on( Event::OnCreateContextMenu,
					   [this]( const Event* event ) { onCreateContextMenu( event ); } );
	// File navigation starts at the top; live source updates and failed loads keep their scroll.
	mMarkdownView->on( Event::OnNavigationCompleted,
					   [this]( const Event* ) { resetScrollPosition(); } );
}

Uint32 UIScrollableMarkdownView::getType() const {
	return UI_TYPE_SCROLLABLEMARKDOWNVIEW;
}

bool UIScrollableMarkdownView::isType( const Uint32& type ) const {
	return UIScrollableMarkdownView::getType() == type || UIScrollView::isType( type );
}

UIMarkdownView* UIScrollableMarkdownView::getMarkdownView() const {
	return mMarkdownView;
}

void UIScrollableMarkdownView::setHistoryNavigationEnabled( bool enabled ) {
	if ( mHistoryNavigationEnabled == enabled )
		return;
	mHistoryNavigationEnabled = enabled;
	if ( enabled ) {
		setCommand( "go-back", [this] { goHistoryBack(); } );
		setCommand( "go-forward", [this] { goHistoryForward(); } );
		if ( !getKeyBindings().hasCommand( "go-back" ) )
			getKeyBindings().addKeybindString( "mod+[", "go-back" );
		if ( !getKeyBindings().hasCommand( "go-forward" ) )
			getKeyBindings().addKeybindString( "mod+]", "go-forward" );
		updateHistory();
	} else {
		unsetCommand( "go-back" );
		unsetCommand( "go-forward" );
		// Remove every shortcut alias, not just each command's reverse binding.
		for ( auto it = getKeyBindings().getShortcutMap().begin();
			  it != getKeyBindings().getShortcutMap().end(); ) {
			if ( it->second == "go-back" || it->second == "go-forward" ) {
				const auto key = it->first;
				getKeyBindings().removeKeybind( key );
				it = getKeyBindings().getShortcutMap().begin();
			} else {
				++it;
			}
		}
		getKeyBindings().removeCommandKeybind( "go-back" );
		getKeyBindings().removeCommandKeybind( "go-forward" );
		mHistory.clear();
		mHistoryIndex = -1;
		mPendingHistoryIndex = -1;
		mHistoryMenuConnection = {};
	}
}

bool UIScrollableMarkdownView::isHistoryNavigationEnabled() const {
	return mHistoryNavigationEnabled;
}

void UIScrollableMarkdownView::goHistoryBack() {
	if ( canGoBack() ) {
		navigateToHistoryIndex(
			( mPendingHistoryIndex >= 0 ? mPendingHistoryIndex : mHistoryIndex ) - 1 );
	}
}

void UIScrollableMarkdownView::goHistoryForward() {
	if ( canGoForward() ) {
		navigateToHistoryIndex(
			( mPendingHistoryIndex >= 0 ? mPendingHistoryIndex : mHistoryIndex ) + 1 );
	}
}

bool UIScrollableMarkdownView::canGoBack() const {
	return mHistoryNavigationEnabled &&
		   ( mPendingHistoryIndex >= 0 ? mPendingHistoryIndex : mHistoryIndex ) > 0;
}

bool UIScrollableMarkdownView::canGoForward() const {
	const Int32 index = mPendingHistoryIndex >= 0 ? mPendingHistoryIndex : mHistoryIndex;
	return mHistoryNavigationEnabled && index >= 0 &&
		   index < static_cast<Int32>( mHistory.size() ) - 1;
}

const std::vector<std::string>& UIScrollableMarkdownView::getHistory() const {
	return mHistory;
}

Int32 UIScrollableMarkdownView::getHistoryIndex() const {
	return mHistoryIndex;
}

void UIScrollableMarkdownView::loadHistoryDocument( const std::string& path ) {
	mMarkdownView->loadFromFile( path );
}

void UIScrollableMarkdownView::navigateToHistoryIndex( Int32 index ) {
	if ( !mHistoryNavigationEnabled || index < 0 || index >= static_cast<Int32>( mHistory.size() ) )
		return;
	mPendingHistoryIndex = index;
	loadHistoryDocument( mHistory[index] );
}

void UIScrollableMarkdownView::updateHistory() {
	if ( !mHistoryNavigationEnabled )
		return;
	const auto& path = mMarkdownView->getDocumentPath();
	if ( mPendingHistoryIndex >= 0 && mHistory[mPendingHistoryIndex] == path ) {
		mHistoryIndex = mPendingHistoryIndex;
		mPendingHistoryIndex = -1;
		resetScrollPosition();
		return;
	}
	mPendingHistoryIndex = -1;
	if ( path.empty() ) {
		mHistory.clear();
		mHistoryIndex = -1;
		return;
	}
	// Updating the current buffer must preserve the forward branch.
	if ( mHistoryIndex >= 0 && mHistory[mHistoryIndex] == path )
		return;
	if ( mHistoryIndex >= 0 )
		mHistory.resize( mHistoryIndex + 1 );
	mHistory.emplace_back( path );
	mHistoryIndex = static_cast<Int32>( mHistory.size() ) - 1;
}

void UIScrollableMarkdownView::resetScrollPosition() {
	getHorizontalScrollBar()->setValue( 0.f );
	getVerticalScrollBar()->setValue( 0.f );
}

void UIScrollableMarkdownView::onCreateContextMenu( const Event* event ) {
	if ( !canGoBack() && !canGoForward() )
		return;
	auto* menu = static_cast<const ContextMenuEvent*>( event )->getMenu();
	menu->addSeparator();
	auto* back = menu->add( i18n( "uimarkdownview_go_back", "Go Back" ), {},
							getKeyBindings().getShortcutString(
								getKeyBindings().getShortcutFromCommand( "go-back" ), true ) );
	back->setId( "go-back" );
	back->setEnabled( canGoBack() );
	auto* forward =
		menu->add( i18n( "uimarkdownview_go_forward", "Go Forward" ), {},
				   getKeyBindings().getShortcutString(
					   getKeyBindings().getShortcutFromCommand( "go-forward" ), true ) );
	forward->setId( "go-forward" );
	forward->setEnabled( canGoForward() );
	mHistoryMenuConnection = menu->connect( Event::OnItemClicked, [this]( const Event* event ) {
		if ( event->getNode()->isType( UI_TYPE_MENUITEM ) ) {
			const auto& id = event->getNode()->asType<UIMenuItem>()->getId();
			if ( id == "go-back" || id == "go-forward" )
				execute( id );
		}
	} );
}

void UIScrollableMarkdownView::loadFromXmlNode( const pugi::xml_node& node ) {
	UIScrollView::loadFromXmlNode( node );
	if ( !node.text().empty() )
		mMarkdownView->loadFromString( node.text().as_string() );
}

Uint32 UIScrollableMarkdownView::onKeyDown( const KeyEvent& event ) {
	return WidgetCommandExecuter::onKeyDown( event ) || UIScrollView::onKeyDown( event );
}

Uint32 UIScrollableMarkdownView::onMessage( const NodeMessage* message ) {
	if ( message->getMsg() == NodeMessage::MouseUp && ( message->getFlags() & EE_BUTTON_RMASK ) &&
		 ( message->getSender() == this || isParentOf( message->getSender() ) ) && getInput() ) {
		// The Markdown child handles text; also offer its menu on empty viewport space.
		const auto position =
			getInput()
				->getMousePosFromView( getUISceneNode()->getWindow()->getDefaultView() )
				.asInt();
		if ( mMarkdownView->getTextSelectionController()->showContextMenu(
				 position, message->getFlags(), message->getSender() ) ) {
			return 1;
		}
	}
	return UIScrollView::onMessage( message );
}

}} // namespace EE::UI
