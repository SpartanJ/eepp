#include "../../eepp/ui/accessibility/accessibilitybackend.hpp"
#include "../../eepp/ui/accessibility/accessibilitybackenduia.hpp"

#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>

#include <future>
#include <stdexcept>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Models;
using namespace EE::UI::Uia;
using namespace EE::Window;

namespace {

void require( bool condition, const char* message ) {
	if ( !condition )
		throw std::runtime_error( message );
}

UIApplication::Settings testSettings() {
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	return settings;
}

WindowSettings testWindow() {
	return WindowSettings( 320, 240, "UIA provider regression", WindowStyle::Hidden,
						   WindowBackend::Default, 32, {}, 1, false, true );
}

struct ReleaseCOM {
	template <typename T> void operator()( T* object ) const {
		if ( object )
			object->Release();
	}
};

class TeardownModel : public Model {
  public:
	size_t rowCount( const ModelIndex& = {} ) const override { return 1; }

	size_t columnCount( const ModelIndex& = {} ) const override { return 1; }

	Variant data( const ModelIndex&, ModelRole = ModelRole::Display ) const override {
		return "Selected row";
	}

	ModelIndex index( int row, int column = 0, const ModelIndex& = {} ) const override {
		if ( auto context = teardown.lock() ) {
			teardown.reset();
			context->detach();
		}
		return createIndex( row, column );
	}

	mutable std::weak_ptr<UIAutomationProviderContext> teardown;
};

} // namespace

void testProviderEventAdviceABI() {
	UIApplication app( testWindow(), testSettings() );
	require( app.getUI() != nullptr, "hidden test window could not be created" );
	auto* manager = app.getUI()->getAccessibilityManager();
	auto context = std::make_shared<UIAutomationProviderContext>( *manager, nullptr, 1 );
	std::unique_ptr<UIAutomationProvider, ReleaseCOM> root(
		context->provider( manager->getRoot() ) );
	require( root != nullptr, "root provider could not be created" );
	const IID adviceId{
		0xa407b27b, 0x0f6d, 0x4427, { 0x92, 0x92, 0x47, 0x3c, 0x7b, 0xf9, 0x32, 0x58 } };
	require( IsEqualIID( adviceId, __uuidof( IRawElementProviderAdviseEvents ) ),
			 "event advice interface UUID does not match the Windows ABI" );
	IRawElementProviderAdviseEvents* rawAdvice{};
	const HRESULT result = root->QueryInterface( adviceId, reinterpret_cast<void**>( &rawAdvice ) );
	std::unique_ptr<IRawElementProviderAdviseEvents, ReleaseCOM> advice( rawAdvice );
	require( result == S_OK && advice != nullptr, "event advice interface query failed" );
	context->updateClientState(); // Also exercises the UIAutomationCore import on old SDKs.
	require( !context->hasActiveClients(), "unobserved context unexpectedly active" );
	for ( int i = 0; i < 2; ++i ) {
		require( advice->AdviseEventAdded( UIA_AutomationFocusChangedEventId, nullptr ) == S_OK,
				 "event advice registration failed" );
	}
	require( context->hasActiveClients(), "event advice did not activate the context" );
	require( advice->AdviseEventRemoved( UIA_AutomationFocusChangedEventId, nullptr ) == S_OK,
			 "first event advice removal failed" );
	require( context->hasActiveClients(), "event advice lost the remaining subscription" );
	require( advice->AdviseEventRemoved( UIA_AutomationFocusChangedEventId, nullptr ) == S_OK,
			 "last event advice removal failed" );
	require( !context->hasActiveClients(), "balanced event advice left the context active" );
}

void testProviderSelectionTeardown() {
	UIApplication app( testWindow(), testSettings() );
	require( app.getUI() != nullptr, "hidden test window could not be created" );
	auto* manager = app.getUI()->getAccessibilityManager();
	auto* list = UIListView::New();
	list->setParent( app.getUI()->getRoot() );
	auto model = std::make_shared<TeardownModel>();
	list->setModel( model );
	list->setSelection( model->index( 0 ) );
	auto context = std::make_shared<UIAutomationProviderContext>( *manager, nullptr, 1 );
	std::unique_ptr<UIAutomationProvider, ReleaseCOM> provider(
		context->provider( manager->getNodeRef( list ) ) );
	require( provider != nullptr, "selection provider could not be created" );
	// The selection query obtains its refs successfully, but loses its native context before
	// building their SAFEARRAY. A model callback reproduces that interleaving deterministically.
	model->teardown = context;
	SAFEARRAY* selection{};
	const HRESULT result = provider->GetSelection( &selection );
	if ( selection )
		SafeArrayDestroy( selection );
	require( !context->isAlive(), "selection query did not trigger teardown" );
	require( result == static_cast<HRESULT>( UIA_E_ELEMENTNOTAVAILABLE ),
			 "selection teardown returned the wrong error" );
	require( selection == nullptr, "failed selection query returned a partial array" );
}

void testProviderTextRangeIdentity() {
	UIApplication app( testWindow(), testSettings() );
	require( app.getUI() != nullptr, "hidden test window could not be created" );
	auto* second = UISceneNode::New( app.getWindow() );
	Scene::SceneManager::instance()->add( second );
	second->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	auto* firstManager = app.getUI()->getAccessibilityManager();
	auto* secondManager = second->getAccessibilityManager();
	auto firstContext = std::make_shared<UIAutomationProviderContext>( *firstManager, nullptr, 1 );
	auto secondContext =
		std::make_shared<UIAutomationProviderContext>( *secondManager, nullptr, 2 );
	auto* firstInput = UITextInput::New();
	firstInput->setParent( app.getUI()->getRoot() );
	auto* secondInput = UITextInput::New();
	secondInput->setParent( second->getRoot() );
	const auto firstRef = firstManager->getNodeRef( firstInput );
	const auto secondRef = secondManager->getNodeRef( secondInput );
	require( firstRef == secondRef, "fixture requires identical manager-local refs" );
	std::unique_ptr<UIAutomationTextRange, ReleaseCOM> first(
		new UIAutomationTextRange( firstContext, firstRef, 0, 5 ) );
	std::unique_ptr<UIAutomationTextRange, ReleaseCOM> otherWindow(
		new UIAutomationTextRange( secondContext, secondRef, 0, 5 ) );
	std::unique_ptr<UIAutomationTextRange, ReleaseCOM> sameElement(
		new UIAutomationTextRange( firstContext, firstRef, 0, 5 ) );
	BOOL same{};
	require( first->Compare( otherWindow.get(), &same ) == S_OK && !same,
			 "ranges from different windows compare equal" );
	int comparison{};
	require( first->CompareEndpoints( TextPatternRangeEndpoint_Start, otherWindow.get(),
									  TextPatternRangeEndpoint_Start, &comparison ) == E_INVALIDARG,
			 "endpoint comparison accepted a different window" );
	require( first->MoveEndpointByRange( TextPatternRangeEndpoint_Start, otherWindow.get(),
										 TextPatternRangeEndpoint_End ) == E_INVALIDARG,
			 "endpoint movement accepted a different window" );
	require( first->Compare( sameElement.get(), &same ) == S_OK && same,
			 "equal ranges from the same element compare unequal" );
	require( first->CompareEndpoints( TextPatternRangeEndpoint_Start, sameElement.get(),
									  TextPatternRangeEndpoint_Start, &comparison ) == S_OK &&
				 comparison == 0,
			 "same-element endpoint comparison failed" );
	require( first->MoveEndpointByRange( TextPatternRangeEndpoint_Start, sameElement.get(),
										 TextPatternRangeEndpoint_End ) == S_OK,
			 "same-element endpoint movement failed" );
	require( first->CompareEndpoints( TextPatternRangeEndpoint_Start, sameElement.get(),
									  TextPatternRangeEndpoint_End, &comparison ) == S_OK &&
				 comparison == 0,
			 "same-element endpoint movement did not update the range" );
}

void testProviderPolicyChangeDuringDispatch() {
	bool completed = false;
	UIApplication app( testWindow(), testSettings() );
	require( app.getUI() != nullptr, "hidden test window could not be created" );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto context = std::make_shared<UIAutomationProviderContext>(
		*manager, reinterpret_cast<HWND>( app.getWindow()->getWindowHandler() ), 1 );
	class DispatchBackend : public AccessibilityBackend {
	  public:
		DispatchBackend( std::shared_ptr<UIAutomationProviderContext> context, bool& completed ) :
			context( std::move( context ) ), completed( completed ) {}

		~DispatchBackend() override { context->detach(); }

		bool isAvailable() const override { return context->isAlive(); }

		void onEvent( const AccessibilityPendingEvent& ) override {}

		void update() override {
			context->dispatchPendingRequests();
			completed = context->isAlive();
		}

	  private:
		std::shared_ptr<UIAutomationProviderContext> context;
		bool& completed;
	};
	manager->setBackend( std::make_unique<DispatchBackend>( context, completed ) );
	auto* button = UIPushButton::New();
	button->setParent( scene->getRoot() );
	button->onClick(
		[scene]( auto ) { scene->setAccessibilityPolicy( AccessibilityPolicy::Enabled ); } );
	std::unique_ptr<UIAutomationProvider, ReleaseCOM> provider(
		context->provider( manager->getNodeRef( button ) ) );
	require( provider != nullptr, "invoke provider could not be created" );
	auto request = std::async( std::launch::async, [&provider] { return provider->Invoke(); } );
	MSG message{};
	const auto deadline = GetTickCount64() + 1000;
	while ( !PeekMessageW( &message, context->window(), accessibilityDispatchMessage(),
						   accessibilityDispatchMessage(), PM_REMOVE ) &&
			GetTickCount64() < deadline )
		Sleep( 1 );
	require( message.message == accessibilityDispatchMessage(), "invoke did not queue a request" );
	manager->update();
	require( request.get() == S_OK, "queued invoke failed during policy change" );
	require( completed && context->isAlive(), "dispatch destroyed its own backend" );
	scene->update( Time::Zero );
	require( !context->isAlive(), "deferred policy change did not detach the old context" );
	require( scene->getAccessibilityPolicy() == AccessibilityPolicy::Enabled,
			 "deferred policy change was not applied" );
	scene->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
}

void testProviderScalarQueries() {
	UIApplication app( testWindow(), testSettings() );
	require( app.getUI() != nullptr, "hidden test window could not be created" );
	auto* manager = app.getUI()->getAccessibilityManager();
	class CountingModel : public TeardownModel {
	  public:
		Variant data( const ModelIndex& index,
					  ModelRole role = ModelRole::Display ) const override {
			++reads;
			return TeardownModel::data( index, role );
		}

		mutable size_t reads{};
	};
	auto* list = UIListView::New();
	auto model = std::make_shared<CountingModel>();
	list->setModel( model );
	auto context = std::make_shared<UIAutomationProviderContext>( *manager, nullptr, 1 );
	const auto row = manager->getChild( manager->getNodeRef( list ), 0 );
	std::unique_ptr<UIAutomationProvider, ReleaseCOM> provider( context->provider( row ) );
	require( provider != nullptr, "virtual row provider could not be created" );
	model->reads = 0;
	const PROPERTYID properties[] = {
		UIA_AutomationIdPropertyId,		   UIA_FrameworkIdPropertyId,
		UIA_ClassNamePropertyId,		   UIA_ProcessIdPropertyId,
		UIA_ControlTypePropertyId,		   UIA_IsEnabledPropertyId,
		UIA_IsKeyboardFocusablePropertyId, UIA_HasKeyboardFocusPropertyId,
		UIA_IsControlElementPropertyId,	   UIA_IsContentElementPropertyId,
		UIA_IsOffscreenPropertyId,		   UIA_IsPasswordPropertyId };
	for ( const auto property : properties ) {
		VARIANT value;
		VariantInit( &value );
		const HRESULT result = provider->GetPropertyValue( property, &value );
		const bool populated = value.vt != VT_EMPTY;
		VariantClear( &value );
		require( result == S_OK && populated, "scalar property query failed" );
	}
	require( model->reads == 0, "scalar queries fetched model display data" );
	VARIANT name;
	VariantInit( &name );
	const HRESULT result = provider->GetPropertyValue( UIA_NamePropertyId, &name );
	VariantClear( &name );
	require( result == S_OK && model->reads > 0, "name query did not fetch model display data" );

	auto* label = UITextView::New();
	label->setId( "scalar-provider-label" );
	label->setText( "Scalar label" );
	auto* button = UIPushButton::New();
	button->setAccessibilityLabelledBy( label->getId() );
	std::unique_ptr<UIAutomationProvider, ReleaseCOM> buttonProvider(
		context->provider( manager->getNodeRef( button ) ) );
	require( buttonProvider != nullptr, "labelled button provider could not be created" );
	manager->onNativeClientObserved();
	for ( const auto property : properties ) {
		VARIANT value;
		VariantInit( &value );
		const HRESULT result = buttonProvider->GetPropertyValue( property, &value );
		VariantClear( &value );
		require( result == S_OK, "button scalar property query failed" );
	}
	require( !manager->isRelationTarget( label->getIdHash() ),
			 "scalar queries tracked an unread label relation" );
	VariantInit( &name );
	const HRESULT nameResult = buttonProvider->GetPropertyValue( UIA_NamePropertyId, &name );
	VariantClear( &name );
	require( nameResult == S_OK && manager->isRelationTarget( label->getIdHash() ),
			 "name query did not track its label relation" );
}
