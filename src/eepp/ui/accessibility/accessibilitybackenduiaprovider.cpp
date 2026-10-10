#include "accessibilitybackenduia.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace UI { namespace Uia {

CONTROLTYPEID controlType( AccessibilityRole role ) {
	switch ( role ) {
		case AccessibilityRole::Application:
		case AccessibilityRole::Window:
		case AccessibilityRole::Dialog:
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
		case AccessibilityRole::TabPanel:
			return UIA_PaneControlTypeId;
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
		case AccessibilityRole::Group:
		case AccessibilityRole::None:
		default:
			return UIA_GroupControlTypeId;
	}
}

bool isSelectionContainerRole( AccessibilityRole role ) {
	return role == AccessibilityRole::TabList || role == AccessibilityRole::List ||
		   role == AccessibilityRole::Table || role == AccessibilityRole::Tree;
}

HRESULT setBstr( VARIANT* value, const String& string ) {
	const auto wide = string.toWideString();
	value->vt = VT_BSTR;
	value->bstrVal = SysAllocStringLen( wide.data(), static_cast<UINT>( wide.size() ) );
	return value->bstrVal ? S_OK : E_OUTOFMEMORY;
}

HRESULT setNotSupported( VARIANT* value ) {
	IUnknown* reserved{};
	const HRESULT result = UiaGetReservedNotSupportedValue( &reserved );
	if ( FAILED( result ) )
		return result;
	value->vt = VT_UNKNOWN;
	value->punkVal = reserved;
	return S_OK;
}

std::vector<int> runtimeIdValues( LONG scope, AccessibilityNodeRef ref ) {
	return { UiaAppendRuntimeId,
			 scope,
			 static_cast<int>( ref.source & 0xffffffffu ),
			 static_cast<int>( ( ref.source >> 32u ) & 0xffffffffu ),
			 static_cast<int>( ref.id & 0xffffffffu ),
			 static_cast<int>( ( ref.id >> 32u ) & 0xffffffffu ) };
}

SAFEARRAY* safeArrayFromInts( const std::vector<int>& values ) {
	SAFEARRAY* array = SafeArrayCreateVector( VT_I4, 0, static_cast<ULONG>( values.size() ) );
	if ( !array )
		return nullptr;
	for ( LONG index = 0; index < static_cast<LONG>( values.size() ); ++index ) {
		int value = values[static_cast<size_t>( index )];
		if ( FAILED( SafeArrayPutElement( array, &index, &value ) ) ) {
			SafeArrayDestroy( array );
			return nullptr;
		}
	}
	return array;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::QueryInterface( REFIID interfaceId,
																void** object ) {
	if ( !object )
		return E_INVALIDARG;
	*object = nullptr;
	if ( interfaceId == __uuidof( IUnknown ) ||
		 interfaceId == __uuidof( IRawElementProviderSimple ) ) {
		*object = static_cast<IRawElementProviderSimple*>( this );
	} else if ( interfaceId == __uuidof( IRawElementProviderFragment ) ) {
		*object = static_cast<IRawElementProviderFragment*>( this );
	} else if ( interfaceId == __uuidof( IRawElementProviderFragmentRoot ) && mRoot ) {
		*object = static_cast<IRawElementProviderFragmentRoot*>( this );
	} else if ( interfaceId == __uuidof( IRawElementProviderAdviseEvents ) && mRoot ) {
		*object = static_cast<IRawElementProviderAdviseEvents*>( this );
	} else {
		// COM probes for marshalling/agility must not enqueue a UI-thread query.
		if ( interfaceId != __uuidof( IInvokeProvider ) &&
			 interfaceId != __uuidof( IToggleProvider ) &&
			 interfaceId != __uuidof( ISelectionProvider ) &&
			 interfaceId != __uuidof( ISelectionItemProvider ) &&
			 interfaceId != __uuidof( IValueProvider ) &&
			 interfaceId != __uuidof( IRangeValueProvider ) &&
			 interfaceId != __uuidof( IExpandCollapseProvider ) &&
			 interfaceId != __uuidof( IScrollItemProvider ) &&
			 interfaceId != __uuidof( ITextProvider ) )
			return E_NOINTERFACE;
		AccessibilityNodeInfo nodeInfo;
		if ( FAILED( info( nodeInfo, false ) ) )
			return E_NOINTERFACE;
		if ( interfaceId == __uuidof( IInvokeProvider ) &&
			 hasAction( nodeInfo.actions, AccessibilityAction::Press ) )
			*object = static_cast<IInvokeProvider*>( this );
		else if ( interfaceId == __uuidof( IToggleProvider ) &&
				  hasAction( nodeInfo.actions, AccessibilityAction::Toggle ) )
			*object = static_cast<IToggleProvider*>( this );
		else if ( interfaceId == __uuidof( ISelectionProvider ) &&
				  isSelectionContainerRole( nodeInfo.role ) )
			*object = static_cast<ISelectionProvider*>( this );
		else if ( interfaceId == __uuidof( ISelectionItemProvider ) &&
				  ( hasAction( nodeInfo.actions, AccessibilityAction::Select ) ||
					hasState( nodeInfo.states, AccessibilityState::Selected ) ) )
			*object = static_cast<ISelectionItemProvider*>( this );
		else if ( interfaceId == __uuidof( IValueProvider ) &&
				  ( hasAction( nodeInfo.actions, AccessibilityAction::SetText ) ||
					nodeInfo.role == AccessibilityRole::ComboBox ) )
			*object = static_cast<IValueProvider*>( this );
		else if ( interfaceId == __uuidof( IRangeValueProvider ) && nodeInfo.range.valid )
			*object = static_cast<IRangeValueProvider*>( this );
		else if ( interfaceId == __uuidof( IExpandCollapseProvider ) &&
				  ( hasAction( nodeInfo.actions, AccessibilityAction::Expand ) ||
					hasAction( nodeInfo.actions, AccessibilityAction::Collapse ) ) )
			*object = static_cast<IExpandCollapseProvider*>( this );
		else if ( interfaceId == __uuidof( IScrollItemProvider ) &&
				  hasAction( nodeInfo.actions, AccessibilityAction::ScrollTo ) )
			*object = static_cast<IScrollItemProvider*>( this );
		else if ( interfaceId == __uuidof( ITextProvider ) && nodeInfo.text.valid &&
				  !hasState( nodeInfo.states, AccessibilityState::Protected ) )
			*object = static_cast<ITextProvider*>( this );
	}
	if ( !*object )
		return E_NOINTERFACE;
	AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE UIAutomationProvider::Release() {
	const ULONG references = mReferences.fetch_sub( 1, std::memory_order_acq_rel ) - 1;
	if ( references == 0 )
		delete this;
	return references;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::get_ProviderOptions( ProviderOptions* options ) {
	if ( !options )
		return E_INVALIDARG;
	if ( unavailable() )
		return UIA_E_ELEMENTNOTAVAILABLE;
	*options = static_cast<ProviderOptions>( ProviderOptions_ServerSideProvider |
											 ProviderOptions_ProviderOwnsSetFocus |
											 ProviderOptions_UseComThreading );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::GetPatternProvider( PATTERNID patternId,
																	IUnknown** provider ) {
	if ( !provider )
		return E_INVALIDARG;
	*provider = nullptr;
	HRESULT result = E_NOINTERFACE;
	if ( patternId == UIA_InvokePatternId )
		result =
			QueryInterface( __uuidof( IInvokeProvider ), reinterpret_cast<void**>( provider ) );
	else if ( patternId == UIA_TogglePatternId )
		result =
			QueryInterface( __uuidof( IToggleProvider ), reinterpret_cast<void**>( provider ) );
	else if ( patternId == UIA_SelectionPatternId )
		result =
			QueryInterface( __uuidof( ISelectionProvider ), reinterpret_cast<void**>( provider ) );
	else if ( patternId == UIA_SelectionItemPatternId )
		result = QueryInterface( __uuidof( ISelectionItemProvider ),
								 reinterpret_cast<void**>( provider ) );
	else if ( patternId == UIA_ValuePatternId )
		result = QueryInterface( __uuidof( IValueProvider ), reinterpret_cast<void**>( provider ) );
	else if ( patternId == UIA_RangeValuePatternId )
		result =
			QueryInterface( __uuidof( IRangeValueProvider ), reinterpret_cast<void**>( provider ) );
	else if ( patternId == UIA_ExpandCollapsePatternId )
		result = QueryInterface( __uuidof( IExpandCollapseProvider ),
								 reinterpret_cast<void**>( provider ) );
	else if ( patternId == UIA_ScrollItemPatternId )
		result =
			QueryInterface( __uuidof( IScrollItemProvider ), reinterpret_cast<void**>( provider ) );
	else if ( patternId == UIA_TextPatternId )
		result = QueryInterface( __uuidof( ITextProvider ), reinterpret_cast<void**>( provider ) );
	return result == E_NOINTERFACE ? S_OK : result;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::GetPropertyValue( PROPERTYID propertyId,
																  VARIANT* value ) {
	if ( !value )
		return E_INVALIDARG;
	VariantInit( value );
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo, propertyId == UIA_ValueValuePropertyId ||
											   propertyId == UIA_RangeValueValuePropertyId );
	if ( FAILED( result ) )
		return result;
	if ( propertyId == UIA_NamePropertyId )
		return setBstr( value, nodeInfo.name );
	if ( propertyId == UIA_HelpTextPropertyId )
		return setBstr( value, nodeInfo.description );
	if ( propertyId == UIA_AcceleratorKeyPropertyId )
		return nodeInfo.shortcut.empty()
				   ? S_OK
				   : setBstr( value, String::fromUtf8( nodeInfo.shortcut.text ) );
	if ( propertyId == UIA_ControlTypePropertyId ) {
		value->vt = VT_I4;
		value->lVal = controlType( nodeInfo.role );
	} else if ( propertyId == UIA_IsEnabledPropertyId ) {
		value->vt = VT_BOOL;
		value->boolVal =
			hasState( nodeInfo.states, AccessibilityState::Enabled ) ? VARIANT_TRUE : VARIANT_FALSE;
	} else if ( propertyId == UIA_HasKeyboardFocusPropertyId ) {
		value->vt = VT_BOOL;
		value->boolVal =
			hasState( nodeInfo.states, AccessibilityState::Focused ) ? VARIANT_TRUE : VARIANT_FALSE;
	} else if ( propertyId == UIA_IsKeyboardFocusablePropertyId ) {
		value->vt = VT_BOOL;
		value->boolVal = hasState( nodeInfo.states, AccessibilityState::Focusable ) ? VARIANT_TRUE
																					: VARIANT_FALSE;
	} else if ( propertyId == UIA_AutomationIdPropertyId ) {
		return setBstr( value, String::fromUtf8( String::toString( mRef.source ) + ":" +
												 String::toString( mRef.id ) ) );
	} else if ( propertyId == UIA_FrameworkIdPropertyId ) {
		value->vt = VT_BSTR;
		value->bstrVal = SysAllocString( L"eepp" );
		if ( !value->bstrVal )
			return E_OUTOFMEMORY;
	} else if ( propertyId == UIA_ClassNamePropertyId ) {
		value->vt = VT_BSTR;
		value->bstrVal = SysAllocString( mRoot ? L"SDL_app" : L"eepp" );
		if ( !value->bstrVal )
			return E_OUTOFMEMORY;
	} else if ( propertyId == UIA_IsControlElementPropertyId ||
				propertyId == UIA_IsContentElementPropertyId ) {
		value->vt = VT_BOOL;
		value->boolVal = nodeInfo.role != AccessibilityRole::None ? VARIANT_TRUE : VARIANT_FALSE;
	} else if ( propertyId == UIA_IsOffscreenPropertyId ) {
		value->vt = VT_BOOL;
		const HWND nativeWindow = mContext->window();
		value->boolVal = hasState( nodeInfo.states, AccessibilityState::Showing ) &&
								 IsWindowVisible( nativeWindow ) && !IsIconic( nativeWindow )
							 ? VARIANT_FALSE
							 : VARIANT_TRUE;
	} else if ( propertyId == UIA_IsPasswordPropertyId ) {
		value->vt = VT_BOOL;
		value->boolVal = hasState( nodeInfo.states, AccessibilityState::Protected ) ? VARIANT_TRUE
																					: VARIANT_FALSE;
	} else if ( propertyId == UIA_ProcessIdPropertyId ) {
		value->vt = VT_I4;
		value->lVal = static_cast<LONG>( GetCurrentProcessId() );
	} else if ( propertyId == UIA_NativeWindowHandlePropertyId && mRoot ) {
		value->vt = VT_I4;
		value->lVal = static_cast<LONG>( reinterpret_cast<LONG_PTR>( mContext->window() ) );
	} else if ( propertyId == UIA_ValueValuePropertyId &&
				( hasAction( nodeInfo.actions, AccessibilityAction::SetText ) ||
				  nodeInfo.role == AccessibilityRole::ComboBox ) ) {
		return setBstr( value, nodeInfo.value );
	} else if ( propertyId == UIA_ValueIsReadOnlyPropertyId ) {
		value->vt = VT_BOOL;
		value->boolVal = hasAction( nodeInfo.actions, AccessibilityAction::SetText ) ? VARIANT_FALSE
																					 : VARIANT_TRUE;
	} else if ( propertyId == UIA_RangeValueValuePropertyId && nodeInfo.range.valid ) {
		double number{};
		if ( !String::fromString( number, nodeInfo.value.toUtf8() ) || !std::isfinite( number ) )
			return UIA_E_INVALIDOPERATION;
		value->vt = VT_R8;
		value->dblVal = number;
	} else if ( propertyId == UIA_RangeValueMinimumPropertyId && nodeInfo.range.valid ) {
		value->vt = VT_R8;
		value->dblVal = nodeInfo.range.minimum;
	} else if ( propertyId == UIA_RangeValueMaximumPropertyId && nodeInfo.range.valid ) {
		value->vt = VT_R8;
		value->dblVal = nodeInfo.range.maximum;
	} else if ( propertyId == UIA_RangeValueSmallChangePropertyId && nodeInfo.range.valid ) {
		value->vt = VT_R8;
		value->dblVal = nodeInfo.range.smallChange;
	} else if ( propertyId == UIA_RangeValueLargeChangePropertyId && nodeInfo.range.valid ) {
		value->vt = VT_R8;
		value->dblVal = nodeInfo.range.largeChange;
	} else if ( propertyId == UIA_RangeValueIsReadOnlyPropertyId && nodeInfo.range.valid ) {
		value->vt = VT_BOOL;
		value->boolVal = hasAction( nodeInfo.actions, AccessibilityAction::SetValue )
							 ? VARIANT_FALSE
							 : VARIANT_TRUE;
	} else if ( propertyId == UIA_ToggleToggleStatePropertyId &&
				hasAction( nodeInfo.actions, AccessibilityAction::Toggle ) ) {
		value->vt = VT_I4;
		value->lVal = hasState( nodeInfo.states, AccessibilityState::Checked ) ? ToggleState_On
																			   : ToggleState_Off;
	} else if ( propertyId == UIA_SelectionItemIsSelectedPropertyId &&
				( hasAction( nodeInfo.actions, AccessibilityAction::Select ) ||
				  hasState( nodeInfo.states, AccessibilityState::Selected ) ) ) {
		value->vt = VT_BOOL;
		value->boolVal = hasState( nodeInfo.states, AccessibilityState::Selected ) ? VARIANT_TRUE
																				   : VARIANT_FALSE;
	} else if ( propertyId == UIA_SelectionCanSelectMultiplePropertyId &&
				isSelectionContainerRole( nodeInfo.role ) ) {
		value->vt = VT_BOOL;
		value->boolVal = VARIANT_FALSE;
	} else if ( propertyId == UIA_SelectionIsSelectionRequiredPropertyId &&
				isSelectionContainerRole( nodeInfo.role ) ) {
		value->vt = VT_BOOL;
		value->boolVal = VARIANT_FALSE;
	} else if ( propertyId == UIA_ExpandCollapseExpandCollapseStatePropertyId &&
				( hasAction( nodeInfo.actions, AccessibilityAction::Expand ) ||
				  hasAction( nodeInfo.actions, AccessibilityAction::Collapse ) ) ) {
		value->vt = VT_I4;
		value->lVal = hasState( nodeInfo.states, AccessibilityState::Expanded )
						  ? ExpandCollapseState_Expanded
						  : ExpandCollapseState_Collapsed;
	} else {
		return setNotSupported( value );
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
UIAutomationProvider::get_HostRawElementProvider( IRawElementProviderSimple** provider ) {
	if ( !provider )
		return E_INVALIDARG;
	*provider = nullptr;
	if ( unavailable() )
		return UIA_E_ELEMENTNOTAVAILABLE;
	return mRoot ? UiaHostProviderFromHwnd( mContext->window(), provider ) : S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::Navigate( NavigateDirection direction,
														  IRawElementProviderFragment** provider ) {
	if ( !provider )
		return E_INVALIDARG;
	*provider = nullptr;
	NavigationResult navigation;
	const HRESULT result = invoke(
		navigation,
		[this, direction]( AccessibilityManager& manager, NavigationResult& output ) -> HRESULT {
			if ( !manager.isValid( mRef ) )
				return UIA_E_ELEMENTNOTAVAILABLE;
			if ( direction == NavigateDirection_Parent ) {
				output.target = manager.getParent( mRef );
			} else if ( direction == NavigateDirection_FirstChild ) {
				output.target = manager.getChild( mRef, 0 );
			} else if ( direction == NavigateDirection_LastChild ) {
				const size_t count = manager.getChildCount( mRef );
				if ( count > 0 )
					output.target = manager.getChild( mRef, count - 1 );
			} else {
				const AccessibilityNodeRef parent = manager.getParent( mRef );
				if ( !parent.isValid() )
					return S_OK;
				const size_t count = manager.getChildCount( parent );
				const Int32 index = manager.getIndexInParent( mRef );
				if ( index >= 0 && direction == NavigateDirection_NextSibling &&
					 static_cast<size_t>( index + 1 ) < count )
					output.target = manager.getChild( parent, index + 1 );
				else if ( direction == NavigateDirection_PreviousSibling && index > 0 )
					output.target = manager.getChild( parent, index - 1 );
			}
			return S_OK;
		} );
	if ( FAILED( result ) )
		return result;
	if ( navigation.target.isValid() )
		*provider =
			static_cast<IRawElementProviderFragment*>( mContext->provider( navigation.target ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::GetRuntimeId( SAFEARRAY** runtimeId ) {
	if ( !runtimeId )
		return E_INVALIDARG;
	*runtimeId = nullptr;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo, false );
	if ( FAILED( result ) )
		return result;
	*runtimeId = safeArrayFromInts( runtimeIdValues( mContext->runtimeScope(), mRef ) );
	return *runtimeId ? S_OK : E_OUTOFMEMORY;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::get_BoundingRectangle( UiaRect* rectangle ) {
	if ( !rectangle )
		return E_INVALIDARG;
	*rectangle = {};
	GeometryResult geometry;
	const HRESULT result = invoke(
		geometry, [this]( AccessibilityManager& manager, GeometryResult& output ) -> HRESULT {
			if ( !manager.isValid( mRef ) )
				return UIA_E_ELEMENTNOTAVAILABLE;
			const auto nodeInfo = manager.getNodeInfo( mRef, false, false );
			output.bounds = nodeInfo.bounds;
			output.boundsValid = nodeInfo.boundsValid;
			const HWND nativeWindow = mContext->window();
			output.windowVisible =
				nativeWindow && IsWindowVisible( nativeWindow ) && !IsIconic( nativeWindow );
			if ( nativeWindow && !ClientToScreen( nativeWindow, &output.clientOrigin ) )
				return HRESULT_FROM_WIN32( GetLastError() );
			return S_OK;
		} );
	if ( FAILED( result ) )
		return result;
	if ( !geometry.boundsValid || !geometry.windowVisible )
		return S_OK;
	rectangle->left = geometry.clientOrigin.x + geometry.bounds.Left;
	rectangle->top = geometry.clientOrigin.y + geometry.bounds.Top;
	rectangle->width = geometry.bounds.getWidth();
	rectangle->height = geometry.bounds.getHeight();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::GetEmbeddedFragmentRoots( SAFEARRAY** roots ) {
	if ( !roots )
		return E_INVALIDARG;
	*roots = nullptr;
	return unavailable() ? UIA_E_ELEMENTNOTAVAILABLE : S_OK;
}

HRESULT STDMETHODCALLTYPE
UIAutomationProvider::get_FragmentRoot( IRawElementProviderFragmentRoot** root ) {
	if ( !root )
		return E_INVALIDARG;
	*root = nullptr;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo, false );
	if ( FAILED( result ) )
		return result;
	UIAutomationProvider* rootProvider = mContext->provider( mContext->rootRef() );
	if ( !rootProvider )
		return UIA_E_ELEMENTNOTAVAILABLE;
	*root = static_cast<IRawElementProviderFragmentRoot*>( rootProvider );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::ElementProviderFromPoint(
	double x, double y, IRawElementProviderFragment** provider ) {
	if ( !provider )
		return E_INVALIDARG;
	*provider = nullptr;
	NavigationResult hit;
	const HRESULT result = invoke(
		hit, [this, x, y]( AccessibilityManager& manager, NavigationResult& output ) -> HRESULT {
			if ( !manager.isValid( mRef ) )
				return UIA_E_ELEMENTNOTAVAILABLE;
			POINT clientOrigin{};
			const HWND nativeWindow = mContext->window();
			if ( !nativeWindow || !ClientToScreen( nativeWindow, &clientOrigin ) )
				return UIA_E_ELEMENTNOTAVAILABLE;
			output.target =
				manager.hitTest( Math::Vector2f( static_cast<Float>( x - clientOrigin.x ),
												 static_cast<Float>( y - clientOrigin.y ) ) );
			return S_OK;
		} );
	if ( FAILED( result ) )
		return result;
	if ( hit.target.isValid() )
		*provider = static_cast<IRawElementProviderFragment*>( mContext->provider( hit.target ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::GetFocus( IRawElementProviderFragment** provider ) {
	if ( !provider )
		return E_INVALIDARG;
	*provider = nullptr;
	AccessibilityNodeRef focused;
	const HRESULT result = invoke(
		focused, [this]( AccessibilityManager& manager, AccessibilityNodeRef& output ) -> HRESULT {
			if ( !manager.isValid( mRef ) )
				return UIA_E_ELEMENTNOTAVAILABLE;
			output = manager.getKeyboardFocusedNode();
			return S_OK;
		} );
	if ( FAILED( result ) )
		return result;
	if ( focused.isValid() )
		*provider = static_cast<IRawElementProviderFragment*>( mContext->provider( focused ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::AdviseEventAdded( EVENTID, SAFEARRAY* ) {
	if ( unavailable() )
		return UIA_E_ELEMENTNOTAVAILABLE;
	mContext->adviseEventAdded();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::AdviseEventRemoved( EVENTID, SAFEARRAY* ) {
	if ( unavailable() )
		return UIA_E_ELEMENTNOTAVAILABLE;
	mContext->adviseEventRemoved();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::get_ToggleState( ToggleState* state ) {
	if ( !state )
		return E_INVALIDARG;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo, false );
	if ( FAILED( result ) )
		return result;
	*state =
		hasState( nodeInfo.states, AccessibilityState::Checked ) ? ToggleState_On : ToggleState_Off;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::GetSelection( SAFEARRAY** selection ) {
	if ( !selection )
		return E_INVALIDARG;
	*selection = nullptr;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT infoResult = info( nodeInfo, false );
	if ( FAILED( infoResult ) )
		return infoResult;
	if ( nodeInfo.text.valid && !hasState( nodeInfo.states, AccessibilityState::Protected ) ) {
		TextSnapshot snapshot;
		const HRESULT textResult = textSnapshot( mContext, mRef, snapshot );
		if ( FAILED( textResult ) )
			return textResult;
		SAFEARRAY* array = SafeArrayCreateVector( VT_UNKNOWN, 0, 1 );
		if ( !array )
			return E_OUTOFMEMORY;
		ITextRangeProvider* range = new UIAutomationTextRange(
			mContext, mRef, snapshot.selectionStart, snapshot.selectionEnd );
		LONG index = 0;
		IUnknown* unknown = range;
		const HRESULT putResult = SafeArrayPutElement( array, &index, unknown );
		range->Release();
		if ( FAILED( putResult ) ) {
			SafeArrayDestroy( array );
			return putResult;
		}
		*selection = array;
		return S_OK;
	}
	std::vector<AccessibilityNodeRef> selected;
	const HRESULT result = invoke( selected,
								   [this]( AccessibilityManager& manager,
										   std::vector<AccessibilityNodeRef>& output ) -> HRESULT {
									   if ( !manager.isValid( mRef ) )
										   return UIA_E_ELEMENTNOTAVAILABLE;
									   const auto containerInfo =
										   manager.getNodeInfo( mRef, false, false );
									   if ( !isSelectionContainerRole( containerInfo.role ) )
										   return UIA_E_INVALIDOPERATION;
									   output = manager.getSelectedChildren( mRef );
									   return S_OK;
								   } );
	if ( FAILED( result ) )
		return result;
	SAFEARRAY* array =
		SafeArrayCreateVector( VT_UNKNOWN, 0, static_cast<ULONG>( selected.size() ) );
	if ( !array )
		return E_OUTOFMEMORY;
	for ( LONG index = 0; index < static_cast<LONG>( selected.size() ); ++index ) {
		UIAutomationProvider* item = mContext->provider( selected[static_cast<size_t>( index )] );
		IUnknown* unknown = static_cast<IRawElementProviderSimple*>( item );
		const HRESULT putResult = SafeArrayPutElement( array, &index, unknown );
		item->Release();
		if ( FAILED( putResult ) ) {
			SafeArrayDestroy( array );
			return putResult;
		}
	}
	*selection = array;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::get_CanSelectMultiple( BOOL* canSelectMultiple ) {
	if ( !canSelectMultiple )
		return E_INVALIDARG;
	if ( unavailable() )
		return UIA_E_ELEMENTNOTAVAILABLE;
	*canSelectMultiple = FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::get_IsSelectionRequired( BOOL* selectionRequired ) {
	if ( !selectionRequired )
		return E_INVALIDARG;
	if ( unavailable() )
		return UIA_E_ELEMENTNOTAVAILABLE;
	*selectionRequired = FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::get_IsSelected( BOOL* selected ) {
	if ( !selected )
		return E_INVALIDARG;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo, false );
	if ( FAILED( result ) )
		return result;
	*selected = hasState( nodeInfo.states, AccessibilityState::Selected ) ? TRUE : FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
UIAutomationProvider::get_SelectionContainer( IRawElementProviderSimple** provider ) {
	if ( !provider )
		return E_INVALIDARG;
	*provider = nullptr;
	AccessibilityNodeRef parent;
	const HRESULT result = invoke(
		parent, [this]( AccessibilityManager& manager, AccessibilityNodeRef& output ) -> HRESULT {
			if ( !manager.isValid( mRef ) )
				return UIA_E_ELEMENTNOTAVAILABLE;
			output = manager.getParent( mRef );
			return S_OK;
		} );
	if ( FAILED( result ) )
		return result;
	if ( parent.isValid() )
		*provider = static_cast<IRawElementProviderSimple*>( mContext->provider( parent ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::SetValue( LPCWSTR value ) {
	String text( value ? value : L"" );
	bool ignored{};
	return invoke( ignored,
				   [this, text = std::move( text )]( AccessibilityManager& manager,
													 bool& ) mutable -> HRESULT {
					   if ( !manager.isValid( mRef ) )
						   return UIA_E_ELEMENTNOTAVAILABLE;
					   const auto nodeInfo = manager.getNodeInfo( mRef, false, false );
					   if ( !hasState( nodeInfo.states, AccessibilityState::Enabled ) )
						   return UIA_E_ELEMENTNOTENABLED;
					   if ( !hasAction( nodeInfo.actions, AccessibilityAction::SetText ) )
						   return UIA_E_INVALIDOPERATION;
					   return manager.performAction(
								  mRef, { AccessibilityAction::SetText, std::move( text ) } )
								  ? S_OK
								  : E_FAIL;
				   } );
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::get_Value( BSTR* value ) {
	if ( !value )
		return E_INVALIDARG;
	*value = nullptr;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo );
	if ( FAILED( result ) )
		return result;
	const auto wide = nodeInfo.value.toWideString();
	*value = SysAllocStringLen( wide.data(), static_cast<UINT>( wide.size() ) );
	return *value ? S_OK : E_OUTOFMEMORY;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::get_IsReadOnly( BOOL* readOnly ) {
	if ( !readOnly )
		return E_INVALIDARG;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo, false );
	if ( FAILED( result ) )
		return result;
	const AccessibilityAction writableAction =
		nodeInfo.range.valid ? AccessibilityAction::SetValue : AccessibilityAction::SetText;
	*readOnly = hasAction( nodeInfo.actions, writableAction ) ? FALSE : TRUE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
UIAutomationProvider::get_ExpandCollapseState( ExpandCollapseState* state ) {
	if ( !state )
		return E_INVALIDARG;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo, false );
	if ( FAILED( result ) )
		return result;
	*state = hasState( nodeInfo.states, AccessibilityState::Expanded )
				 ? ExpandCollapseState_Expanded
				 : ExpandCollapseState_Collapsed;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::SetValue( double value ) {
	if ( !std::isfinite( value ) )
		return E_INVALIDARG;
	bool ignored{};
	return invoke( ignored, [this, value]( AccessibilityManager& manager, bool& ) -> HRESULT {
		if ( !manager.isValid( mRef ) )
			return UIA_E_ELEMENTNOTAVAILABLE;
		const auto nodeInfo = manager.getNodeInfo( mRef, false, false );
		if ( !hasState( nodeInfo.states, AccessibilityState::Enabled ) )
			return UIA_E_ELEMENTNOTENABLED;
		if ( !nodeInfo.range.valid ||
			 !hasAction( nodeInfo.actions, AccessibilityAction::SetValue ) )
			return UIA_E_INVALIDOPERATION;
		if ( value < nodeInfo.range.minimum || value > nodeInfo.range.maximum )
			return E_INVALIDARG;
		return manager.performAction(
				   mRef, { AccessibilityAction::SetValue, String( String::toString( value ) ) } )
				   ? S_OK
				   : E_FAIL;
	} );
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::get_Value( double* value ) {
	if ( !value )
		return E_INVALIDARG;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo );
	if ( FAILED( result ) )
		return result;
	if ( !String::fromString( *value, nodeInfo.value.toUtf8() ) || !std::isfinite( *value ) )
		return UIA_E_INVALIDOPERATION;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::GetVisibleRanges( SAFEARRAY** ranges ) {
	if ( !ranges )
		return E_INVALIDARG;
	*ranges = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = textSnapshot( mContext, mRef, snapshot );
	if ( FAILED( result ) )
		return result;
	SAFEARRAY* array = SafeArrayCreateVector( VT_UNKNOWN, 0, 1 );
	if ( !array )
		return E_OUTOFMEMORY;
	ITextRangeProvider* range =
		new UIAutomationTextRange( mContext, mRef, 0, static_cast<int>( snapshot.text().size() ) );
	LONG index = 0;
	IUnknown* unknown = range;
	const HRESULT putResult = SafeArrayPutElement( array, &index, unknown );
	range->Release();
	if ( FAILED( putResult ) ) {
		SafeArrayDestroy( array );
		return putResult;
	}
	*ranges = array;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::RangeFromChild( IRawElementProviderSimple* child,
																ITextRangeProvider** range ) {
	if ( !child || !range )
		return E_INVALIDARG;
	*range = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = textSnapshot( mContext, mRef, snapshot );
	return FAILED( result ) ? result : E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::RangeFromPoint( UiaPoint point,
																ITextRangeProvider** range ) {
	if ( !range )
		return E_INVALIDARG;
	*range = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = textSnapshot( mContext, mRef, snapshot );
	if ( FAILED( result ) )
		return result;
	POINT origin{};
	if ( !snapshot.boundsValid || !ClientToScreen( mContext->window(), &origin ) )
		return UIA_E_ELEMENTNOTAVAILABLE;
	const double left = origin.x + snapshot.bounds.Left;
	const double width = snapshot.bounds.getWidth();
	double ratio = width > 0 ? ( point.x - left ) / width : 0;
	ratio = std::max( 0.0, std::min( ratio, 1.0 ) );
	size_t offset = static_cast<size_t>( ratio * snapshot.text().size() );
	if ( offset < snapshot.text().size() && isLowSurrogate( snapshot.text()[offset] ) )
		--offset;
	*range = new UIAutomationTextRange( mContext, mRef, static_cast<int>( offset ),
										static_cast<int>( offset ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationProvider::get_DocumentRange( ITextRangeProvider** range ) {
	if ( !range )
		return E_INVALIDARG;
	*range = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = textSnapshot( mContext, mRef, snapshot );
	if ( FAILED( result ) )
		return result;
	*range =
		new UIAutomationTextRange( mContext, mRef, 0, static_cast<int>( snapshot.text().size() ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
UIAutomationProvider::get_SupportedTextSelection( SupportedTextSelection* selection ) {
	if ( !selection )
		return E_INVALIDARG;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo, false );
	if ( FAILED( result ) )
		return result;
	if ( !nodeInfo.text.valid || hasState( nodeInfo.states, AccessibilityState::Protected ) )
		return UIA_E_NOTSUPPORTED;
	*selection = SupportedTextSelection_Single;
	return S_OK;
}

HRESULT UIAutomationProvider::info( AccessibilityNodeInfo& nodeInfo, bool includeValue ) const {
	if ( unavailable() )
		return UIA_E_ELEMENTNOTAVAILABLE;
	return invoke( nodeInfo,
				   [this, includeValue]( AccessibilityManager& manager,
										 AccessibilityNodeInfo& output ) -> HRESULT {
					   if ( !manager.isValid( mRef ) )
						   return UIA_E_ELEMENTNOTAVAILABLE;
					   output = manager.getNodeInfo( mRef, includeValue, includeValue );
					   return S_OK;
				   } );
}

HRESULT UIAutomationProvider::perform( AccessibilityAction action ) {
	if ( unavailable() )
		return UIA_E_ELEMENTNOTAVAILABLE;
	bool ignored{};
	return invoke( ignored, [this, action]( AccessibilityManager& manager, bool& ) -> HRESULT {
		if ( !manager.isValid( mRef ) )
			return UIA_E_ELEMENTNOTAVAILABLE;
		const auto nodeInfo = manager.getNodeInfo( mRef, false, false );
		if ( !hasState( nodeInfo.states, AccessibilityState::Enabled ) )
			return UIA_E_ELEMENTNOTENABLED;
		if ( !hasAction( nodeInfo.actions, action ) )
			return UIA_E_INVALIDOPERATION;
		if ( action == AccessibilityAction::Focus ) {
			const HWND nativeWindow = mContext->window();
			if ( !nativeWindow || !IsWindow( nativeWindow ) )
				return UIA_E_ELEMENTNOTAVAILABLE;
			SetForegroundWindow( nativeWindow );
			::SetFocus( nativeWindow );
		}
		return manager.performAction( mRef, { action, {} } ) ? S_OK : E_FAIL;
	} );
}

HRESULT UIAutomationProvider::rangeValue( double* value, double AccessibilityRangeInfo::* member ) {
	if ( !value )
		return E_INVALIDARG;
	AccessibilityNodeInfo nodeInfo;
	const HRESULT result = info( nodeInfo, false );
	if ( FAILED( result ) )
		return result;
	if ( !nodeInfo.range.valid )
		return UIA_E_INVALIDOPERATION;
	*value = nodeInfo.range.*member;
	return S_OK;
}

}}} // namespace EE::UI::Uia

#endif
