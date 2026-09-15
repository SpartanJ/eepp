#include "accessibilitybackend.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/window.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cwctype>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

// clang-format off
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
// clang-format on
#include <uiautomationclient.h>
#include <uiautomationcore.h>
#include <uiautomationcoreapi.h>

namespace EE { namespace UI {

namespace {

using namespace EE::Scene;

constexpr DWORD ProviderCallTimeoutMilliseconds = 2000;

UINT accessibilityDispatchMessage() {
	static const UINT message = RegisterWindowMessageW( L"eepp.UIAutomation.Dispatch" );
	return message;
}

class UIAutomationProvider;

class UIAutomationProviderContext final
	: public std::enable_shared_from_this<UIAutomationProviderContext> {
  private:
	class PendingRequest {
	  public:
		virtual ~PendingRequest() = default;

		virtual void execute( AccessibilityManager& manager ) = 0;

		void cancel( HRESULT result ) {
			std::lock_guard<std::mutex> lock( mMutex );
			if ( mComplete )
				return;
			mResult = result;
			mComplete = true;
			mCondition.notify_all();
		}

		HRESULT wait() {
			std::unique_lock<std::mutex> lock( mMutex );
			if ( !mCondition.wait_for( lock,
									   std::chrono::milliseconds( ProviderCallTimeoutMilliseconds ),
									   [&] { return mComplete; } ) ) {
				mResult = UIA_E_TIMEOUT;
				mComplete = true;
			}
			return mResult;
		}

	  protected:
		bool begin() {
			std::lock_guard<std::mutex> lock( mMutex );
			if ( mComplete )
				return false;
			return true;
		}

		void finish( HRESULT result ) {
			std::lock_guard<std::mutex> lock( mMutex );
			if ( mComplete )
				return;
			mResult = result;
			mComplete = true;
			mCondition.notify_all();
		}

		std::mutex mMutex;
		std::condition_variable mCondition;
		HRESULT mResult{ E_FAIL };
		bool mComplete{ false };
	};

	template <typename T> class TypedPendingRequest final : public PendingRequest {
	  public:
		using Callback = std::function<HRESULT( AccessibilityManager&, T& )>;

		explicit TypedPendingRequest( Callback callback ) : mCallback( std::move( callback ) ) {}

		void execute( AccessibilityManager& manager ) override {
			if ( !begin() )
				return;
			T value{};
			const HRESULT result = mCallback( manager, value );
			{
				std::lock_guard<std::mutex> lock( mMutex );
				if ( !mComplete && SUCCEEDED( result ) )
					mValue = std::move( value );
			}
			finish( result );
		}

		HRESULT wait( T& value ) {
			const HRESULT result = PendingRequest::wait();
			if ( SUCCEEDED( result ) ) {
				std::lock_guard<std::mutex> lock( mMutex );
				value = std::move( mValue );
			}
			return result;
		}

	  private:
		Callback mCallback;
		T mValue{};
	};

  public:
	UIAutomationProviderContext( AccessibilityManager& manager, HWND window, LONG runtimeScope ) :
		mManager( &manager ),
		mWindow( window ),
		mUIThread( GetCurrentThreadId() ),
		mRuntimeScope( runtimeScope ),
		mRootRef( manager.getRoot() ) {}

	~UIAutomationProviderContext() { detach(); }

	UIAutomationProviderContext( const UIAutomationProviderContext& ) = delete;
	UIAutomationProviderContext& operator=( const UIAutomationProviderContext& ) = delete;

	bool isAlive() const { return mAlive.load( std::memory_order_acquire ); }

	HWND window() const { return mWindow.load( std::memory_order_acquire ); }

	LONG runtimeScope() const { return mRuntimeScope; }

	AccessibilityNodeRef rootRef() const { return mRootRef; }

	void updateClientState() {
		mClientsListening.store( UiaClientsAreListening(), std::memory_order_release );
	}

	bool hasActiveClients() const {
		return isAlive() && ( mClientsListening.load( std::memory_order_acquire ) ||
							  mAdvisedEventCount.load( std::memory_order_acquire ) != 0 ||
							  mClientObserved.load( std::memory_order_acquire ) );
	}

	void clientObserved() {
		mClientObserved.store( true, std::memory_order_release );
		if ( mManager )
			mManager->onNativeClientObserved();
	}

	void adviseEventAdded() { mAdvisedEventCount.fetch_add( 1, std::memory_order_relaxed ); }

	void adviseEventRemoved() {
		long count = mAdvisedEventCount.load( std::memory_order_relaxed );
		while ( count > 0 && !mAdvisedEventCount.compare_exchange_weak(
								 count, count - 1, std::memory_order_relaxed ) ) {
		}
	}

	template <typename T, typename Callback> HRESULT invoke( T& value, Callback&& callback ) {
		if ( !isAlive() )
			return UIA_E_ELEMENTNOTAVAILABLE;
		if ( GetCurrentThreadId() == mUIThread ) {
			if ( !mManager )
				return UIA_E_ELEMENTNOTAVAILABLE;
			return callback( *mManager, value );
		}

		using Request = TypedPendingRequest<T>;
		auto request = std::make_shared<Request>(
			typename Request::Callback{ std::forward<Callback>( callback ) } );
		{
			std::lock_guard<std::mutex> lock( mPendingMutex );
			if ( !isAlive() )
				return UIA_E_ELEMENTNOTAVAILABLE;
			mPendingRequests.emplace_back( request );
		}

		const HWND nativeWindow = window();
		if ( !nativeWindow ||
			 !PostMessageW( nativeWindow, accessibilityDispatchMessage(), 0, 0 ) ) {
			request->cancel( UIA_E_ELEMENTNOTAVAILABLE );
			removePendingRequest( request.get() );
		}

		const HRESULT result = request->wait( value );
		if ( result == static_cast<HRESULT>( UIA_E_TIMEOUT ) ) {
			request->cancel( result );
			removePendingRequest( request.get() );
		}
		return result;
	}

	UIAutomationProvider* provider( AccessibilityNodeRef ref );

	void dispatchPendingRequests() {
		std::vector<std::shared_ptr<PendingRequest>> pending;
		{
			std::lock_guard<std::mutex> lock( mPendingMutex );
			pending.swap( mPendingRequests );
		}
		AccessibilityManager* manager = mManager;
		for ( const auto& request : pending ) {
			if ( manager && isAlive() )
				request->execute( *manager );
			else
				request->cancel( UIA_E_ELEMENTNOTAVAILABLE );
		}
	}

	void invalidateProvider( AccessibilityNodeRef ref );

	void invalidateSource( AccessibilitySourceId sourceId );

	void detach();

  private:
	void removePendingRequest( PendingRequest* request ) {
		std::lock_guard<std::mutex> lock( mPendingMutex );
		for ( auto iterator = mPendingRequests.begin(); iterator != mPendingRequests.end();
			  ++iterator ) {
			if ( iterator->get() == request ) {
				mPendingRequests.erase( iterator );
				break;
			}
		}
	}

	AccessibilityManager* mManager{};
	std::atomic<HWND> mWindow{};
	DWORD mUIThread{};
	LONG mRuntimeScope{};
	AccessibilityNodeRef mRootRef;
	std::atomic<bool> mAlive{ true };
	std::atomic<bool> mClientsListening{ false };
	std::atomic<bool> mClientObserved{ false };
	std::atomic<long> mAdvisedEventCount{ 0 };
	std::mutex mProviderMutex;
	std::unordered_map<AccessibilitySourceId, std::unordered_map<Uint64, UIAutomationProvider*>>
		mProviders;
	std::mutex mPendingMutex;
	std::vector<std::shared_ptr<PendingRequest>> mPendingRequests;
};

bool hasState( AccessibilityState states, AccessibilityState state ) {
	return static_cast<Uint64>( states ) & static_cast<Uint64>( state );
}

bool hasAction( AccessibilityActions actions, AccessibilityAction action ) {
	return actions & accessibilityActionMask( action );
}

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

struct NavigationResult {
	AccessibilityNodeRef target;
};

struct GeometryResult {
	Math::Rectf bounds;
	POINT clientOrigin{};
	bool boundsValid{ false };
	bool windowVisible{ false };
};

struct TextSnapshot {
	String source;
	std::u16string text;
	AccessibilityActions actions{};
	int selectionStart{};
	int selectionEnd{};
	Math::Rectf bounds;
	bool boundsValid{ false };
};

size_t nextSourceCodePoint( const std::u16string& text, size_t index ) {
	if ( index >= text.size() )
		return text.size();
	const char16_t value = text[index++];
	if ( value >= 0xd800 && value <= 0xdbff && index < text.size() && text[index] >= 0xdc00 &&
		 text[index] <= 0xdfff )
		++index;
	return index;
}

size_t codePointToUTF16( const String& string, Int32 offset ) {
	const auto source = string.toUtf16();
	const size_t wanted = offset > 0 ? static_cast<size_t>( offset ) : 0;
	size_t sourceIndex = 0;
	size_t displayIndex = 0;
	for ( size_t codePoint = 0; sourceIndex < source.size() && codePoint < wanted; ++codePoint ) {
		const size_t next = nextSourceCodePoint( source, sourceIndex );
		if ( source[sourceIndex] == u'\n' &&
			 ( sourceIndex == 0 || source[sourceIndex - 1] != u'\r' ) )
			++displayIndex;
		displayIndex += next - sourceIndex;
		sourceIndex = next;
	}
	return displayIndex;
}

Int32 utf16ToCodePoint( const String& string, size_t offset ) {
	const auto source = string.toUtf16();
	size_t sourceIndex = 0;
	size_t displayIndex = 0;
	Int32 codePoints = 0;
	while ( sourceIndex < source.size() && displayIndex < offset ) {
		const size_t next = nextSourceCodePoint( source, sourceIndex );
		size_t displayLength = next - sourceIndex;
		if ( source[sourceIndex] == u'\n' &&
			 ( sourceIndex == 0 || source[sourceIndex - 1] != u'\r' ) )
			++displayLength;
		if ( displayIndex + displayLength > offset )
			break;
		displayIndex += displayLength;
		sourceIndex = next;
		++codePoints;
	}
	return codePoints;
}

std::u16string normalizedUTF16( const String& string ) {
	const auto source = string.toUtf16();
	std::u16string result;
	result.reserve( source.size() );
	for ( size_t index = 0; index < source.size(); ++index ) {
		if ( source[index] == u'\n' && ( index == 0 || source[index - 1] != u'\r' ) )
			result.push_back( u'\r' );
		result.push_back( source[index] );
	}
	return result;
}

HRESULT textSnapshot( const std::shared_ptr<UIAutomationProviderContext>& context,
					  AccessibilityNodeRef ref, TextSnapshot& snapshot ) {
	if ( !context || !context->isAlive() )
		return UIA_E_ELEMENTNOTAVAILABLE;
	return context->invoke(
		snapshot, [=]( AccessibilityManager& manager, TextSnapshot& output ) -> HRESULT {
			if ( !manager.isValid( ref ) )
				return UIA_E_ELEMENTNOTAVAILABLE;
			const auto info = manager.getNodeInfo( ref );
			if ( !info.text.valid || hasState( info.states, AccessibilityState::Protected ) )
				return UIA_E_INVALIDOPERATION;
			output.source = info.value;
			output.text = normalizedUTF16( info.value );
			output.actions = info.actions;
			output.selectionStart = static_cast<int>( codePointToUTF16(
				info.value, std::min( info.text.selectionStart, info.text.selectionEnd ) ) );
			output.selectionEnd = static_cast<int>( codePointToUTF16(
				info.value, std::max( info.text.selectionStart, info.text.selectionEnd ) ) );
			output.bounds = info.bounds;
			output.boundsValid = info.boundsValid;
			return S_OK;
		} );
}

bool isLowSurrogate( char16_t value ) {
	return value >= 0xdc00 && value <= 0xdfff;
}

bool isHighSurrogate( char16_t value ) {
	return value >= 0xd800 && value <= 0xdbff;
}

size_t nextCharacterBoundary( const std::u16string& text, size_t offset ) {
	if ( offset >= text.size() )
		return text.size();
	size_t next = offset + 1;
	if ( text[offset] == u'\r' && next < text.size() && text[next] == u'\n' )
		++next;
	else if ( isHighSurrogate( text[offset] ) && next < text.size() &&
			  isLowSurrogate( text[next] ) )
		++next;
	while ( next < text.size() ) {
		WORD type{};
		const wchar_t value = static_cast<wchar_t>( text[next] );
		if ( !GetStringTypeW( CT_CTYPE3, &value, 1, &type ) ||
			 !( type & ( C3_NONSPACING | C3_DIACRITIC | C3_VOWELMARK ) ) )
			break;
		++next;
	}
	return next;
}

size_t previousCharacterBoundary( const std::u16string& text, size_t offset ) {
	if ( offset == 0 )
		return 0;
	size_t previous = std::min( offset, text.size() ) - 1;
	if ( isLowSurrogate( text[previous] ) && previous > 0 && isHighSurrogate( text[previous - 1] ) )
		--previous;
	if ( text[previous] == u'\n' && previous > 0 && text[previous - 1] == u'\r' )
		--previous;
	while ( previous > 0 ) {
		WORD type{};
		const wchar_t value = static_cast<wchar_t>( text[previous] );
		if ( !GetStringTypeW( CT_CTYPE3, &value, 1, &type ) ||
			 !( type & ( C3_NONSPACING | C3_DIACRITIC | C3_VOWELMARK ) ) )
			break;
		previous = previousCharacterBoundary( text, previous );
	}
	return previous;
}

bool isTextSpace( char16_t value ) {
	return value == u'\r' || value == u'\n' || value == u'\t' || value == u' ';
}

std::pair<size_t, size_t> enclosingTextUnit( const std::u16string& text, size_t offset,
											 TextUnit unit ) {
	offset = std::min( offset, text.size() );
	if ( unit == TextUnit_Document || unit == TextUnit_Page || unit == TextUnit_Format )
		return { 0, text.size() };
	if ( unit == TextUnit_Character ) {
		if ( text.empty() )
			return { 0, 0 };
		if ( offset == text.size() )
			offset = previousCharacterBoundary( text, offset );
		return { offset, nextCharacterBoundary( text, offset ) };
	}
	if ( unit == TextUnit_Line || unit == TextUnit_Paragraph ) {
		size_t start = offset;
		while ( start > 0 && text[start - 1] != u'\n' )
			--start;
		size_t end = offset;
		while ( end < text.size() && text[end] != u'\n' )
			++end;
		if ( end < text.size() )
			++end;
		return { start, end };
	}
	if ( text.empty() )
		return { 0, 0 };
	if ( offset == text.size() )
		--offset;
	const bool space = isTextSpace( text[offset] );
	size_t start = offset;
	while ( start > 0 && isTextSpace( text[start - 1] ) == space )
		--start;
	size_t end = offset;
	while ( end < text.size() && isTextSpace( text[end] ) == space )
		++end;
	return { start, end };
}

size_t moveTextEndpoint( const std::u16string& text, size_t offset, TextUnit unit, int count,
						 int& moved ) {
	moved = 0;
	offset = std::min( offset, text.size() );
	if ( count == 0 )
		return offset;
	if ( unit == TextUnit_Document || unit == TextUnit_Page || unit == TextUnit_Format ) {
		const size_t target = count > 0 ? text.size() : 0;
		if ( target != offset )
			moved = count > 0 ? 1 : -1;
		return target;
	}
	const int direction = count > 0 ? 1 : -1;
	for ( int step = 0; step < std::abs( count ); ++step ) {
		size_t target = offset;
		if ( unit == TextUnit_Character )
			target = direction > 0 ? nextCharacterBoundary( text, offset )
								   : previousCharacterBoundary( text, offset );
		else if ( direction > 0 ) {
			const auto bounds = enclosingTextUnit( text, offset, unit );
			target = bounds.second;
		} else if ( offset > 0 ) {
			const auto bounds = enclosingTextUnit( text, offset - 1, unit );
			target = bounds.first;
		}
		if ( target == offset )
			break;
		offset = target;
		moved += direction;
	}
	return offset;
}

class UIAutomationTextRange final : public ITextRangeProvider {
  public:
	UIAutomationTextRange( std::shared_ptr<UIAutomationProviderContext> context,
						   AccessibilityNodeRef ref, int start, int end ) :
		mContext( std::move( context ) ), mRef( ref ), mStart( start ), mEnd( end ) {}

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID interfaceId, void** object ) override {
		if ( !object )
			return E_INVALIDARG;
		*object = nullptr;
		if ( interfaceId != __uuidof( IUnknown ) && interfaceId != __uuidof( ITextRangeProvider ) )
			return E_NOINTERFACE;
		*object = static_cast<ITextRangeProvider*>( this );
		AddRef();
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef() override {
		return mReferences.fetch_add( 1, std::memory_order_relaxed ) + 1;
	}

	ULONG STDMETHODCALLTYPE Release() override {
		const ULONG references = mReferences.fetch_sub( 1, std::memory_order_acq_rel ) - 1;
		if ( references == 0 )
			delete this;
		return references;
	}

	HRESULT STDMETHODCALLTYPE Clone( ITextRangeProvider** range ) override {
		if ( !range )
			return E_INVALIDARG;
		*range = nullptr;
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		if ( FAILED( result ) )
			return result;
		auto endpoints = normalizedEndpoints( snapshot.text.size() );
		*range = new UIAutomationTextRange( mContext, mRef, endpoints.first, endpoints.second );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Compare( ITextRangeProvider* range, BOOL* same ) override {
		if ( !range || !same )
			return E_INVALIDARG;
		*same = FALSE;
		auto other = dynamic_cast<UIAutomationTextRange*>( range );
		if ( !other || other->mRef != mRef )
			return S_OK;
		const auto ours = endpoints();
		const auto theirs = other->endpoints();
		*same = ours == theirs ? TRUE : FALSE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CompareEndpoints( TextPatternRangeEndpoint endpoint,
												ITextRangeProvider* targetRange,
												TextPatternRangeEndpoint targetEndpoint,
												int* comparison ) override {
		if ( !targetRange || !comparison )
			return E_INVALIDARG;
		auto other = dynamic_cast<UIAutomationTextRange*>( targetRange );
		if ( !other || other->mRef != mRef )
			return E_INVALIDARG;
		const auto ours = endpoints();
		const auto theirs = other->endpoints();
		const int left = endpoint == TextPatternRangeEndpoint_Start ? ours.first : ours.second;
		const int right =
			targetEndpoint == TextPatternRangeEndpoint_Start ? theirs.first : theirs.second;
		*comparison = left < right ? -1 : left > right ? 1 : 0;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE ExpandToEnclosingUnit( TextUnit unit ) override {
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		if ( FAILED( result ) )
			return result;
		const auto current = normalizedEndpoints( snapshot.text.size() );
		const auto expanded = enclosingTextUnit( snapshot.text, current.first, unit );
		setEndpoints( static_cast<int>( expanded.first ), static_cast<int>( expanded.second ) );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE FindAttribute( TEXTATTRIBUTEID, VARIANT, BOOL,
											 ITextRangeProvider** range ) override {
		if ( !range )
			return E_INVALIDARG;
		*range = nullptr;
		TextSnapshot snapshot;
		return currentSnapshot( snapshot );
	}

	HRESULT STDMETHODCALLTYPE FindText( BSTR text, BOOL backward, BOOL ignoreCase,
										ITextRangeProvider** range ) override {
		if ( !text || !range )
			return E_INVALIDARG;
		*range = nullptr;
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		if ( FAILED( result ) )
			return result;
		const auto current = normalizedEndpoints( snapshot.text.size() );
		const size_t rangeStart = static_cast<size_t>( current.first );
		const size_t rangeEnd = static_cast<size_t>( current.second );
		std::u16string needle( reinterpret_cast<const char16_t*>( text ), SysStringLen( text ) );
		if ( needle.empty() )
			return S_OK;
		auto equal = [=]( char16_t left, char16_t right ) {
			return ignoreCase ? std::towlower( static_cast<wchar_t>( left ) ) ==
									std::towlower( static_cast<wchar_t>( right ) )
							  : left == right;
		};
		size_t found = std::u16string::npos;
		if ( backward ) {
			for ( size_t candidate = rangeEnd; candidate-- > rangeStart; ) {
				if ( candidate + needle.size() <= rangeEnd &&
					 std::equal( needle.begin(), needle.end(), snapshot.text.begin() + candidate,
								 equal ) ) {
					found = candidate;
					break;
				}
			}
		} else {
			for ( size_t candidate = rangeStart; candidate + needle.size() <= rangeEnd;
				  ++candidate ) {
				if ( std::equal( needle.begin(), needle.end(), snapshot.text.begin() + candidate,
								 equal ) ) {
					found = candidate;
					break;
				}
			}
		}
		if ( found != std::u16string::npos )
			*range = new UIAutomationTextRange( mContext, mRef, static_cast<int>( found ),
												static_cast<int>( found + needle.size() ) );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetAttributeValue( TEXTATTRIBUTEID, VARIANT* value ) override {
		if ( !value )
			return E_INVALIDARG;
		VariantInit( value );
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		return FAILED( result ) ? result : setNotSupported( value );
	}

	HRESULT STDMETHODCALLTYPE GetBoundingRectangles( SAFEARRAY** rectangles ) override {
		if ( !rectangles )
			return E_INVALIDARG;
		*rectangles = nullptr;
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		if ( FAILED( result ) )
			return result;
		const auto current = normalizedEndpoints( snapshot.text.size() );
		const bool hasRectangle = current.first != current.second && snapshot.boundsValid &&
								  IsWindowVisible( mContext->window() ) &&
								  !IsIconic( mContext->window() );
		POINT origin{};
		if ( hasRectangle && !ClientToScreen( mContext->window(), &origin ) )
			return UIA_E_ELEMENTNOTAVAILABLE;
		*rectangles = SafeArrayCreateVector( VT_R8, 0, hasRectangle ? 4 : 0 );
		if ( !*rectangles )
			return E_OUTOFMEMORY;
		if ( !hasRectangle )
			return S_OK;
		double values[] = { origin.x + snapshot.bounds.Left, origin.y + snapshot.bounds.Top,
							snapshot.bounds.getWidth(), snapshot.bounds.getHeight() };
		for ( LONG index = 0; index < 4; ++index ) {
			const HRESULT result = SafeArrayPutElement( *rectangles, &index, &values[index] );
			if ( FAILED( result ) ) {
				SafeArrayDestroy( *rectangles );
				*rectangles = nullptr;
				return result;
			}
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetEnclosingElement( IRawElementProviderSimple** element ) override;

	HRESULT STDMETHODCALLTYPE GetText( int maxLength, BSTR* text ) override {
		if ( !text || maxLength < -1 )
			return E_INVALIDARG;
		*text = nullptr;
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		if ( FAILED( result ) )
			return result;
		const auto current = normalizedEndpoints( snapshot.text.size() );
		size_t length = current.second - current.first;
		if ( maxLength >= 0 )
			length = std::min( length, static_cast<size_t>( maxLength ) );
		*text = SysAllocStringLen(
			reinterpret_cast<const wchar_t*>( snapshot.text.data() + current.first ),
			static_cast<UINT>( length ) );
		return *text ? S_OK : E_OUTOFMEMORY;
	}

	HRESULT STDMETHODCALLTYPE Move( TextUnit unit, int count, int* moved ) override {
		if ( !moved )
			return E_INVALIDARG;
		*moved = 0;
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		if ( FAILED( result ) )
			return result;
		const auto current = normalizedEndpoints( snapshot.text.size() );
		int actual{};
		const size_t start = moveTextEndpoint( snapshot.text, current.first, unit, count, actual );
		if ( current.first == current.second )
			setEndpoints( static_cast<int>( start ), static_cast<int>( start ) );
		else {
			const auto expanded = enclosingTextUnit( snapshot.text, start, unit );
			setEndpoints( static_cast<int>( expanded.first ), static_cast<int>( expanded.second ) );
		}
		*moved = actual;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE MoveEndpointByUnit( TextPatternRangeEndpoint endpoint, TextUnit unit,
												  int count, int* moved ) override {
		if ( !moved )
			return E_INVALIDARG;
		*moved = 0;
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		if ( FAILED( result ) )
			return result;
		auto current = normalizedEndpoints( snapshot.text.size() );
		int actual{};
		const size_t target = moveTextEndpoint(
			snapshot.text,
			endpoint == TextPatternRangeEndpoint_Start ? current.first : current.second, unit,
			count, actual );
		if ( endpoint == TextPatternRangeEndpoint_Start ) {
			current.first = static_cast<int>( target );
			if ( current.first > current.second )
				current.second = current.first;
		} else {
			current.second = static_cast<int>( target );
			if ( current.second < current.first )
				current.first = current.second;
		}
		setEndpoints( current.first, current.second );
		*moved = actual;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE
	MoveEndpointByRange( TextPatternRangeEndpoint endpoint, ITextRangeProvider* targetRange,
						 TextPatternRangeEndpoint targetEndpoint ) override {
		if ( !targetRange )
			return E_INVALIDARG;
		auto other = dynamic_cast<UIAutomationTextRange*>( targetRange );
		if ( !other || other->mRef != mRef )
			return E_INVALIDARG;
		auto current = endpoints();
		const auto target = other->endpoints();
		const int position =
			targetEndpoint == TextPatternRangeEndpoint_Start ? target.first : target.second;
		if ( endpoint == TextPatternRangeEndpoint_Start ) {
			current.first = position;
			if ( current.first > current.second )
				current.second = current.first;
		} else {
			current.second = position;
			if ( current.second < current.first )
				current.first = current.second;
		}
		setEndpoints( current.first, current.second );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Select() override {
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		if ( FAILED( result ) )
			return result;
		if ( !hasAction( snapshot.actions, AccessibilityAction::SetTextSelection ) )
			return UIA_E_INVALIDOPERATION;
		const auto current = normalizedEndpoints( snapshot.text.size() );
		const Int32 start = utf16ToCodePoint( snapshot.source, current.first );
		const Int32 end = utf16ToCodePoint( snapshot.source, current.second );
		bool ignored{};
		return mContext->invoke(
			ignored, [this, start, end]( AccessibilityManager& manager, bool& ) -> HRESULT {
				if ( !manager.isValid( mRef ) )
					return UIA_E_ELEMENTNOTAVAILABLE;
				return manager.performAction( mRef, { AccessibilityAction::SetTextSelection,
													  String( String::toString( start ) + ":" +
															  String::toString( end ) ) } )
						   ? S_OK
						   : E_FAIL;
			} );
	}

	HRESULT STDMETHODCALLTYPE AddToSelection() override { return UIA_E_INVALIDOPERATION; }

	HRESULT STDMETHODCALLTYPE RemoveFromSelection() override { return UIA_E_INVALIDOPERATION; }

	HRESULT STDMETHODCALLTYPE ScrollIntoView( BOOL ) override {
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		if ( FAILED( result ) )
			return result;
		if ( !hasAction( snapshot.actions, AccessibilityAction::ScrollTo ) )
			return UIA_E_NOTSUPPORTED;
		bool ignored{};
		return mContext->invoke(
			ignored, [this]( AccessibilityManager& manager, bool& ) -> HRESULT {
				if ( !manager.isValid( mRef ) )
					return UIA_E_ELEMENTNOTAVAILABLE;
				return manager.performAction( mRef, { AccessibilityAction::ScrollTo, {} } )
						   ? S_OK
						   : E_FAIL;
			} );
	}

	HRESULT STDMETHODCALLTYPE GetChildren( SAFEARRAY** children ) override {
		if ( !children )
			return E_INVALIDARG;
		*children = nullptr;
		TextSnapshot snapshot;
		const HRESULT result = currentSnapshot( snapshot );
		if ( FAILED( result ) )
			return result;
		*children = SafeArrayCreateVector( VT_UNKNOWN, 0, 0 );
		return *children ? S_OK : E_OUTOFMEMORY;
	}

  private:
	~UIAutomationTextRange() = default;

	HRESULT currentSnapshot( TextSnapshot& snapshot ) const {
		return textSnapshot( mContext, mRef, snapshot );
	}

	std::pair<int, int> endpoints() const {
		std::lock_guard<std::mutex> lock( mMutex );
		return { mStart, mEnd };
	}

	std::pair<int, int> normalizedEndpoints( size_t length ) const {
		auto result = endpoints();
		result.first = std::max( 0, std::min( result.first, static_cast<int>( length ) ) );
		result.second =
			std::max( result.first, std::min( result.second, static_cast<int>( length ) ) );
		return result;
	}

	void setEndpoints( int start, int end ) {
		std::lock_guard<std::mutex> lock( mMutex );
		mStart = start;
		mEnd = end;
	}

	std::atomic<ULONG> mReferences{ 1 };
	std::shared_ptr<UIAutomationProviderContext> mContext;
	AccessibilityNodeRef mRef;
	mutable std::mutex mMutex;
	int mStart{};
	int mEnd{};
};

class UIAutomationProvider final : public IRawElementProviderSimple,
								   public IRawElementProviderFragment,
								   public IRawElementProviderFragmentRoot,
								   public IRawElementProviderAdviseEvents,
								   public IInvokeProvider,
								   public IToggleProvider,
								   public ISelectionProvider,
								   public ISelectionItemProvider,
								   public IValueProvider,
								   public IRangeValueProvider,
								   public IExpandCollapseProvider,
								   public IScrollItemProvider,
								   public ITextProvider {
  public:
	UIAutomationProvider( std::shared_ptr<UIAutomationProviderContext> context,
						  AccessibilityNodeRef ref, bool root ) :
		mContext( std::move( context ) ), mRef( ref ), mRoot( root ) {}

	void detach() { mDetached.store( true, std::memory_order_release ); }

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID interfaceId, void** object ) override {
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
			AccessibilityNodeInfo nodeInfo;
			if ( FAILED( info( nodeInfo ) ) )
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

	ULONG STDMETHODCALLTYPE AddRef() override {
		return mReferences.fetch_add( 1, std::memory_order_relaxed ) + 1;
	}

	ULONG STDMETHODCALLTYPE Release() override {
		const ULONG references = mReferences.fetch_sub( 1, std::memory_order_acq_rel ) - 1;
		if ( references == 0 )
			delete this;
		return references;
	}

	HRESULT STDMETHODCALLTYPE get_ProviderOptions( ProviderOptions* options ) override {
		if ( !options )
			return E_INVALIDARG;
		if ( unavailable() )
			return UIA_E_ELEMENTNOTAVAILABLE;
		*options = static_cast<ProviderOptions>( ProviderOptions_ServerSideProvider |
												 ProviderOptions_ProviderOwnsSetFocus |
												 ProviderOptions_UseComThreading );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetPatternProvider( PATTERNID patternId,
												  IUnknown** provider ) override {
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
			result = QueryInterface( __uuidof( ISelectionProvider ),
									 reinterpret_cast<void**>( provider ) );
		else if ( patternId == UIA_SelectionItemPatternId )
			result = QueryInterface( __uuidof( ISelectionItemProvider ),
									 reinterpret_cast<void**>( provider ) );
		else if ( patternId == UIA_ValuePatternId )
			result =
				QueryInterface( __uuidof( IValueProvider ), reinterpret_cast<void**>( provider ) );
		else if ( patternId == UIA_RangeValuePatternId )
			result = QueryInterface( __uuidof( IRangeValueProvider ),
									 reinterpret_cast<void**>( provider ) );
		else if ( patternId == UIA_ExpandCollapsePatternId )
			result = QueryInterface( __uuidof( IExpandCollapseProvider ),
									 reinterpret_cast<void**>( provider ) );
		else if ( patternId == UIA_ScrollItemPatternId )
			result = QueryInterface( __uuidof( IScrollItemProvider ),
									 reinterpret_cast<void**>( provider ) );
		else if ( patternId == UIA_TextPatternId )
			result =
				QueryInterface( __uuidof( ITextProvider ), reinterpret_cast<void**>( provider ) );
		return result == E_NOINTERFACE ? S_OK : result;
	}

	HRESULT STDMETHODCALLTYPE GetPropertyValue( PROPERTYID propertyId, VARIANT* value ) override {
		if ( !value )
			return E_INVALIDARG;
		VariantInit( value );
		AccessibilityNodeInfo nodeInfo;
		const HRESULT result = info( nodeInfo );
		if ( FAILED( result ) )
			return result;
		if ( propertyId == UIA_NamePropertyId )
			return setBstr( value, nodeInfo.name );
		if ( propertyId == UIA_HelpTextPropertyId )
			return setBstr( value, nodeInfo.description );
		if ( propertyId == UIA_ControlTypePropertyId ) {
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
			value->boolVal =
				nodeInfo.role != AccessibilityRole::None ? VARIANT_TRUE : VARIANT_FALSE;
		} else if ( propertyId == UIA_IsOffscreenPropertyId ) {
			value->vt = VT_BOOL;
			const HWND nativeWindow = mContext->window();
			value->boolVal = hasState( nodeInfo.states, AccessibilityState::Showing ) &&
									 IsWindowVisible( nativeWindow ) && !IsIconic( nativeWindow )
								 ? VARIANT_FALSE
								 : VARIANT_TRUE;
		} else if ( propertyId == UIA_IsPasswordPropertyId ) {
			value->vt = VT_BOOL;
			value->boolVal = hasState( nodeInfo.states, AccessibilityState::Protected )
								 ? VARIANT_TRUE
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
			value->boolVal = hasAction( nodeInfo.actions, AccessibilityAction::SetText )
								 ? VARIANT_FALSE
								 : VARIANT_TRUE;
		} else if ( propertyId == UIA_RangeValueValuePropertyId && nodeInfo.range.valid ) {
			double number{};
			if ( !String::fromString( number, nodeInfo.value.toUtf8() ) ||
				 !std::isfinite( number ) )
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
			value->lVal = hasState( nodeInfo.states, AccessibilityState::Checked )
							  ? ToggleState_On
							  : ToggleState_Off;
		} else if ( propertyId == UIA_SelectionItemIsSelectedPropertyId &&
					( hasAction( nodeInfo.actions, AccessibilityAction::Select ) ||
					  hasState( nodeInfo.states, AccessibilityState::Selected ) ) ) {
			value->vt = VT_BOOL;
			value->boolVal = hasState( nodeInfo.states, AccessibilityState::Selected )
								 ? VARIANT_TRUE
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
	get_HostRawElementProvider( IRawElementProviderSimple** provider ) override {
		if ( !provider )
			return E_INVALIDARG;
		*provider = nullptr;
		if ( unavailable() )
			return UIA_E_ELEMENTNOTAVAILABLE;
		return mRoot ? UiaHostProviderFromHwnd( mContext->window(), provider ) : S_OK;
	}

	HRESULT STDMETHODCALLTYPE Navigate( NavigateDirection direction,
										IRawElementProviderFragment** provider ) override {
		if ( !provider )
			return E_INVALIDARG;
		*provider = nullptr;
		NavigationResult navigation;
		const HRESULT result = mContext->invoke(
			navigation,
			[this, direction]( AccessibilityManager& manager,
							   NavigationResult& output ) -> HRESULT {
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
					for ( size_t index = 0; index < count; ++index ) {
						if ( manager.getChild( parent, index ) != mRef )
							continue;
						if ( direction == NavigateDirection_NextSibling && index + 1 < count )
							output.target = manager.getChild( parent, index + 1 );
						else if ( direction == NavigateDirection_PreviousSibling && index > 0 )
							output.target = manager.getChild( parent, index - 1 );
						break;
					}
				}
				return S_OK;
			} );
		if ( FAILED( result ) )
			return result;
		if ( navigation.target.isValid() )
			*provider = static_cast<IRawElementProviderFragment*>(
				mContext->provider( navigation.target ) );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetRuntimeId( SAFEARRAY** runtimeId ) override {
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

	HRESULT STDMETHODCALLTYPE get_BoundingRectangle( UiaRect* rectangle ) override {
		if ( !rectangle )
			return E_INVALIDARG;
		*rectangle = {};
		GeometryResult geometry;
		const HRESULT result = mContext->invoke(
			geometry, [this]( AccessibilityManager& manager, GeometryResult& output ) -> HRESULT {
				if ( !manager.isValid( mRef ) )
					return UIA_E_ELEMENTNOTAVAILABLE;
				const auto nodeInfo = manager.getNodeInfo( mRef, false );
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

	HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots( SAFEARRAY** roots ) override {
		if ( !roots )
			return E_INVALIDARG;
		*roots = nullptr;
		return unavailable() ? UIA_E_ELEMENTNOTAVAILABLE : S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetFocus() override { return perform( AccessibilityAction::Focus ); }

	HRESULT STDMETHODCALLTYPE get_FragmentRoot( IRawElementProviderFragmentRoot** root ) override {
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

	HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(
		double x, double y, IRawElementProviderFragment** provider ) override {
		if ( !provider )
			return E_INVALIDARG;
		*provider = nullptr;
		NavigationResult hit;
		const HRESULT result = mContext->invoke(
			hit,
			[this, x, y]( AccessibilityManager& manager, NavigationResult& output ) -> HRESULT {
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
			*provider =
				static_cast<IRawElementProviderFragment*>( mContext->provider( hit.target ) );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetFocus( IRawElementProviderFragment** provider ) override {
		if ( !provider )
			return E_INVALIDARG;
		*provider = nullptr;
		AccessibilityNodeRef focused;
		const HRESULT result = mContext->invoke(
			focused,
			[this]( AccessibilityManager& manager, AccessibilityNodeRef& output ) -> HRESULT {
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

	HRESULT STDMETHODCALLTYPE AdviseEventAdded( EVENTID, SAFEARRAY* ) override {
		if ( unavailable() )
			return UIA_E_ELEMENTNOTAVAILABLE;
		mContext->adviseEventAdded();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE AdviseEventRemoved( EVENTID, SAFEARRAY* ) override {
		if ( unavailable() )
			return UIA_E_ELEMENTNOTAVAILABLE;
		mContext->adviseEventRemoved();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Invoke() override { return perform( AccessibilityAction::Press ); }

	HRESULT STDMETHODCALLTYPE Toggle() override { return perform( AccessibilityAction::Toggle ); }

	HRESULT STDMETHODCALLTYPE get_ToggleState( ToggleState* state ) override {
		if ( !state )
			return E_INVALIDARG;
		AccessibilityNodeInfo nodeInfo;
		const HRESULT result = info( nodeInfo );
		if ( FAILED( result ) )
			return result;
		*state = hasState( nodeInfo.states, AccessibilityState::Checked ) ? ToggleState_On
																		  : ToggleState_Off;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetSelection( SAFEARRAY** selection ) override {
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
		const HRESULT result =
			mContext->invoke( selected,
							  [this]( AccessibilityManager& manager,
									  std::vector<AccessibilityNodeRef>& output ) -> HRESULT {
								  if ( !manager.isValid( mRef ) )
									  return UIA_E_ELEMENTNOTAVAILABLE;
								  const auto containerInfo = manager.getNodeInfo( mRef, false );
								  if ( !isSelectionContainerRole( containerInfo.role ) )
									  return UIA_E_INVALIDOPERATION;
								  const size_t childCount = manager.getChildCount( mRef );
								  for ( size_t index = 0; index < childCount; ++index ) {
									  const AccessibilityNodeRef child =
										  manager.getChild( mRef, index );
									  if ( hasState( manager.getNodeInfo( child, false ).states,
													 AccessibilityState::Selected ) )
										  output.emplace_back( child );
								  }
								  return S_OK;
							  } );
		if ( FAILED( result ) )
			return result;
		SAFEARRAY* array =
			SafeArrayCreateVector( VT_UNKNOWN, 0, static_cast<ULONG>( selected.size() ) );
		if ( !array )
			return E_OUTOFMEMORY;
		for ( LONG index = 0; index < static_cast<LONG>( selected.size() ); ++index ) {
			UIAutomationProvider* item =
				mContext->provider( selected[static_cast<size_t>( index )] );
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

	HRESULT STDMETHODCALLTYPE get_CanSelectMultiple( BOOL* canSelectMultiple ) override {
		if ( !canSelectMultiple )
			return E_INVALIDARG;
		if ( unavailable() )
			return UIA_E_ELEMENTNOTAVAILABLE;
		*canSelectMultiple = FALSE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE get_IsSelectionRequired( BOOL* selectionRequired ) override {
		if ( !selectionRequired )
			return E_INVALIDARG;
		if ( unavailable() )
			return UIA_E_ELEMENTNOTAVAILABLE;
		*selectionRequired = FALSE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Select() override { return perform( AccessibilityAction::Select ); }

	HRESULT STDMETHODCALLTYPE AddToSelection() override { return Select(); }

	HRESULT STDMETHODCALLTYPE RemoveFromSelection() override { return UIA_E_INVALIDOPERATION; }

	HRESULT STDMETHODCALLTYPE get_IsSelected( BOOL* selected ) override {
		if ( !selected )
			return E_INVALIDARG;
		AccessibilityNodeInfo nodeInfo;
		const HRESULT result = info( nodeInfo );
		if ( FAILED( result ) )
			return result;
		*selected = hasState( nodeInfo.states, AccessibilityState::Selected ) ? TRUE : FALSE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE
	get_SelectionContainer( IRawElementProviderSimple** provider ) override {
		if ( !provider )
			return E_INVALIDARG;
		*provider = nullptr;
		AccessibilityNodeRef parent;
		const HRESULT result = mContext->invoke(
			parent,
			[this]( AccessibilityManager& manager, AccessibilityNodeRef& output ) -> HRESULT {
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

	HRESULT STDMETHODCALLTYPE SetValue( LPCWSTR value ) override {
		const String text( value ? value : L"" );
		bool ignored{};
		return mContext->invoke(
			ignored, [this, text]( AccessibilityManager& manager, bool& ) -> HRESULT {
				if ( !manager.isValid( mRef ) )
					return UIA_E_ELEMENTNOTAVAILABLE;
				const auto nodeInfo = manager.getNodeInfo( mRef, false );
				if ( !hasState( nodeInfo.states, AccessibilityState::Enabled ) )
					return UIA_E_ELEMENTNOTENABLED;
				if ( !hasAction( nodeInfo.actions, AccessibilityAction::SetText ) )
					return UIA_E_INVALIDOPERATION;
				return manager.performAction( mRef, { AccessibilityAction::SetText, text } )
						   ? S_OK
						   : E_FAIL;
			} );
	}

	HRESULT STDMETHODCALLTYPE get_Value( BSTR* value ) override {
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

	HRESULT STDMETHODCALLTYPE get_IsReadOnly( BOOL* readOnly ) override {
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

	HRESULT STDMETHODCALLTYPE Expand() override { return perform( AccessibilityAction::Expand ); }

	HRESULT STDMETHODCALLTYPE Collapse() override {
		return perform( AccessibilityAction::Collapse );
	}

	HRESULT STDMETHODCALLTYPE get_ExpandCollapseState( ExpandCollapseState* state ) override {
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

	HRESULT STDMETHODCALLTYPE ScrollIntoView() override {
		return perform( AccessibilityAction::ScrollTo );
	}

	HRESULT STDMETHODCALLTYPE SetValue( double value ) override {
		if ( !std::isfinite( value ) )
			return E_INVALIDARG;
		bool ignored{};
		return mContext->invoke(
			ignored, [this, value]( AccessibilityManager& manager, bool& ) -> HRESULT {
				if ( !manager.isValid( mRef ) )
					return UIA_E_ELEMENTNOTAVAILABLE;
				const auto nodeInfo = manager.getNodeInfo( mRef, false );
				if ( !hasState( nodeInfo.states, AccessibilityState::Enabled ) )
					return UIA_E_ELEMENTNOTENABLED;
				if ( !nodeInfo.range.valid ||
					 !hasAction( nodeInfo.actions, AccessibilityAction::SetValue ) )
					return UIA_E_INVALIDOPERATION;
				if ( value < nodeInfo.range.minimum || value > nodeInfo.range.maximum )
					return E_INVALIDARG;
				return manager.performAction( mRef, { AccessibilityAction::SetValue,
													  String( String::toString( value ) ) } )
						   ? S_OK
						   : E_FAIL;
			} );
	}

	HRESULT STDMETHODCALLTYPE get_Value( double* value ) override {
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

	HRESULT STDMETHODCALLTYPE get_Maximum( double* value ) override {
		return rangeValue( value, &AccessibilityRangeInfo::maximum );
	}

	HRESULT STDMETHODCALLTYPE get_Minimum( double* value ) override {
		return rangeValue( value, &AccessibilityRangeInfo::minimum );
	}

	HRESULT STDMETHODCALLTYPE get_LargeChange( double* value ) override {
		return rangeValue( value, &AccessibilityRangeInfo::largeChange );
	}

	HRESULT STDMETHODCALLTYPE get_SmallChange( double* value ) override {
		return rangeValue( value, &AccessibilityRangeInfo::smallChange );
	}

	HRESULT STDMETHODCALLTYPE GetVisibleRanges( SAFEARRAY** ranges ) override {
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
		ITextRangeProvider* range = new UIAutomationTextRange(
			mContext, mRef, 0, static_cast<int>( snapshot.text.size() ) );
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

	HRESULT STDMETHODCALLTYPE RangeFromChild( IRawElementProviderSimple* child,
											  ITextRangeProvider** range ) override {
		if ( !child || !range )
			return E_INVALIDARG;
		*range = nullptr;
		TextSnapshot snapshot;
		const HRESULT result = textSnapshot( mContext, mRef, snapshot );
		return FAILED( result ) ? result : E_INVALIDARG;
	}

	HRESULT STDMETHODCALLTYPE RangeFromPoint( UiaPoint point,
											  ITextRangeProvider** range ) override {
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
		size_t offset = static_cast<size_t>( ratio * snapshot.text.size() );
		if ( offset < snapshot.text.size() && isLowSurrogate( snapshot.text[offset] ) )
			--offset;
		*range = new UIAutomationTextRange( mContext, mRef, static_cast<int>( offset ),
											static_cast<int>( offset ) );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE get_DocumentRange( ITextRangeProvider** range ) override {
		if ( !range )
			return E_INVALIDARG;
		*range = nullptr;
		TextSnapshot snapshot;
		const HRESULT result = textSnapshot( mContext, mRef, snapshot );
		if ( FAILED( result ) )
			return result;
		*range = new UIAutomationTextRange( mContext, mRef, 0,
											static_cast<int>( snapshot.text.size() ) );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE
	get_SupportedTextSelection( SupportedTextSelection* selection ) override {
		if ( !selection )
			return E_INVALIDARG;
		TextSnapshot snapshot;
		const HRESULT result = textSnapshot( mContext, mRef, snapshot );
		if ( FAILED( result ) )
			return result;
		*selection = SupportedTextSelection_Single;
		return S_OK;
	}

  private:
	~UIAutomationProvider() = default;

	bool unavailable() const {
		return mDetached.load( std::memory_order_acquire ) || !mContext || !mContext->isAlive();
	}

	HRESULT info( AccessibilityNodeInfo& nodeInfo, bool includeValue = true ) const {
		if ( unavailable() )
			return UIA_E_ELEMENTNOTAVAILABLE;
		return mContext->invoke( nodeInfo,
								 [this, includeValue]( AccessibilityManager& manager,
													   AccessibilityNodeInfo& output ) -> HRESULT {
									 if ( !manager.isValid( mRef ) )
										 return UIA_E_ELEMENTNOTAVAILABLE;
									 output = manager.getNodeInfo( mRef, includeValue );
									 return S_OK;
								 } );
	}

	HRESULT perform( AccessibilityAction action ) {
		if ( unavailable() )
			return UIA_E_ELEMENTNOTAVAILABLE;
		bool ignored{};
		return mContext->invoke(
			ignored, [this, action]( AccessibilityManager& manager, bool& ) -> HRESULT {
				if ( !manager.isValid( mRef ) )
					return UIA_E_ELEMENTNOTAVAILABLE;
				const auto nodeInfo = manager.getNodeInfo( mRef, false );
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

	HRESULT rangeValue( double* value, double AccessibilityRangeInfo::* member ) {
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

	std::atomic<ULONG> mReferences{ 1 };
	std::shared_ptr<UIAutomationProviderContext> mContext;
	AccessibilityNodeRef mRef;
	bool mRoot{ false };
	std::atomic<bool> mDetached{ false };
};

HRESULT STDMETHODCALLTYPE
UIAutomationTextRange::GetEnclosingElement( IRawElementProviderSimple** element ) {
	if ( !element )
		return E_INVALIDARG;
	*element = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	UIAutomationProvider* provider = mContext->provider( mRef );
	if ( !provider )
		return UIA_E_ELEMENTNOTAVAILABLE;
	*element = static_cast<IRawElementProviderSimple*>( provider );
	return S_OK;
}

UIAutomationProvider* UIAutomationProviderContext::provider( AccessibilityNodeRef ref ) {
	if ( !ref.isValid() || !isAlive() )
		return nullptr;
	std::lock_guard<std::mutex> lock( mProviderMutex );
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
	UiaDisconnectProvider( static_cast<IRawElementProviderSimple*>( providerToDetach ) );
	providerToDetach->detach();
	providerToDetach->Release();
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
		UiaDisconnectProvider( static_cast<IRawElementProviderSimple*>( provider ) );
		provider->detach();
		provider->Release();
	}
}

void UIAutomationProviderContext::detach() {
	bool expected = true;
	if ( !mAlive.compare_exchange_strong( expected, false, std::memory_order_acq_rel ) )
		return;
	mWindow.store( nullptr, std::memory_order_release );
	mManager = nullptr;

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

class UIAutomationAccessibilityBackend final : public AccessibilityBackend {
  public:
	explicit UIAutomationAccessibilityBackend( AccessibilityManager& manager );

	~UIAutomationAccessibilityBackend() override;

	bool isAvailable() const override { return mContext && mContext->isAlive() && mWindow; }

	bool hasActiveClients() const override { return isAvailable() && mContext->hasActiveClients(); }

	void update() override;

	void onEvent( const AccessibilityPendingEvent& event ) override;

	void onSourceInvalidated( AccessibilitySourceId sourceId ) override {
		if ( mContext )
			mContext->invalidateSource( sourceId );
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
	for ( const auto& event : pendingEvents )
		raiseEvent( event );
}

void UIAutomationAccessibilityBackend::onEvent( const AccessibilityPendingEvent& event ) {
	if ( !mContext )
		return;
	if ( event.type == AccessibilityEvent::Destroyed ) {
		mContext->invalidateProvider( event.related.isValid() ? event.related : event.ref );
		if ( !event.related.isValid() )
			return;
	}
	if ( !mContext->hasActiveClients() )
		return;
	mPendingEvents.emplace_back( event );
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
		const auto info = mManager.getNodeInfo( event.ref );
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
		auto info = mManager.getNodeInfo( event.ref );
		if ( info.role != AccessibilityRole::CheckBox &&
			 info.role != AccessibilityRole::CheckMenuItem &&
			 info.role != AccessibilityRole::RadioButton &&
			 info.role != AccessibilityRole::RadioMenuItem &&
			 info.role != AccessibilityRole::ComboBox &&
			 info.role != AccessibilityRole::TreeItem ) {
			const AccessibilityNodeRef parent = mManager.getParent( event.ref );
			const auto parentInfo = mManager.getNodeInfo( parent );
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
		const auto info = mManager.getNodeInfo( event.ref );
		if ( info.text.valid )
			UiaRaiseAutomationEvent( static_cast<IRawElementProviderSimple*>( nativeProvider ),
									 UIA_Text_TextSelectionChangedEventId );
		else if ( isSelectionContainerRole( info.role ) ) {
			AccessibilityNodeRef selected;
			const auto& children = mManager.getChildren( event.ref );
			for ( const auto& child : children ) {
				const auto childInfo = mManager.getNodeInfo( child, false );
				if ( hasState( childInfo.states, AccessibilityState::Selected ) ) {
					selected = child;
					break;
				}
			}
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
