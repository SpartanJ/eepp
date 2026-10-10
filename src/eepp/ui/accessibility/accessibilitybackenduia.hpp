#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITYBACKENDUIA_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITYBACKENDUIA_HPP

#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#include "accessibilitybackend.hpp"

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

/**
 * Windows UI Automation accessibility backend.
 *
 * Each top-level window owns a UIAutomationProviderContext. UIA calls arrive on arbitrary COM
 * threads; every query is marshalled to the UI thread through invoke(), which posts a window
 * message and waits (bounded by ProviderCallTimeoutMilliseconds). The implementation is split:
 *  - accessibilitybackenduia.cpp:         provider cache, window subclass, manager events -> UIA
 *                                         events, and the AccessibilityBackend entry point.
 *  - accessibilitybackenduiaprovider.cpp: the element provider (IRawElementProvider* and the
 *                                         control patterns), plus role and VARIANT helpers.
 *  - accessibilitybackenduiatext.cpp:     ITextRangeProvider and UTF-16 text unit navigation.
 */

namespace EE { namespace UI { namespace Uia {

using namespace EE::Scene;

constexpr DWORD ProviderCallTimeoutMilliseconds = 2000;

inline UINT accessibilityDispatchMessage() {
	static const UINT message = RegisterWindowMessageW( L"eepp.UIAutomation.Dispatch" );
	return message;
}

class UIAutomationProvider;

/** A text element's contents at one revision, converted once and shared by every range query
 * until the text changes. Immutable, so UIA threads may read it while the UI thread replaces it. */
struct TextContents {
	/** UTF-16 of the exposed text, the base for code-point offsets. */
	std::u16string source;
	/** What UIA sees: `source` with CRLF line endings. */
	std::u16string text;
};

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

		explicit TypedPendingRequest( Callback callback, IUnknown* owner ) :
			mCallback( std::move( callback ) ), mOwner( owner ) {
			if ( mOwner )
				mOwner->AddRef();
		}

		~TypedPendingRequest() override {
			if ( mOwner )
				mOwner->Release();
		}

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
		IUnknown* mOwner{};
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
		const bool listening = UiaClientsAreListening();
		mClientsListening.store( listening, std::memory_order_release );
		if ( !listening && mAdvisedEventCount.load( std::memory_order_acquire ) == 0 )
			mClientObserved.store( false, std::memory_order_release );
	}

	bool hasActiveClients() const {
		// A listener elsewhere on the desktop is not evidence that it uses this window.
		return isAlive() && ( mAdvisedEventCount.load( std::memory_order_acquire ) != 0 ||
							  ( mClientsListening.load( std::memory_order_acquire ) &&
								mClientObserved.load( std::memory_order_acquire ) ) );
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

	template <typename T, typename Callback>
	HRESULT invoke( T& value, Callback&& callback, IUnknown* owner = nullptr ) {
		if ( !isAlive() )
			return UIA_E_ELEMENTNOTAVAILABLE;
		if ( GetCurrentThreadId() == mUIThread ) {
			if ( !mManager )
				return UIA_E_ELEMENTNOTAVAILABLE;
			return callback( *mManager, value );
		}

		using Request = TypedPendingRequest<T>;
		auto request = std::make_shared<Request>(
			typename Request::Callback{ std::forward<Callback>( callback ) }, owner );
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

	void pruneSource( AccessibilitySourceId sourceId );

	void disconnectInvalidatedProviders();

	void detach();

	/** UI thread only: the element's contents at its current revision, from a small cache. */
	std::shared_ptr<const TextContents> textContents( AccessibilityManager& manager,
													  AccessibilityNodeRef ref );

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
	/** Set, under mProviderMutex, whenever mInvalidatedProviders gains entries: the per-update
	 * drain skips the lock while there is nothing to release. */
	std::atomic<bool> mHasInvalidatedProviders{ false };
	std::atomic<long> mAdvisedEventCount{ 0 };
	std::mutex mProviderMutex;
	std::unordered_map<AccessibilitySourceId, std::unordered_map<Uint64, UIAutomationProvider*>>
		mProviders;
	std::vector<UIAutomationProvider*> mInvalidatedProviders;
	std::mutex mPendingMutex;
	std::vector<std::shared_ptr<PendingRequest>> mPendingRequests;
	struct CachedTextContents {
		AccessibilityNodeRef ref;
		AccessibilityTextRevision revision;
		std::shared_ptr<const TextContents> contents;
	};
	/** A handful of entries covers the focused editor and the fields around it. */
	static constexpr size_t TextCacheSize = 4;
	std::vector<CachedTextContents> mTextCache;
	size_t mNextTextCacheEntry{ 0 };
};

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
	std::shared_ptr<const TextContents> contents;
	AccessibilityActions actions{};
	int selectionStart{};
	int selectionEnd{};
	Math::Rectf bounds;
	bool boundsValid{ false };

	const std::u16string& source() const { return contents->source; }

	const std::u16string& text() const { return contents->text; }
};

// Role, VARIANT and runtime id helpers: accessibilitybackenduiaprovider.cpp

CONTROLTYPEID controlType( AccessibilityRole role );

bool isSelectionContainerRole( AccessibilityRole role );

HRESULT setBstr( VARIANT* value, const String& string );

HRESULT setNotSupported( VARIANT* value );

std::vector<int> runtimeIdValues( LONG scope, AccessibilityNodeRef ref );

SAFEARRAY* safeArrayFromInts( const std::vector<int>& values );

// UTF-16 text helpers: accessibilitybackenduiatext.cpp

size_t nextSourceCodePoint( const std::u16string& text, size_t index );

size_t codePointToUTF16( const std::u16string& source, Int32 offset );

size_t codePointToUTF16( const String& string, Int32 offset );

Int32 utf16ToCodePoint( const std::u16string& source, size_t offset );

Int32 utf16ToCodePoint( const String& string, size_t offset );

std::u16string normalizedUTF16( const std::u16string& source );

std::u16string normalizedUTF16( const String& string );

HRESULT textSnapshot( const std::shared_ptr<UIAutomationProviderContext>& context,
					  AccessibilityNodeRef ref, TextSnapshot& snapshot );

bool isLowSurrogate( char16_t value );

bool isHighSurrogate( char16_t value );

size_t nextCharacterBoundary( const std::u16string& text, size_t offset );

size_t previousCharacterBoundary( const std::u16string& text, size_t offset );

bool isTextSpace( char16_t value );

std::pair<size_t, size_t> enclosingTextUnit( const std::u16string& text, size_t offset,
											 TextUnit unit );

size_t moveTextEndpoint( const std::u16string& text, size_t offset, TextUnit unit, int count,
						 int& moved );

class UIAutomationTextRange final : public ITextRangeProvider {
  public:
	UIAutomationTextRange( std::shared_ptr<UIAutomationProviderContext> context,
						   AccessibilityNodeRef ref, int start, int end ) :
		mContext( std::move( context ) ), mRef( ref ), mStart( start ), mEnd( end ) {}

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID interfaceId, void** object ) override;

	ULONG STDMETHODCALLTYPE AddRef() override {
		return mReferences.fetch_add( 1, std::memory_order_relaxed ) + 1;
	}

	ULONG STDMETHODCALLTYPE Release() override;

	HRESULT STDMETHODCALLTYPE Clone( ITextRangeProvider** range ) override;

	HRESULT STDMETHODCALLTYPE Compare( ITextRangeProvider* range, BOOL* same ) override;

	HRESULT STDMETHODCALLTYPE CompareEndpoints( TextPatternRangeEndpoint endpoint,
												ITextRangeProvider* targetRange,
												TextPatternRangeEndpoint targetEndpoint,
												int* comparison ) override;

	HRESULT STDMETHODCALLTYPE ExpandToEnclosingUnit( TextUnit unit ) override;

	HRESULT STDMETHODCALLTYPE FindAttribute( TEXTATTRIBUTEID, VARIANT, BOOL,
											 ITextRangeProvider** range ) override;

	HRESULT STDMETHODCALLTYPE FindText( BSTR text, BOOL backward, BOOL ignoreCase,
										ITextRangeProvider** range ) override;

	HRESULT STDMETHODCALLTYPE GetAttributeValue( TEXTATTRIBUTEID, VARIANT* value ) override;

	HRESULT STDMETHODCALLTYPE GetBoundingRectangles( SAFEARRAY** rectangles ) override;

	HRESULT STDMETHODCALLTYPE GetEnclosingElement( IRawElementProviderSimple** element ) override;

	HRESULT STDMETHODCALLTYPE GetText( int maxLength, BSTR* text ) override;

	HRESULT STDMETHODCALLTYPE Move( TextUnit unit, int count, int* moved ) override;

	HRESULT STDMETHODCALLTYPE MoveEndpointByUnit( TextPatternRangeEndpoint endpoint, TextUnit unit,
												  int count, int* moved ) override;

	HRESULT STDMETHODCALLTYPE
	MoveEndpointByRange( TextPatternRangeEndpoint endpoint, ITextRangeProvider* targetRange,
						 TextPatternRangeEndpoint targetEndpoint ) override;

	HRESULT STDMETHODCALLTYPE Select() override;

	HRESULT STDMETHODCALLTYPE AddToSelection() override { return UIA_E_INVALIDOPERATION; }

	HRESULT STDMETHODCALLTYPE RemoveFromSelection() override { return UIA_E_INVALIDOPERATION; }

	HRESULT STDMETHODCALLTYPE ScrollIntoView( BOOL ) override;

	HRESULT STDMETHODCALLTYPE GetChildren( SAFEARRAY** children ) override;

  private:
	~UIAutomationTextRange() = default;

	template <typename T, typename Callback> HRESULT invoke( T& value, Callback&& callback ) const {
		return mContext->invoke(
			value, std::forward<Callback>( callback ),
			static_cast<ITextRangeProvider*>( const_cast<UIAutomationTextRange*>( this ) ) );
	}

	HRESULT currentSnapshot( TextSnapshot& snapshot ) const {
		return textSnapshot( mContext, mRef, snapshot );
	}

	std::pair<int, int> endpoints() const;

	std::pair<int, int> normalizedEndpoints( size_t length ) const;

	void setEndpoints( int start, int end );

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

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID interfaceId, void** object ) override;

	ULONG STDMETHODCALLTYPE AddRef() override {
		return mReferences.fetch_add( 1, std::memory_order_relaxed ) + 1;
	}

	ULONG STDMETHODCALLTYPE Release() override;

	HRESULT STDMETHODCALLTYPE get_ProviderOptions( ProviderOptions* options ) override;

	HRESULT STDMETHODCALLTYPE GetPatternProvider( PATTERNID patternId,
												  IUnknown** provider ) override;

	HRESULT STDMETHODCALLTYPE GetPropertyValue( PROPERTYID propertyId, VARIANT* value ) override;

	HRESULT STDMETHODCALLTYPE
	get_HostRawElementProvider( IRawElementProviderSimple** provider ) override;

	HRESULT STDMETHODCALLTYPE Navigate( NavigateDirection direction,
										IRawElementProviderFragment** provider ) override;

	HRESULT STDMETHODCALLTYPE GetRuntimeId( SAFEARRAY** runtimeId ) override;

	HRESULT STDMETHODCALLTYPE get_BoundingRectangle( UiaRect* rectangle ) override;

	HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots( SAFEARRAY** roots ) override;

	HRESULT STDMETHODCALLTYPE SetFocus() override { return perform( AccessibilityAction::Focus ); }

	HRESULT STDMETHODCALLTYPE get_FragmentRoot( IRawElementProviderFragmentRoot** root ) override;

	HRESULT STDMETHODCALLTYPE
	ElementProviderFromPoint( double x, double y, IRawElementProviderFragment** provider ) override;

	HRESULT STDMETHODCALLTYPE GetFocus( IRawElementProviderFragment** provider ) override;

	HRESULT STDMETHODCALLTYPE AdviseEventAdded( EVENTID, SAFEARRAY* ) override;

	HRESULT STDMETHODCALLTYPE AdviseEventRemoved( EVENTID, SAFEARRAY* ) override;

	HRESULT STDMETHODCALLTYPE Invoke() override { return perform( AccessibilityAction::Press ); }

	HRESULT STDMETHODCALLTYPE Toggle() override { return perform( AccessibilityAction::Toggle ); }

	HRESULT STDMETHODCALLTYPE get_ToggleState( ToggleState* state ) override;

	HRESULT STDMETHODCALLTYPE GetSelection( SAFEARRAY** selection ) override;

	HRESULT STDMETHODCALLTYPE get_CanSelectMultiple( BOOL* canSelectMultiple ) override;

	HRESULT STDMETHODCALLTYPE get_IsSelectionRequired( BOOL* selectionRequired ) override;

	HRESULT STDMETHODCALLTYPE Select() override { return perform( AccessibilityAction::Select ); }

	HRESULT STDMETHODCALLTYPE AddToSelection() override { return Select(); }

	HRESULT STDMETHODCALLTYPE RemoveFromSelection() override { return UIA_E_INVALIDOPERATION; }

	HRESULT STDMETHODCALLTYPE get_IsSelected( BOOL* selected ) override;

	HRESULT STDMETHODCALLTYPE
	get_SelectionContainer( IRawElementProviderSimple** provider ) override;

	HRESULT STDMETHODCALLTYPE SetValue( LPCWSTR value ) override;

	HRESULT STDMETHODCALLTYPE get_Value( BSTR* value ) override;

	HRESULT STDMETHODCALLTYPE get_IsReadOnly( BOOL* readOnly ) override;

	HRESULT STDMETHODCALLTYPE Expand() override { return perform( AccessibilityAction::Expand ); }

	HRESULT STDMETHODCALLTYPE Collapse() override {
		return perform( AccessibilityAction::Collapse );
	}

	HRESULT STDMETHODCALLTYPE get_ExpandCollapseState( ExpandCollapseState* state ) override;

	HRESULT STDMETHODCALLTYPE ScrollIntoView() override {
		return perform( AccessibilityAction::ScrollTo );
	}

	HRESULT STDMETHODCALLTYPE SetValue( double value ) override;

	HRESULT STDMETHODCALLTYPE get_Value( double* value ) override;

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

	HRESULT STDMETHODCALLTYPE GetVisibleRanges( SAFEARRAY** ranges ) override;

	HRESULT STDMETHODCALLTYPE RangeFromChild( IRawElementProviderSimple* child,
											  ITextRangeProvider** range ) override;

	HRESULT STDMETHODCALLTYPE RangeFromPoint( UiaPoint point, ITextRangeProvider** range ) override;

	HRESULT STDMETHODCALLTYPE get_DocumentRange( ITextRangeProvider** range ) override;

	HRESULT STDMETHODCALLTYPE
	get_SupportedTextSelection( SupportedTextSelection* selection ) override;

  private:
	~UIAutomationProvider() = default;

	template <typename T, typename Callback> HRESULT invoke( T& value, Callback&& callback ) const {
		// The request, not the waiting caller, owns this reference. A timeout may return while
		// the UI thread is still executing its callback; raw [this] captures must remain alive.
		return mContext->invoke(
			value, std::forward<Callback>( callback ),
			static_cast<IRawElementProviderSimple*>( const_cast<UIAutomationProvider*>( this ) ) );
	}

	bool unavailable() const {
		return mDetached.load( std::memory_order_acquire ) || !mContext || !mContext->isAlive();
	}

	HRESULT info( AccessibilityNodeInfo& nodeInfo, bool includeValue = true ) const;

	HRESULT perform( AccessibilityAction action );

	HRESULT rangeValue( double* value, double AccessibilityRangeInfo::* member );

	std::atomic<ULONG> mReferences{ 1 };
	std::shared_ptr<UIAutomationProviderContext> mContext;
	AccessibilityNodeRef mRef;
	bool mRoot{ false };
	std::atomic<bool> mDetached{ false };
};

}}} // namespace EE::UI::Uia

#endif

#endif
