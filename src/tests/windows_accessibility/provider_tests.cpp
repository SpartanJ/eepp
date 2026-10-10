#include "../../eepp/ui/accessibility/accessibilitybackenduia.hpp"

#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uitextinput.hpp>

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
