#ifndef EE_GRAPHICS_IMAGETHUMBNAILER_HPP
#define EE_GRAPHICS_IMAGETHUMBNAILER_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/small_unique_function.hpp>
#include <eepp/core/task.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/system/iostream.hpp>
#include <memory>
#include <string>
#include <string_view>

namespace EE::Graphics {

using namespace EE::System;

/** Application-owned CPU thumbnail service. All source/cache I/O and decoding run on its own
 * bounded worker queue. No UI or graphics context is required. Shut down before source providers
 * or continuation executors are destroyed. Returned images are shared immutable cache values. */
class EE_API ImageThumbnailer {
  public:
	enum class Error : Uint8 { None, Cancelled, Busy, Unsupported, TooLarge, IO, Changed };

	/** Lossless on-disk encodings. WebP uses its fast lossless settings; TGA uses RLE. */
	enum class CacheFormat : Uint8 { QOI, PNG, WebP, TGA };

	struct Result {
		std::shared_ptr<const Image> image;
		Error error{ Error::None };
		bool cached{ false };

		explicit operator bool() const { return image != nullptr; }
	};

	struct Config {
		/** Empty selects Sys::getCachePath("eepp")/thumbnails/v1. */
		std::string cachePath;
		/** Optional explicit runtime library path. No TurboJPEG headers/linking are required. */
		std::string turboJPEGPath;
		/** Decoded pixels plus estimated per-entry bookkeeping. */
		Uint64 memoryCacheBytes{ 32 * 1024 * 1024 };
		Uint64 diskCacheBytes{ 512 * 1024 * 1024 };
		Uint64 maxSourceBytes{ 128 * 1024 * 1024 };
		Uint64 maxPixels{ 64 * 1024 * 1024 };
		/** Aggregate working-memory reservation, including compressed input and decoder overhead.
		 */
		Uint64 decodeBytes{ 768 * 1024 * 1024 };
		Uint32 workers{ 2 };
		Uint32 maxPending{ 128 };
		bool diskCache{ true };
		bool useTurboJPEG{ true };
		CacheFormat cacheFormat{ CacheFormat::QOI };
	};

	struct Source {
		/** Stable, credential-free identity; never persisted verbatim. */
		std::string key;
		/** Caller-defined persistent freshness signature. Empty disables cache reuse. */
		std::string revision;
		/** Called only on a cache miss, on a thumbnail worker. Owns the opened stream. */
		SmallUniqueFunction<std::unique_ptr<IOStream>( CancellationToken ), 64> open;
		/** Rechecks freshness on the worker after generation. Empty means no revalidation. */
		SmallUniqueFunction<std::string(), 64> currentRevision;
		bool persistent{ true };
	};

	ImageThumbnailer();

	explicit ImageThumbnailer( Config config );

	~ImageThumbnailer();

	ImageThumbnailer( const ImageThumbnailer& ) = delete;

	ImageThumbnailer& operator=( const ImageThumbnailer& ) = delete;

	/** Physical-pixel box, up to 1024px. Higher priorities run first; duplicate requests share
	 * work, while cancellation belongs to each consumer. A full queue returns Error::Busy.
	 * Generated pixels are delivered before disk persistence; shutdown joins pending writes. */
	Task<Result> request( Source source, Sizei box, CancellationToken token = {},
						  int priority = 0 );

	/** Cancels and joins workers. Call from the owning application thread, not a continuation
	 * executing inline on a thumbnail worker. Safe to call more than once. */
	void shutdown();

	/** Clears both caches on a worker, serialized against disk cache writes. */
	Task<void> clearCache();

	void setDiskCacheEnabled( bool enabled );

	/** Applies to new disk requests/writes. In-memory images and existing files are retained. */
	void setCacheFormat( CacheFormat format );

	static const char* cacheFormatToString( CacheFormat format );

	/** Unknown names select QOI. Accepts qoi, png, webp and tga, case-insensitively. */
	static CacheFormat cacheFormatFromString( std::string_view name );

	/** Fractional-area reduction with alpha-weighted colors. Preserves aspect ratio and never
	 * enlarges an image. The caller owns the returned pixels; no graphics API is used. */
	static Image reduce( const Image& image, Sizei box, CancellationToken token = {} );

  private:
	struct Impl;
	std::unique_ptr<Impl> mImpl;
};

} // namespace EE::Graphics
#endif
