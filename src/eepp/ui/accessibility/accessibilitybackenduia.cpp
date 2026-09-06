#include "accessibilitybackend.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/window.hpp>
#include <uiautomationclient.h>
#include <uiautomationcore.h>
#include <uiautomationcoreapi.h>
#include <windows.h>

namespace EE { namespace UI {

namespace {

class UIAutomationProvider;

class UIAutomationAccessibilityBackend final : public AccessibilityBackend {
  public:
	explicit UIAutomationAccessibilityBackend( AccessibilityManager& manager );

	~UIAutomationAccessibilityBackend();

	bool isAvailable() const { return mWindow != nullptr; }

	bool hasActiveClients() const { return isAvailable() && UiaClientsAreListening(); }

	void onEvent( const AccessibilityPendingEvent& event );

	AccessibilityManager& manager() { return mManager; }

	HWND window() const { return mWindow; }

	UIAutomationProvider* provider( AccessibilityNodeRef ref );

	void invalidateProvider( AccessibilityNodeRef ref );

  private:
	static LRESULT CALLBACK windowProcedure( HWND window, UINT message, WPARAM wParam,
											 LPARAM lParam );

	AccessibilityManager& mManager;
	HWND mWindow{};
	WNDPROC mPreviousWindowProcedure{};
	UnorderedMap<AccessibilitySourceId, UnorderedMap<Uint64, UIAutomationProvider*>> mProviders;
};

class UIAutomationProvider final : public IRawElementProviderSimple,
								   public IRawElementProviderFragment,
								   public IRawElementProviderFragmentRoot,
								   public IInvokeProvider,
								   public IToggleProvider,
								   public ISelectionItemProvider,
								   public IValueProvider,
								   public IRangeValueProvider,
								   public IExpandCollapseProvider,
								   public IScrollItemProvider {
  public:
	UIAutomationProvider( UIAutomationAccessibilityBackend& backend, AccessibilityNodeRef ref ) :
		mBackend( &backend ), mRef( ref ) {}

	void detach() { mBackend = nullptr; }

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID interfaceId, void** object ) {
		if ( !object )
			return E_INVALIDARG;
		*object = nullptr;
		auto actions = info().actions;
		if ( interfaceId == __uuidof( IUnknown ) ||
			 interfaceId == __uuidof( IRawElementProviderSimple ) ) {
			*object = static_cast<IRawElementProviderSimple*>( this );
		} else if ( interfaceId == __uuidof( IRawElementProviderFragment ) ) {
			*object = static_cast<IRawElementProviderFragment*>( this );
		} else if ( interfaceId == __uuidof( IRawElementProviderFragmentRoot ) ) {
			*object = static_cast<IRawElementProviderFragmentRoot*>( this );
		} else if ( interfaceId == __uuidof( IInvokeProvider ) &&
					actions & accessibilityActionMask( AccessibilityAction::Press ) ) {
			*object = static_cast<IInvokeProvider*>( this );
		} else if ( interfaceId == __uuidof( IToggleProvider ) &&
					actions & accessibilityActionMask( AccessibilityAction::Toggle ) ) {
			*object = static_cast<IToggleProvider*>( this );
		} else if ( interfaceId == __uuidof( ISelectionItemProvider ) &&
					( actions & accessibilityActionMask( AccessibilityAction::Select ) ||
					  hasState( info().states, AccessibilityState::Selected ) ) ) {
			*object = static_cast<ISelectionItemProvider*>( this );
		} else if ( interfaceId == __uuidof( IValueProvider ) &&
					( actions & accessibilityActionMask( AccessibilityAction::SetText ) ||
					  info().role == AccessibilityRole::ComboBox ) ) {
			*object = static_cast<IValueProvider*>( this );
		} else if ( interfaceId == __uuidof( IRangeValueProvider ) && info().range.valid ) {
			*object = static_cast<IRangeValueProvider*>( this );
		} else if ( interfaceId == __uuidof( IExpandCollapseProvider ) &&
					( actions & accessibilityActionMask( AccessibilityAction::Expand ) ||
					  actions & accessibilityActionMask( AccessibilityAction::Collapse ) ) ) {
			*object = static_cast<IExpandCollapseProvider*>( this );
		} else if ( interfaceId == __uuidof( IScrollItemProvider ) &&
					actions & accessibilityActionMask( AccessibilityAction::ScrollTo ) ) {
			*object = static_cast<IScrollItemProvider*>( this );
		}
		if ( !*object )
			return E_NOINTERFACE;
		AddRef();
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement( &mReferences ); }

	ULONG STDMETHODCALLTYPE Release() {
		ULONG references = InterlockedDecrement( &mReferences );
		if ( references == 0 )
			delete this;
		return references;
	}

	HRESULT STDMETHODCALLTYPE get_ProviderOptions( ProviderOptions* options ) {
		if ( !options )
			return E_INVALIDARG;
		*options = ProviderOptions_ServerSideProvider;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetPatternProvider( PATTERNID patternId, IUnknown** provider ) {
		if ( !provider )
			return E_INVALIDARG;
		*provider = nullptr;
		if ( patternId == UIA_InvokePatternId )
			return QueryInterface( __uuidof( IInvokeProvider ),
								   reinterpret_cast<void**>( provider ) );
		if ( patternId == UIA_TogglePatternId )
			return QueryInterface( __uuidof( IToggleProvider ),
								   reinterpret_cast<void**>( provider ) );
		if ( patternId == UIA_SelectionItemPatternId )
			return QueryInterface( __uuidof( ISelectionItemProvider ),
								   reinterpret_cast<void**>( provider ) );
		if ( patternId == UIA_ValuePatternId )
			return QueryInterface( __uuidof( IValueProvider ),
								   reinterpret_cast<void**>( provider ) );
		if ( patternId == UIA_RangeValuePatternId )
			return QueryInterface( __uuidof( IRangeValueProvider ),
								   reinterpret_cast<void**>( provider ) );
		if ( patternId == UIA_ExpandCollapsePatternId )
			return QueryInterface( __uuidof( IExpandCollapseProvider ),
								   reinterpret_cast<void**>( provider ) );
		if ( patternId == UIA_ScrollItemPatternId )
			return QueryInterface( __uuidof( IScrollItemProvider ),
								   reinterpret_cast<void**>( provider ) );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetPropertyValue( PROPERTYID propertyId, VARIANT* value ) {
		if ( !value )
			return E_INVALIDARG;
		VariantInit( value );
		auto nodeInfo = info();
		if ( propertyId == UIA_NamePropertyId ) {
			value->vt = VT_BSTR;
			value->bstrVal = SysAllocString( nodeInfo.name.toWideString().c_str() );
		} else if ( propertyId == UIA_HelpTextPropertyId ) {
			value->vt = VT_BSTR;
			value->bstrVal = SysAllocString( nodeInfo.description.toWideString().c_str() );
		} else if ( propertyId == UIA_ControlTypePropertyId ) {
			value->vt = VT_I4;
			value->lVal = controlType( nodeInfo.role );
		} else if ( propertyId == UIA_IsEnabledPropertyId ) {
			value->vt = VT_BOOL;
			value->boolVal = hasState( nodeInfo.states, AccessibilityState::Enabled )
								 ? VARIANT_TRUE
								 : VARIANT_FALSE;
		} else if ( propertyId == UIA_HasKeyboardFocusPropertyId ) {
			value->vt = VT_BOOL;
			value->boolVal = hasState( nodeInfo.states, AccessibilityState::Focused )
								 ? VARIANT_TRUE
								 : VARIANT_FALSE;
		} else if ( propertyId == UIA_IsKeyboardFocusablePropertyId ) {
			value->vt = VT_BOOL;
			value->boolVal = hasState( nodeInfo.states, AccessibilityState::Focusable )
								 ? VARIANT_TRUE
								 : VARIANT_FALSE;
		} else if ( propertyId == UIA_AutomationIdPropertyId ) {
			value->vt = VT_BSTR;
			value->bstrVal = SysAllocString(
				String::fromUtf8( String::toString( mRef.id ) ).toWideString().c_str() );
		} else if ( propertyId == UIA_FrameworkIdPropertyId ) {
			value->vt = VT_BSTR;
			value->bstrVal = SysAllocString( L"eepp" );
		} else if ( propertyId == UIA_IsControlElementPropertyId ||
					propertyId == UIA_IsContentElementPropertyId ) {
			value->vt = VT_BOOL;
			value->boolVal =
				nodeInfo.role != AccessibilityRole::None ? VARIANT_TRUE : VARIANT_FALSE;
		} else if ( propertyId == UIA_IsOffscreenPropertyId ) {
			value->vt = VT_BOOL;
			value->boolVal = hasState( nodeInfo.states, AccessibilityState::Showing )
								 ? VARIANT_FALSE
								 : VARIANT_TRUE;
		} else if ( propertyId == UIA_ValueValuePropertyId ) {
			value->vt = VT_BSTR;
			value->bstrVal = SysAllocString( nodeInfo.value.toWideString().c_str() );
		} else if ( propertyId == UIA_RangeValueValuePropertyId && nodeInfo.range.valid ) {
			value->vt = VT_R8;
			String::fromString( value->dblVal, nodeInfo.value.toUtf8() );
		} else if ( propertyId == UIA_ToggleToggleStatePropertyId ) {
			value->vt = VT_I4;
			value->lVal = hasState( nodeInfo.states, AccessibilityState::Checked )
							  ? ToggleState_On
							  : ToggleState_Off;
		} else if ( propertyId == UIA_SelectionItemIsSelectedPropertyId ) {
			value->vt = VT_BOOL;
			value->boolVal = hasState( nodeInfo.states, AccessibilityState::Selected )
								 ? VARIANT_TRUE
								 : VARIANT_FALSE;
		} else if ( propertyId == UIA_ExpandCollapseExpandCollapseStatePropertyId ) {
			value->vt = VT_I4;
			value->lVal = hasState( nodeInfo.states, AccessibilityState::Expanded )
							  ? ExpandCollapseState_Expanded
							  : ExpandCollapseState_Collapsed;
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE get_HostRawElementProvider( IRawElementProviderSimple** provider ) {
		if ( !provider )
			return E_INVALIDARG;
		*provider = nullptr;
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		return mRef == manager().getRoot() ? UiaHostProviderFromHwnd( mBackend->window(), provider )
										   : S_OK;
	}

	HRESULT STDMETHODCALLTYPE Navigate( NavigateDirection direction,
										IRawElementProviderFragment** provider ) {
		if ( !provider )
			return E_INVALIDARG;
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		*provider = nullptr;
		AccessibilityNodeRef target;
		if ( direction == NavigateDirection_Parent ) {
			target = manager().getParent( mRef );
		} else if ( direction == NavigateDirection_FirstChild ) {
			target = manager().getChild( mRef, 0 );
		} else if ( direction == NavigateDirection_LastChild ) {
			auto count = manager().getChildCount( mRef );
			if ( count )
				target = manager().getChild( mRef, count - 1 );
		} else {
			auto parent = manager().getParent( mRef );
			for ( size_t i = 0; i < manager().getChildCount( parent ); ++i ) {
				if ( manager().getChild( parent, i ) == mRef ) {
					if ( direction == NavigateDirection_NextSibling )
						target = manager().getChild( parent, i + 1 );
					else if ( direction == NavigateDirection_PreviousSibling && i > 0 )
						target = manager().getChild( parent, i - 1 );
					break;
				}
			}
		}
		if ( target.isValid() )
			*provider = static_cast<IRawElementProviderFragment*>( mBackend->provider( target ) );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetRuntimeId( SAFEARRAY** runtimeId ) {
		if ( !runtimeId )
			return E_INVALIDARG;
		int values[] = { UiaAppendRuntimeId, static_cast<int>( mRef.source & 0x7fffffff ),
						 static_cast<int>( mRef.id & 0x7fffffff ) };
		*runtimeId = SafeArrayCreateVector( VT_I4, 0, 3 );
		if ( !*runtimeId )
			return E_OUTOFMEMORY;
		for ( LONG index = 0; index < 3; ++index )
			SafeArrayPutElement( *runtimeId, &index, &values[index] );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE get_BoundingRectangle( UiaRect* rectangle ) {
		if ( !rectangle )
			return E_INVALIDARG;
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto bounds = info().bounds;
		auto position = mBackend->manager().getSceneNode()->getWindow()->getPosition();
		rectangle->left = position.x + bounds.Left;
		rectangle->top = position.y + bounds.Top;
		rectangle->width = bounds.getWidth();
		rectangle->height = bounds.getHeight();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots( SAFEARRAY** roots ) {
		if ( !roots )
			return E_INVALIDARG;
		*roots = nullptr;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetFocus() {
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		return manager().performAction( mRef, { AccessibilityAction::Focus, {} } )
				   ? S_OK
				   : UIA_E_ELEMENTNOTENABLED;
	}

	HRESULT STDMETHODCALLTYPE get_FragmentRoot( IRawElementProviderFragmentRoot** root ) {
		if ( !root )
			return E_INVALIDARG;
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		*root = mBackend->provider( manager().getRoot() );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE ElementProviderFromPoint( double x, double y,
														IRawElementProviderFragment** provider ) {
		if ( !provider )
			return E_INVALIDARG;
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto position = manager().getSceneNode()->getWindow()->getPosition();
		auto ref = manager().hitTest( Math::Vector2f( x - position.x, y - position.y ) );
		*provider = ref.isValid()
						? static_cast<IRawElementProviderFragment*>( mBackend->provider( ref ) )
						: nullptr;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetFocus( IRawElementProviderFragment** provider ) {
		if ( !provider )
			return E_INVALIDARG;
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto ref = manager().getKeyboardFocusedNode();
		*provider = ref.isValid()
						? static_cast<IRawElementProviderFragment*>( mBackend->provider( ref ) )
						: nullptr;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Invoke() { return perform( AccessibilityAction::Press ); }

	HRESULT STDMETHODCALLTYPE Toggle() { return perform( AccessibilityAction::Toggle ); }

	HRESULT STDMETHODCALLTYPE get_ToggleState( ToggleState* state ) {
		if ( !state )
			return E_INVALIDARG;
		*state = hasState( info().states, AccessibilityState::Checked ) ? ToggleState_On
																		: ToggleState_Off;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Select() { return perform( AccessibilityAction::Select ); }

	HRESULT STDMETHODCALLTYPE AddToSelection() { return Select(); }

	HRESULT STDMETHODCALLTYPE RemoveFromSelection() { return E_NOTIMPL; }

	HRESULT STDMETHODCALLTYPE get_IsSelected( BOOL* selected ) {
		if ( !selected )
			return E_INVALIDARG;
		*selected = hasState( info().states, AccessibilityState::Selected ) ? TRUE : FALSE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE get_SelectionContainer( IRawElementProviderSimple** provider ) {
		if ( !provider )
			return E_INVALIDARG;
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		auto parent = manager().getParent( mRef );
		*provider = parent.isValid()
						? static_cast<IRawElementProviderSimple*>( mBackend->provider( parent ) )
						: nullptr;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetValue( LPCWSTR value ) {
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		return manager().performAction(
				   mRef, { AccessibilityAction::SetText, String( value ? value : L"" ) } )
				   ? S_OK
				   : E_FAIL;
	}

	HRESULT STDMETHODCALLTYPE get_Value( BSTR* value ) {
		if ( !value )
			return E_INVALIDARG;
		*value = SysAllocString( info().value.toWideString().c_str() );
		return *value ? S_OK : E_OUTOFMEMORY;
	}

	HRESULT STDMETHODCALLTYPE get_IsReadOnly( BOOL* readOnly ) {
		if ( !readOnly )
			return E_INVALIDARG;
		auto actions = info().actions;
		*readOnly = actions & ( accessibilityActionMask( AccessibilityAction::SetText ) |
								accessibilityActionMask( AccessibilityAction::SetValue ) )
						? FALSE
						: TRUE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Expand() { return perform( AccessibilityAction::Expand ); }

	HRESULT STDMETHODCALLTYPE Collapse() { return perform( AccessibilityAction::Collapse ); }

	HRESULT STDMETHODCALLTYPE get_ExpandCollapseState( ExpandCollapseState* state ) {
		if ( !state )
			return E_INVALIDARG;
		*state = hasState( info().states, AccessibilityState::Expanded )
					 ? ExpandCollapseState_Expanded
					 : ExpandCollapseState_Collapsed;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE ScrollIntoView() { return perform( AccessibilityAction::ScrollTo ); }

	HRESULT STDMETHODCALLTYPE SetValue( double value ) {
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		return manager().performAction(
				   mRef, { AccessibilityAction::SetValue, String( String::toString( value ) ) } )
				   ? S_OK
				   : E_FAIL;
	}

	HRESULT STDMETHODCALLTYPE get_Value( double* value ) {
		if ( !value )
			return E_INVALIDARG;
		return String::fromString( *value, info().value.toUtf8() ) ? S_OK : E_FAIL;
	}

	HRESULT STDMETHODCALLTYPE get_Maximum( double* value ) {
		return rangeValue( value, &AccessibilityRangeInfo::maximum );
	}

	HRESULT STDMETHODCALLTYPE get_Minimum( double* value ) {
		return rangeValue( value, &AccessibilityRangeInfo::minimum );
	}

	HRESULT STDMETHODCALLTYPE get_LargeChange( double* value ) {
		return rangeValue( value, &AccessibilityRangeInfo::largeChange );
	}

	HRESULT STDMETHODCALLTYPE get_SmallChange( double* value ) {
		return rangeValue( value, &AccessibilityRangeInfo::smallChange );
	}

  private:
	volatile LONG mReferences{ 1 };
	UIAutomationAccessibilityBackend* mBackend;
	AccessibilityNodeRef mRef;

	AccessibilityManager& manager() { return mBackend->manager(); }

	AccessibilityNodeInfo info() {
		return mBackend ? manager().getNodeInfo( mRef ) : AccessibilityNodeInfo{};
	}

	static bool hasState( AccessibilityState states, AccessibilityState state ) {
		return static_cast<Uint64>( states ) & static_cast<Uint64>( state );
	}

	static CONTROLTYPEID controlType( AccessibilityRole role ) {
		switch ( role ) {
			case AccessibilityRole::Application:
			case AccessibilityRole::Window:
				return UIA_WindowControlTypeId;
			case AccessibilityRole::Button:
				return UIA_ButtonControlTypeId;
			case AccessibilityRole::CheckBox:
				return UIA_CheckBoxControlTypeId;
			case AccessibilityRole::RadioButton:
				return UIA_RadioButtonControlTypeId;
			case AccessibilityRole::Label:
			case AccessibilityRole::Text:
				return UIA_TextControlTypeId;
			case AccessibilityRole::TextBox:
				return UIA_EditControlTypeId;
			case AccessibilityRole::Image:
				return UIA_ImageControlTypeId;
			case AccessibilityRole::ComboBox:
				return UIA_ComboBoxControlTypeId;
			case AccessibilityRole::Slider:
				return UIA_SliderControlTypeId;
			case AccessibilityRole::SpinButton:
				return UIA_SpinnerControlTypeId;
			case AccessibilityRole::ProgressBar:
				return UIA_ProgressBarControlTypeId;
			case AccessibilityRole::Tab:
				return UIA_TabItemControlTypeId;
			case AccessibilityRole::TabList:
				return UIA_TabControlTypeId;
			case AccessibilityRole::List:
				return UIA_ListControlTypeId;
			case AccessibilityRole::ListItem:
				return UIA_ListItemControlTypeId;
			case AccessibilityRole::Table:
				return UIA_DataGridControlTypeId;
			case AccessibilityRole::Row:
			case AccessibilityRole::Cell:
				return UIA_DataItemControlTypeId;
			case AccessibilityRole::Tree:
				return UIA_TreeControlTypeId;
			case AccessibilityRole::TreeItem:
				return UIA_TreeItemControlTypeId;
			case AccessibilityRole::MenuBar:
				return UIA_MenuBarControlTypeId;
			case AccessibilityRole::Menu:
				return UIA_MenuControlTypeId;
			case AccessibilityRole::MenuItem:
			case AccessibilityRole::CheckMenuItem:
			case AccessibilityRole::RadioMenuItem:
				return UIA_MenuItemControlTypeId;
			default:
				return UIA_GroupControlTypeId;
		}
	}

	HRESULT perform( AccessibilityAction action ) {
		if ( !mBackend )
			return UIA_E_ELEMENTNOTAVAILABLE;
		return manager().performAction( mRef, { action, {} } ) ? S_OK : E_FAIL;
	}

	HRESULT rangeValue( double* value, double AccessibilityRangeInfo::* member ) {
		if ( !value )
			return E_INVALIDARG;
		auto range = info().range;
		if ( !range.valid )
			return E_FAIL;
		*value = range.*member;
		return S_OK;
	}
};

constexpr wchar_t BackendProperty[] = L"eepp.AccessibilityBackend";

UIAutomationAccessibilityBackend::UIAutomationAccessibilityBackend(
	AccessibilityManager& manager ) :
	mManager( manager ) {
	auto scene = manager.getSceneNode();
	if ( !scene || !scene->getWindow() )
		return;
	mWindow = reinterpret_cast<HWND>( scene->getWindow()->getWindowHandler() );
	if ( !mWindow || GetPropW( mWindow, BackendProperty ) ||
		 !SetPropW( mWindow, BackendProperty, this ) ) {
		mWindow = nullptr;
		return;
	}
	mPreviousWindowProcedure = reinterpret_cast<WNDPROC>( SetWindowLongPtrW(
		mWindow, GWLP_WNDPROC,
		reinterpret_cast<LONG_PTR>( &UIAutomationAccessibilityBackend::windowProcedure ) ) );
	if ( !mPreviousWindowProcedure ) {
		RemovePropW( mWindow, BackendProperty );
		mWindow = nullptr;
	}
}

UIAutomationAccessibilityBackend::~UIAutomationAccessibilityBackend() {
	if ( mWindow && IsWindow( mWindow ) && GetPropW( mWindow, BackendProperty ) == this ) {
		if ( reinterpret_cast<WNDPROC>( GetWindowLongPtrW( mWindow, GWLP_WNDPROC ) ) ==
			 &UIAutomationAccessibilityBackend::windowProcedure ) {
			SetWindowLongPtrW( mWindow, GWLP_WNDPROC,
							   reinterpret_cast<LONG_PTR>( mPreviousWindowProcedure ) );
		}
		RemovePropW( mWindow, BackendProperty );
	}
	for ( auto& source : mProviders ) {
		for ( auto& provider : source.second ) {
			provider.second->detach();
			provider.second->Release();
		}
	}
}

LRESULT CALLBACK UIAutomationAccessibilityBackend::windowProcedure( HWND window, UINT message,
																	WPARAM wParam, LPARAM lParam ) {
	auto backend =
		static_cast<UIAutomationAccessibilityBackend*>( GetPropW( window, BackendProperty ) );
	if ( backend && message == WM_GETOBJECT && static_cast<LONG>( lParam ) == UiaRootObjectId ) {
		auto root = backend->provider( backend->manager().getRoot() );
		LRESULT result = UiaReturnRawElementProvider(
			window, wParam, lParam, static_cast<IRawElementProviderSimple*>( root ) );
		root->Release();
		return result;
	}
	return backend ? CallWindowProcW( backend->mPreviousWindowProcedure, window, message, wParam,
									  lParam )
				   : DefWindowProcW( window, message, wParam, lParam );
}

UIAutomationProvider* UIAutomationAccessibilityBackend::provider( AccessibilityNodeRef ref ) {
	if ( !ref.isValid() )
		return nullptr;
	auto& providers = mProviders[ref.source];
	auto found = providers.find( ref.id );
	if ( found != providers.end() ) {
		found->second->AddRef();
		return found->second;
	}
	auto nativeProvider = new UIAutomationProvider( *this, ref );
	providers.emplace( ref.id, nativeProvider );
	nativeProvider->AddRef();
	return nativeProvider;
}

void UIAutomationAccessibilityBackend::invalidateProvider( AccessibilityNodeRef ref ) {
	auto source = mProviders.find( ref.source );
	if ( source == mProviders.end() )
		return;
	auto found = source->second.find( ref.id );
	if ( found == source->second.end() )
		return;
	found->second->detach();
	found->second->Release();
	source->second.erase( found );
	if ( source->second.empty() )
		mProviders.erase( source );
}

void UIAutomationAccessibilityBackend::onEvent( const AccessibilityPendingEvent& event ) {
	auto nativeProvider = provider( event.ref );
	if ( !nativeProvider )
		return;
	auto raisePropertyChanged = [&]( PROPERTYID propertyId ) {
		VARIANT oldValue;
		VARIANT newValue;
		VariantInit( &oldValue );
		VariantInit( &newValue );
		if ( SUCCEEDED( nativeProvider->GetPropertyValue( propertyId, &newValue ) ) &&
			 newValue.vt != VT_EMPTY )
			UiaRaiseAutomationPropertyChangedEvent(
				static_cast<IRawElementProviderSimple*>( nativeProvider ), propertyId, oldValue,
				newValue );
		VariantClear( &newValue );
	};
	if ( event.type == AccessibilityEvent::NameChanged ) {
		raisePropertyChanged( UIA_NamePropertyId );
	} else if ( event.type == AccessibilityEvent::DescriptionChanged ) {
		raisePropertyChanged( UIA_HelpTextPropertyId );
	} else if ( event.type == AccessibilityEvent::ValueChanged ) {
		raisePropertyChanged( manager().getNodeInfo( event.ref ).range.valid
								  ? UIA_RangeValueValuePropertyId
								  : UIA_ValueValuePropertyId );
	} else if ( event.type == AccessibilityEvent::EnabledChanged ) {
		raisePropertyChanged( UIA_IsEnabledPropertyId );
	} else if ( event.type == AccessibilityEvent::VisibilityChanged ) {
		raisePropertyChanged( UIA_IsOffscreenPropertyId );
	} else if ( event.type == AccessibilityEvent::StateChanged ) {
		auto info = manager().getNodeInfo( event.ref );
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
				event.type == AccessibilityEvent::Destroyed ||
				event.type == AccessibilityEvent::ChildrenChanged ) {
		StructureChangeType changeType = StructureChangeType_ChildrenInvalidated;
		if ( event.type == AccessibilityEvent::Created )
			changeType = StructureChangeType_ChildrenBulkAdded;
		else if ( event.type == AccessibilityEvent::Destroyed )
			changeType = StructureChangeType_ChildrenBulkRemoved;
		UiaRaiseStructureChangedEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
									   changeType, nullptr, 0 );
	} else {
		EVENTID eventId = UIA_LayoutInvalidatedEventId;
		if ( event.type == AccessibilityEvent::FocusChanged )
			eventId = UIA_AutomationFocusChangedEventId;
		else if ( event.type == AccessibilityEvent::SelectionChanged ) {
			raisePropertyChanged( UIA_SelectionItemIsSelectedPropertyId );
			eventId = UIA_SelectionItem_ElementSelectedEventId;
		}
		UiaRaiseAutomationEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
								 eventId );
	}
	nativeProvider->Release();
	if ( event.type == AccessibilityEvent::Destroyed )
		invalidateProvider( event.related.isValid() ? event.related : event.ref );
}

} // namespace

std::unique_ptr<AccessibilityBackend> createAccessibilityBackend( AccessibilityManager& manager ) {
	return std::make_unique<UIAutomationAccessibilityBackend>( manager );
}

}} // namespace EE::UI

#endif
