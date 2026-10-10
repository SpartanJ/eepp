#include "accessibilitybackenduia.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace UI { namespace Uia {

UIAutomationProvider* UIAutomationProviderContext::provider( AccessibilityNodeRef ref ) {
	if ( !ref.isValid() || !isAlive() )
		return nullptr;
	std::lock_guard<std::mutex> lock( mProviderMutex );
	// Teardown may have cleared the cache while this caller was waiting for its lock.
	if ( !isAlive() )
		return nullptr;
	auto& sourceProviders = mProviders[ref.source];
	auto found = sourceProviders.find( ref.id );
	if ( found != sourceProviders.end() ) {
		found->second->AddRef();
		return found->second;
	}
	auto nativeProvider = new UIAutomationProvider( shared_from_this(), ref, ref == mRootRef );
	sourceProviders.emplace( ref.id, nativeProvider );
	nativeProvider->AddRef();
	return nativeProvider;
}

void UIAutomationProviderContext::invalidateProvider( AccessibilityNodeRef ref ) {
	UIAutomationProvider* providerToDetach{};
	{
		std::lock_guard<std::mutex> lock( mProviderMutex );
		auto source = mProviders.find( ref.source );
		if ( source == mProviders.end() )
			return;
		auto found = source->second.find( ref.id );
		if ( found == source->second.end() )
			return;
		providerToDetach = found->second;
		source->second.erase( found );
		if ( source->second.empty() )
			mProviders.erase( source );
	}
	providerToDetach->detach();
	{
		std::lock_guard<std::mutex> lock( mProviderMutex );
		mInvalidatedProviders.emplace_back( providerToDetach );
		mHasInvalidatedProviders.store( true, std::memory_order_release );
	}
}

void UIAutomationProviderContext::invalidateSource( AccessibilitySourceId sourceId ) {
	std::vector<UIAutomationProvider*> providers;
	{
		std::lock_guard<std::mutex> lock( mProviderMutex );
		auto source = mProviders.find( sourceId );
		if ( source == mProviders.end() )
			return;
		providers.reserve( source->second.size() );
		for ( const auto& entry : source->second )
			providers.emplace_back( entry.second );
		mProviders.erase( source );
	}
	for ( auto provider : providers ) {
		provider->detach();
	}
	std::lock_guard<std::mutex> lock( mProviderMutex );
	mInvalidatedProviders.insert( mInvalidatedProviders.end(), providers.begin(), providers.end() );
	mHasInvalidatedProviders.store( true, std::memory_order_release );
}

void UIAutomationProviderContext::disconnectInvalidatedProviders() {
	// Producers set the flag under the lock after appending, so a provider queued after this
	// check is drained by the next update.
	if ( !mHasInvalidatedProviders.load( std::memory_order_acquire ) )
		return;
	std::vector<UIAutomationProvider*> providers;
	{
		std::lock_guard<std::mutex> lock( mProviderMutex );
		providers.swap( mInvalidatedProviders );
		mHasInvalidatedProviders.store( false, std::memory_order_relaxed );
	}
	// Outgoing COM calls can re-enter an STA. Run these only after widget destruction finishes;
	// the detached providers already reject queries and retain their original cache reference.
	for ( auto* provider : providers ) {
		UiaDisconnectProvider( static_cast<IRawElementProviderSimple*>( provider ) );
		provider->Release();
	}
}

void UIAutomationProviderContext::pruneSource( AccessibilitySourceId sourceId ) {
	if ( !mManager || !isAlive() )
		return;
	std::lock_guard<std::mutex> lock( mProviderMutex );
	auto source = mProviders.find( sourceId );
	if ( source == mProviders.end() )
		return;
	for ( auto it = source->second.begin(); it != source->second.end(); ) {
		if ( mManager->isValid( { sourceId, it->first } ) ) {
			++it;
			continue;
		}
		it->second->detach();
		mInvalidatedProviders.emplace_back( it->second );
		mHasInvalidatedProviders.store( true, std::memory_order_release );
		it = source->second.erase( it );
	}
	if ( source->second.empty() )
		mProviders.erase( source );
}

void UIAutomationProviderContext::detach() {
	bool expected = true;
	if ( !mAlive.compare_exchange_strong( expected, false, std::memory_order_acq_rel ) )
		return;
	mWindow.store( nullptr, std::memory_order_release );
	mManager = nullptr;
	disconnectInvalidatedProviders();

	std::vector<std::shared_ptr<PendingRequest>> pending;
	{
		std::lock_guard<std::mutex> lock( mPendingMutex );
		pending.swap( mPendingRequests );
	}
	for ( const auto& request : pending )
		request->cancel( UIA_E_ELEMENTNOTAVAILABLE );

	std::vector<UIAutomationProvider*> providers;
	{
		std::lock_guard<std::mutex> lock( mProviderMutex );
		for ( auto& source : mProviders ) {
			for ( auto& entry : source.second )
				providers.emplace_back( entry.second );
		}
		mProviders.clear();
	}
	for ( auto provider : providers ) {
		UiaDisconnectProvider( static_cast<IRawElementProviderSimple*>( provider ) );
		provider->detach();
		provider->Release();
	}
}

}}} // namespace EE::UI::Uia

namespace EE { namespace UI {

namespace {

using namespace Uia;

class UIAutomationAccessibilityBackend final : public AccessibilityBackend {
  public:
	explicit UIAutomationAccessibilityBackend( AccessibilityManager& manager );

	~UIAutomationAccessibilityBackend() override;

	bool isAvailable() const override { return mContext && mContext->isAlive() && mWindow; }

	bool hasActiveClients() const override { return isAvailable() && mContext->hasActiveClients(); }

	void update() override;

	void onEvent( const AccessibilityPendingEvent& event ) override;

	void announce( const String& message, AccessibilityLive priority ) override;

	void onSourceInvalidated( AccessibilitySourceId sourceId ) override {
		if ( mContext )
			mContext->invalidateSource( sourceId );
	}

	void onSourceChanged( AccessibilitySourceId sourceId ) override {
		if ( mContext )
			mContext->pruneSource( sourceId );
	}

  private:
	static LRESULT CALLBACK windowSubclassProcedure( HWND window, UINT message, WPARAM wParam,
													 LPARAM lParam, UINT_PTR subclassId,
													 DWORD_PTR referenceData );

	void windowDestroyed();
	void raiseEvent( const AccessibilityPendingEvent& event );

	AccessibilityManager& mManager;
	std::shared_ptr<UIAutomationProviderContext> mContext;
	HWND mWindow{};
	UINT_PTR mSubclassId{};
	bool mSubclassInstalled{ false };
	bool mComInitialized{ false };
	std::vector<AccessibilityPendingEvent> mPendingEvents;
};

std::atomic<LONG> NextRuntimeScope{ 1 };

UIAutomationAccessibilityBackend::UIAutomationAccessibilityBackend(
	AccessibilityManager& manager ) :
	mManager( manager ) {
	auto scene = manager.getSceneNode();
	if ( !scene || !scene->getWindow() )
		return;
	mWindow = reinterpret_cast<HWND>( scene->getWindow()->getWindowHandler() );
	if ( !mWindow )
		return;
	mComInitialized = SUCCEEDED( CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED ) );
	const LONG runtimeScope = NextRuntimeScope.fetch_add( 1, std::memory_order_relaxed );
	mContext = std::make_shared<UIAutomationProviderContext>( manager, mWindow, runtimeScope );
	mSubclassId = reinterpret_cast<UINT_PTR>( this );
	if ( !SetWindowSubclass( mWindow, &UIAutomationAccessibilityBackend::windowSubclassProcedure,
							 mSubclassId, reinterpret_cast<DWORD_PTR>( this ) ) ) {
		mContext->detach();
		mContext.reset();
		mWindow = nullptr;
		return;
	}
	mSubclassInstalled = true;
}

UIAutomationAccessibilityBackend::~UIAutomationAccessibilityBackend() {
	if ( mSubclassInstalled && mWindow && IsWindow( mWindow ) )
		RemoveWindowSubclass( mWindow, &UIAutomationAccessibilityBackend::windowSubclassProcedure,
							  mSubclassId );
	mSubclassInstalled = false;
	if ( mContext )
		mContext->detach();
	mWindow = nullptr;
	if ( mComInitialized )
		CoUninitialize();
}

LRESULT CALLBACK UIAutomationAccessibilityBackend::windowSubclassProcedure(
	HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId,
	DWORD_PTR referenceData ) {
	auto backend = reinterpret_cast<UIAutomationAccessibilityBackend*>( referenceData );
	if ( !backend || subclassId != backend->mSubclassId )
		return DefSubclassProc( window, message, wParam, lParam );
	if ( message == accessibilityDispatchMessage() ) {
		if ( backend->mContext ) {
			backend->mContext->dispatchPendingRequests();
			backend->update();
		}
		return 0;
	}
	if ( message == WM_GETOBJECT && static_cast<LONG>( lParam ) == UiaRootObjectId &&
		 backend->isAvailable() ) {
		backend->mContext->clientObserved();
		UIAutomationProvider* root = backend->mContext->provider( backend->mManager.getRoot() );
		if ( root ) {
			const LRESULT result = UiaReturnRawElementProvider(
				window, wParam, lParam, static_cast<IRawElementProviderSimple*>( root ) );
			root->Release();
			return result;
		}
	}
	if ( message == WM_DESTROY )
		UiaReturnRawElementProvider( window, 0, 0, nullptr );
	const LRESULT result = DefSubclassProc( window, message, wParam, lParam );
	if ( message == WM_NCDESTROY ) {
		RemoveWindowSubclass( window, &UIAutomationAccessibilityBackend::windowSubclassProcedure,
							  subclassId );
		backend->windowDestroyed();
	}
	return result;
}

void UIAutomationAccessibilityBackend::windowDestroyed() {
	mSubclassInstalled = false;
	mWindow = nullptr;
	mPendingEvents.clear();
	if ( mContext )
		mContext->detach();
}

void UIAutomationAccessibilityBackend::update() {
	if ( mContext )
		mContext->disconnectInvalidatedProviders();
	if ( !isAvailable() ) {
		mPendingEvents.clear();
		return;
	}
	mContext->updateClientState();
	if ( !mContext->hasActiveClients() ) {
		mPendingEvents.clear();
		return;
	}
	auto pendingEvents = std::move( mPendingEvents );
	mPendingEvents.clear();
	bool focusChanged = false;
	for ( const auto& event : pendingEvents ) {
		if ( event.type == AccessibilityEvent::FocusChanged )
			focusChanged = true;
		else
			raiseEvent( event );
	}
	if ( focusChanged ) {
		auto focused = mManager.getKeyboardFocusedNode();
		if ( focused.isValid() )
			raiseEvent( { focused, {}, -1, AccessibilityEvent::FocusChanged } );
	}
}

void UIAutomationAccessibilityBackend::onEvent( const AccessibilityPendingEvent& event ) {
	if ( !mContext )
		return;
	if ( event.type == AccessibilityEvent::Destroyed && !event.related.isValid() ) {
		// A related-node event removes a tree edge, not necessarily the widget. Same-scene
		// reparenting preserves its identity; only final subtree invalidation detaches providers.
		mContext->invalidateProvider( event.ref );
		return;
	}
	if ( !mContext->hasActiveClients() )
		return;
	mPendingEvents.emplace_back( event );
}

namespace {

// UiaRaiseNotificationEvent exists from Windows 10 1709. Resolve it at runtime so the library
// still loads on older systems and with SDK headers that predate it.
using RaiseNotificationEventFunction = HRESULT( WINAPI* )( IRawElementProviderSimple*, int, int,
														   BSTR, BSTR );

RaiseNotificationEventFunction raiseNotificationEventFunction() {
	static const auto function = []() -> RaiseNotificationEventFunction {
		HMODULE module = GetModuleHandleW( L"uiautomationcore.dll" );
		return module ? reinterpret_cast<RaiseNotificationEventFunction>( reinterpret_cast<void*>(
							GetProcAddress( module, "UiaRaiseNotificationEvent" ) ) )
					  : nullptr;
	}();
	return function;
}

} // namespace

void UIAutomationAccessibilityBackend::announce( const String& message,
												 AccessibilityLive priority ) {
	auto raise = raiseNotificationEventFunction();
	if ( !raise || !mContext || !mContext->hasActiveClients() )
		return;
	UIAutomationProvider* root = mContext->provider( mManager.getRoot() );
	if ( !root )
		return;
	// NotificationKind_Other = 4; NotificationProcessing_ImportantAll = 0 for assertive and
	// NotificationProcessing_MostRecent = 3 for polite, so a stream of updates speaks the latest.
	constexpr int NotificationKindOther = 4;
	const int processing = priority == AccessibilityLive::Assertive ? 0 : 3;
	const auto wide = message.toWideString();
	BSTR display = SysAllocStringLen( wide.data(), static_cast<UINT>( wide.size() ) );
	BSTR activity = SysAllocString( L"eepp.announcement" );
	if ( display && activity )
		raise( static_cast<IRawElementProviderSimple*>( root ), NotificationKindOther, processing,
			   display, activity );
	SysFreeString( display );
	SysFreeString( activity );
	root->Release();
}

void UIAutomationAccessibilityBackend::raiseEvent( const AccessibilityPendingEvent& event ) {
	if ( !mContext || !mContext->hasActiveClients() )
		return;
	UIAutomationProvider* nativeProvider = mContext->provider( event.ref );
	if ( !nativeProvider )
		return;
	auto raisePropertyChanged = [&]( PROPERTYID propertyId ) {
		VARIANT oldValue;
		VARIANT newValue;
		VariantInit( &oldValue );
		VariantInit( &newValue );
		if ( SUCCEEDED( nativeProvider->GetPropertyValue( propertyId, &newValue ) ) &&
			 newValue.vt != VT_EMPTY ) {
			UiaRaiseAutomationPropertyChangedEvent(
				static_cast<IRawElementProviderSimple*>( nativeProvider ), propertyId, oldValue,
				newValue );
		}
		VariantClear( &oldValue );
		VariantClear( &newValue );
	};

	if ( event.type == AccessibilityEvent::NameChanged ) {
		raisePropertyChanged( UIA_NamePropertyId );
	} else if ( event.type == AccessibilityEvent::DescriptionChanged ) {
		raisePropertyChanged( UIA_HelpTextPropertyId );
	} else if ( event.type == AccessibilityEvent::ValueChanged ) {
		const auto info = mManager.getNodeInfo( event.ref, false, false );
		raisePropertyChanged( info.range.valid ? UIA_RangeValueValuePropertyId
											   : UIA_ValueValuePropertyId );
		if ( info.text.valid )
			UiaRaiseAutomationEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
									 UIA_Text_TextChangedEventId );
	} else if ( event.type == AccessibilityEvent::EnabledChanged ) {
		raisePropertyChanged( UIA_IsEnabledPropertyId );
	} else if ( event.type == AccessibilityEvent::VisibilityChanged ) {
		raisePropertyChanged( UIA_IsOffscreenPropertyId );
	} else if ( event.type == AccessibilityEvent::StateChanged ) {
		auto info = mManager.getNodeInfo( event.ref, false, false );
		if ( info.role != AccessibilityRole::CheckBox &&
			 info.role != AccessibilityRole::CheckMenuItem &&
			 info.role != AccessibilityRole::RadioButton &&
			 info.role != AccessibilityRole::RadioMenuItem &&
			 info.role != AccessibilityRole::ComboBox &&
			 info.role != AccessibilityRole::TreeItem ) {
			const AccessibilityNodeRef parent = mManager.getParent( event.ref );
			const auto parentInfo = mManager.getNodeInfo( parent, false, false );
			if ( parentInfo.role == AccessibilityRole::ComboBox ) {
				nativeProvider->Release();
				nativeProvider = mContext->provider( parent );
				if ( !nativeProvider )
					return;
				info = parentInfo;
			}
		}
		if ( info.role == AccessibilityRole::CheckBox ||
			 info.role == AccessibilityRole::CheckMenuItem )
			raisePropertyChanged( UIA_ToggleToggleStatePropertyId );
		else if ( info.role == AccessibilityRole::RadioButton ||
				  info.role == AccessibilityRole::RadioMenuItem )
			raisePropertyChanged( UIA_SelectionItemIsSelectedPropertyId );
		else if ( info.role == AccessibilityRole::ComboBox ||
				  info.role == AccessibilityRole::TreeItem )
			raisePropertyChanged( UIA_ExpandCollapseExpandCollapseStatePropertyId );
	} else if ( event.type == AccessibilityEvent::Created ||
				event.type == AccessibilityEvent::Destroyed ) {
		const auto runtimeId = runtimeIdValues( mContext->runtimeScope(), event.related );
		UiaRaiseStructureChangedEvent(
			static_cast<IRawElementProviderSimple*>( nativeProvider ),
			event.type == AccessibilityEvent::Created ? StructureChangeType_ChildAdded
													  : StructureChangeType_ChildRemoved,
			const_cast<int*>( runtimeId.data() ), static_cast<int>( runtimeId.size() ) );
	} else if ( event.type == AccessibilityEvent::ChildrenChanged ||
				event.type == AccessibilityEvent::ModelChanged ) {
		UiaRaiseStructureChangedEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
									   StructureChangeType_ChildrenInvalidated, nullptr, 0 );
	} else if ( event.type == AccessibilityEvent::FocusChanged ) {
		raisePropertyChanged( UIA_HasKeyboardFocusPropertyId );
		UiaRaiseAutomationEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
								 UIA_AutomationFocusChangedEventId );
	} else if ( event.type == AccessibilityEvent::SelectionChanged ) {
		const auto info = mManager.getNodeInfo( event.ref, false, false );
		if ( info.text.valid )
			UiaRaiseAutomationEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
									 UIA_Text_TextSelectionChangedEventId );
		else if ( isSelectionContainerRole( info.role ) ) {
			AccessibilityNodeRef selected;
			const auto children = mManager.getSelectedChildren( event.ref );
			if ( !children.empty() )
				selected = children.front();
			if ( selected.isValid() ) {
				nativeProvider->Release();
				nativeProvider = mContext->provider( selected );
				if ( !nativeProvider )
					return;
				raisePropertyChanged( UIA_SelectionItemIsSelectedPropertyId );
				UiaRaiseAutomationEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
										 UIA_SelectionItem_ElementSelectedEventId );
			} else {
				UiaRaiseAutomationEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
										 UIA_Selection_InvalidatedEventId );
			}
		} else {
			raisePropertyChanged( UIA_SelectionItemIsSelectedPropertyId );
			UiaRaiseAutomationEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
									 UIA_SelectionItem_ElementSelectedEventId );
		}
	} else {
		UiaRaiseAutomationEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
								 UIA_LayoutInvalidatedEventId );
	}
	nativeProvider->Release();
}

} // namespace

std::unique_ptr<AccessibilityBackend> createAccessibilityBackend( AccessibilityManager& manager ) {
	return std::make_unique<UIAutomationAccessibilityBackend>( manager );
}

}} // namespace EE::UI

#endif
