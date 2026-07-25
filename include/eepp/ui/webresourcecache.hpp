#ifndef EE_UI_WEBRESOURCECACHE_HPP
#define EE_UI_WEBRESOURCECACHE_HPP

#include <eepp/graphics/resource.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/time.hpp>

#include <functional>
#include <memory>
#include <string>

namespace EE { namespace UI {

/** Shared ownership handle for a WebResourceCache.
 *
 * A cache can be installed into more than one UIWebView. Sharing the cache does not by itself make
 * all resources visible between those views: sessions must also use the same CachePartitionId. */
using WebResourceCachePtr = std::shared_ptr<class WebResourceCache>;

/** Identifies one document consumer of a WebResourceCache.
 *
 * UISceneNode creates a session for its current document. A session tracks the current navigation
 * generation and the cache entries leased by that document. IDs are process-wide and opaque. */
using DocumentSessionId = Uint64;

/** Identifies a cache sharing and privacy boundary.
 *
 * Sessions in the same partition may reuse and coalesce resources. Sessions in different
 * partitions never share entries, even when requesting the same URI. A partition should therefore
 * represent one intentionally shared HTTP state, normally one cookie jar/authentication context.
 *
 * For example, browser tabs that share cookies should use the same WebResourceCache and
 * CachePartitionId. A private tab or a view logged into a different account must use another
 * partition. Passing zero to createSession() creates a new private partition automatically.
 * Partition IDs are process-wide and opaque.
 *
 * @code
 * auto cache = WebResourceCache::New();
 * auto normalProfile = cache->createPartition();
 * firstTab->setWebResourceCache( cache, normalProfile );
 * secondTab->setWebResourceCache( cache, normalProfile ); // May reuse firstTab resources.
 * privateTab->setWebResourceCache( cache, cache->createPartition() ); // Fully isolated entries.
 * @endcode */
using CachePartitionId = Uint64;

/** Canonical network origin used to describe a document navigation.
 *
 * Origins compare scheme, host, and port. Consequently HTTP and HTTPS origins are distinct, as are
 * equal hosts using different ports. Paths, queries, and fragments are not part of an origin. */
struct EE_API WebOriginKey {
	/** Lowercase URI scheme, such as "http" or "https". */
	std::string scheme;
	/** Lowercase host name. */
	std::string host;
	/** URI port, or the URI implementation's default/empty port value. */
	Uint16 port{ 0 };

	/** @return True when no origin components have been assigned. */
	bool empty() const { return scheme.empty() && host.empty() && port == 0; }
	/** @return True when both values describe the same scheme, host, and port. */
	bool operator==( const WebOriginKey& other ) const;
	/** @return True when at least one origin component differs. */
	bool operator!=( const WebOriginKey& other ) const { return !( *this == other ); }

	/** Creates an origin key from a URI, normalizing the scheme and host to lowercase. */
	static WebOriginKey fromURI( const Network::URI& uri );
};

/** Semantic type of a cached web resource.
 *
 * The kind is part of cache identity, so one URI requested as a document and as a stylesheet uses
 * distinct entries. Images additionally carry texture decode options in WebResourceRequest. */
enum class WebResourceKind : Uint8 {
	/** Top-level HTML or another document response. */
	Document,
	/** CSS stylesheet response. */
	StyleSheet,
	/** Downloadable font response. */
	Font,
	/** Image response decoded into a Graphics::Texture. */
	Image
};

/** Current lifecycle state of a cache entry. */
enum class WebResourceLoadState : Uint8 {
	/** Entry exists but has not started loading. */
	Empty,
	/** One fetch is active; additional subscribers join it. */
	Loading,
	/** Resource completed successfully and is available for reuse. */
	Ready,
	/** Last fetch failed and remains subject to the retry delay. */
	Failed,
	/** Entry was cleared and any late fetch completion will be ignored. */
	Cancelled
};

/** Complete input required to identify and fetch one web resource variant.
 *
 * Cache identity includes the partition, canonical URI, resource kind, method, non-navigation
 * headers, body, and image decode options. Cookie and Referer are deliberately excluded from entry
 * identity: the partition isolates cookie contexts, while these two values naturally change during
 * navigation. They are still sent with the HTTP request.
 *
 * Transport policy such as timeout, proxy, and completionDispatcher controls a fetch but does not
 * describe the resulting resource and is therefore not part of cache identity. */
struct EE_API WebResourceRequest {
	/** Dispatches a completion function to the thread on which it is safe to finalize a resource.
	 */
	using CompletionDispatcher = std::function<void( std::function<void()> )>;

	/** Absolute URI to fetch. Fragments are removed and the URI is normalized for cache lookup. */
	Network::URI uri;
	/** Semantic resource type. */
	WebResourceKind kind{ WebResourceKind::Document };
	/** HTTP method. The method is part of cache identity. */
	Network::Http::Request::Method method{ Network::Http::Request::Method::Get };
	/** HTTP request headers. See the type documentation for cache-key rules. */
	Network::Http::Request::FieldTable headers;
	/** HTTP request body. The complete body is part of cache identity. */
	std::string body;
	/** Maximum duration allowed for the HTTP request. */
	System::Time timeout{ System::Seconds( 5 ) };
	/** Whether HTTPS certificates must be validated. */
	bool validateCertificate{ true };
	/** Optional HTTP proxy URI. An empty URI uses a direct connection. */
	Network::URI proxy;
	/** Whether HTTP redirects should be followed. */
	bool followRedirect{ true };
	/** Image-only texture wrapping policy. True selects ClampToEdge. */
	bool clampToEdge{ true };
	/** Image-only option controlling mipmap generation. */
	bool mipmaps{ false };
	/** Image-only option requesting texture compression. */
	bool compressTexture{ false };
	/** Image-only SVG rasterization scale. */
	Float svgScale{ 1.f };
	/** Image-only completion dispatcher.
	 *
	 * Image decode/upload completion must execute where a graphics context is current. This policy
	 * is not part of cache identity and only the request that starts a coalesced load supplies it.
	 */
	CompletionDispatcher completionDispatcher;
};

/** Result delivered to a WebResourceCache subscriber.
 *
 * Data resources set data, while image resources set texture. A texture request may already have
 * returned the same TexturePtr as a transparent placeholder before this result is delivered. */
struct EE_API WebResourceResult {
	/** True when the request completed and produced a usable resource. */
	bool success{ false };
	/** Final cache-entry state observed by this completion. */
	WebResourceLoadState state{ WebResourceLoadState::Empty };
	/** Numeric HTTP response status. */
	int status{ 0 };
	/** Failure description. Empty for successful results. */
	std::string error;
	/** Set-Cookie value from the final response, or from its redirect chain when applicable. */
	std::string setCookie;
	/** Shared immutable response body for documents, stylesheets, and fonts. */
	std::shared_ptr<const std::string> data;
	/** Shared texture for image resources. */
	Graphics::TexturePtr texture;
};

/** Shared HTTP-backed document resource cache.
 *
 * The cache separates three concepts:
 * - A partition defines which HTTP/cookie context may share entries.
 * - A session represents one active document using entries from that partition.
 * - A navigation generation prevents an obsolete document from receiving asynchronous results.
 *
 * Entries are strongly retained by the cache after their final document lease is released. Their
 * TTL starts at that release point, allowing Back/Forward navigation to reuse them. Expired entries
 * are removed by prune(); UIWebView invokes it periodically. A byte budget additionally evicts the
 * least-recently-used unleased entries. Active leases and in-flight loads are not evicted.
 *
 * Requests for the same key are coalesced into one HTTP operation. Each current subscriber receives
 * the result, while callbacks belonging to obsolete navigation generations are discarded. Public
 * operations are internally synchronized. Subscriber callbacks run without the cache mutex held. */
class EE_API WebResourceCache : public std::enable_shared_from_this<WebResourceCache> {
  public:
	/** Receives the completed result of a data or texture request. */
	using Callback = std::function<void( const WebResourceResult& )>;

	/** @return A newly allocated shared web-resource cache. */
	static WebResourceCachePtr New();

	/** Creates an empty cache with a 30-second TTL, 1-second retry delay, and 64 MiB budget. */
	WebResourceCache();
	/** Cancels cache delivery and releases all retained entries and sessions. */
	~WebResourceCache();

	/** Allocates a new process-wide partition identifier.
	 * @return An opaque, non-zero partition ID. */
	CachePartitionId createPartition();

	/** Creates a document session.
	 * @param partition Partition to join. Zero creates a new private partition.
	 * @return An opaque, non-zero document session ID. */
	DocumentSessionId createSession( CachePartitionId partition = 0 );

	/** Destroys a session and releases all entries leased by it.
	 *
	 * Releasing the final lease starts each entry's TTL; it does not normally erase entries
	 * immediately. Pending callbacks for the destroyed session will not be delivered. */
	void destroySession( DocumentSessionId session );

	/** Begins a new document navigation in an existing session.
	 *
	 * Previous document leases are released and the generation is incremented. Pending subscribers
	 * from older generations become stale, but shared in-flight requests continue for other current
	 * subscribers.
	 * @param session Session performing the navigation.
	 * @param uri Destination document URI, used to record its origin.
	 * @return The new generation, or zero when the session does not exist. */
	Uint64 beginNavigation( DocumentSessionId session, const Network::URI& uri );

	/** @return The current generation for a session, or zero when it does not exist. */
	Uint64 getSessionGeneration( DocumentSessionId session ) const;
	/** @return The partition used by a session, or zero when it does not exist. */
	CachePartitionId getSessionPartition( DocumentSessionId session ) const;

	/** Requests a document, stylesheet, font, or other non-texture response body.
	 *
	 * A ready cache hit may invoke callback synchronously. A miss starts or joins an asynchronous
	 * fetch. The callback is omitted when the session/generation is stale.
	 * @param session Requesting document session.
	 * @param generation Generation returned by beginNavigation().
	 * @param request Resource and transport description.
	 * @param callback Optional completion callback. */
	void requestData( DocumentSessionId session, Uint64 generation, WebResourceRequest request,
					  Callback callback );

	/** Requests an image as a shared texture.
	 *
	 * On a miss, this creates and returns a transparent placeholder immediately. The same
	 * TexturePtr is populated after download and decode. A ready hit returns the existing texture
	 * without a new HTTP request. This function must be called where texture creation is valid, and
	 * the request must provide an appropriate completionDispatcher for asynchronous GPU upload.
	 * @return The cached/new texture, or null for an invalid or stale session. */
	Graphics::TexturePtr requestTexture( DocumentSessionId session, Uint64 generation,
										 WebResourceRequest request, Callback callback = {} );

	/** Sets the retention duration used when an entry's final document lease is released. */
	void setTTL( System::Time ttl );
	/** @return The current post-lease retention duration. */
	System::Time getTTL() const;
	/** Sets how long a failed entry suppresses another fetch attempt. */
	void setRetryDelay( System::Time delay );
	/** @return The current failed-request retry delay. */
	System::Time getRetryDelay() const;
	/** Sets the maximum retained response/texture byte estimate.
	 *
	 * Applying a smaller budget immediately evicts least-recently-used unleased entries where
	 * possible. Active entries may temporarily make retained bytes exceed the budget. */
	void setByteBudget( std::size_t bytes );
	/** @return The configured retained-byte budget. */
	std::size_t getByteBudget() const;
	/** @return Estimated bytes retained by ready data bodies and decoded image textures. */
	std::size_t getRetainedBytes() const;
	/** @return Number of cache entries in all states and partitions. */
	std::size_t getEntryCount() const;
	/** @return Number of HTTP requests belonging to the cache's current epoch.
	 *
	 * `clear()` resets this count even though invalidated underlying operations may still finish;
	 * those stale completions are ignored and do not alter the count. */
	std::size_t getInFlightCount() const;

	/** Removes expired entries and enforces the byte budget.
	 *
	 * Active document leases and loading entries are preserved. UIWebView calls this periodically;
	 * other cache owners should provide their own maintenance point. */
	void prune();

	/** Removes every entry immediately and suppresses delivery from outstanding fetches.
	 *
	 * Sessions remain valid but no longer lease entries. This does not cancel the underlying HTTP
	 * operation. A late completion is ignored using its operation identity, including when a new
	 * request has already recreated an entry with the same cache key. */
	void clear();

	/** Completion supplied to a custom Fetcher. */
	using FetchCompletion = std::function<void( Network::Http::Response )>;
	/** Custom fetch implementation, primarily intended for deterministic tests.
	 *
	 * The fetcher must eventually invoke FetchCompletion exactly once. Production uses
	 * Http::requestAsync when no custom fetcher is installed. */
	using Fetcher = std::function<void( const WebResourceRequest&, FetchCompletion )>;

	/** Installs or clears a custom HTTP fetch implementation. */
	void setFetcher( Fetcher fetcher );

  private:
	/** Private implementation containing synchronized sessions and cache entries. */
	struct Impl;
	/** Exclusive implementation state. */
	std::unique_ptr<Impl> mImpl;
};

}} // namespace EE::UI

#endif
