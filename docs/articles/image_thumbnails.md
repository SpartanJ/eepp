# Image thumbnails and EXIF orientation

`ImageThumbnailer` is an application-owned CPU service in
`<eepp/graphics/imagethumbnailer.hpp>`. It opens sources lazily, generates thumbnails on a bounded
worker queue, and returns shared immutable `Image` values. It needs no window or graphics context.
Upload a returned image to a texture on the application's graphics thread. Generated images are
delivered before cache encoding and disk writes; persistence continues on the thumbnail worker.
`shutdown()` waits for those writes.

```cpp
using namespace EE;
using namespace EE::Graphics;
using namespace EE::System;

ImageThumbnailer thumbnails;
ImageThumbnailer::Source source;
source.key = path;                 // Stable identity without credentials.
source.revision = revision;       // Changes whenever the source changes.
source.open = [path]( CancellationToken ) -> std::unique_ptr<IOStream> {
    return std::make_unique<IOStreamFile>( path );
};
thumbnails.request( std::move( source ), { 256, 256 }, cancellation.token() )
    .then( uiExecutor, []( ImageThumbnailer::Result&& result ) {
        if ( result ) {
            // result.image owns the decoded pixels. Create the UI texture here.
        }
    } );
```

The box uses physical pixels, preserves aspect ratio and never enlarges a source. Its largest
supported dimension is 1024 pixels. Supported sources are the formats decoded by `Image`, including
SVG. Animated formats use their first frame. Reduction uses fractional pixel coverage and weights
colors by alpha, avoiding transparent-edge color fringes.

JPEGs use a sufficiently large embedded EXIF thumbnail when available. Otherwise the service tries
an optional runtime-loaded TurboJPEG library for scaled decoding and falls back to eepp's bundled
decoder. TurboJPEG headers and link libraries are not required to build eepp. Runtime bindings are shared
process-wide (once per configured library path); each decode owns its decompressor handle. WebP uses its scaled
decoder; SVG renders at the requested size. These optimizations live inside `Image`, exposed by
its `Image(IOStream&, Sizei decodeSizeHint, ...)` constructor. The hint preserves aspect ratio and
avoids enlargement; other codecs may ignore it and decode before reduction.
`Image::getInfoFromStream()` reads dimensions without decoding pixels and restores stream position.

`Source::open` runs on a thumbnail worker only on a cache miss. A VFS adapter may wait for
`VirtualFileSystem::openStream()` there; the thumbnail queue is separate from provider queues.
Supply `currentRevision` to recheck the source on the worker before publishing or saving a result.
An empty revision disables cache reuse. Identical requests share work, while each consumer has its
own cancellation token. Cancel demand when an item leaves the visible range. A full queue returns
`Error::Busy`, allowing the caller to retry later.

The defaults are two workers, 128 pending requests, a 32 MiB weighted memory cache and a 512 MiB
private disk cache. The memory cache counts pixels and estimated entry bookkeeping; it evicts the
least recently used images under pressure and has no time expiry. Consumers may keep images alive
after eviction through their shared ownership. Source limits default to 128 MiB encoded data,
64 megapixels, and an aggregate 768 MiB working-memory reservation. These limits and cache budgets
are configurable.

Disk entries are normalized images under `Sys::getCachePath("eepp")/thumbnails/v1/`, shared by eepp
applications. `Config::cacheFormat` defaults to `CacheFormat::QOI`. Alternatives are `PNG`, `WebP`
(fast lossless settings), and `TGA` (RLE). All preserve colors and alpha; QOI and WebP expand
one/two-channel grayscale to RGB/RGBA when needed. The files use their standard image format and
extension, so they can also be opened by ordinary image tools.

```cpp
ImageThumbnailer::Config config;
config.cacheFormat = ImageThumbnailer::CacheFormat::WebP;
ImageThumbnailer thumbnails( std::move( config ) );
```

`setCacheFormat()` changes the encoding for new disk requests and generated entries. In-flight
jobs keep their original encoding. Existing files and in-memory images are retained; changing
formats does not convert them. The shared disk quota and `clearCache()` cover all four formats. Keys include source identity, revision, requested box and generator version; source
identities are hashed rather than stored verbatim. Writes use temporary files and rename, corrupt
entries regenerate, and old entries are pruned to respect the budget. Set `diskCache=false` or call
`setDiskCacheEnabled(false)` to stop disk reuse and writes. `clearCache()` asynchronously clears
both caches; it does not revoke images already held by consumers.

Keep source providers and continuation executors alive until `shutdown()` has cancelled and joined
the thumbnail workers. Call it from the owning application thread. Destruction also shuts down the
service.

## EXIF orientation

`Exif::read(IOStream&)`, in `<eepp/graphics/exif.hpp>`, reads orientation and embedded JPEG thumbnail
ranges from JPEG, PNG, WebP and TIFF metadata without changing the stream position. Thumbnail
orientation can differ from the primary image's orientation. Missing or malformed metadata falls
back to normal orientation; invalid embedded thumbnail ranges are ignored.

`UIImageViewer`, including eeiv, applies EXIF orientation through rotation and mirroring at draw
time, retaining the original texture pixels. `Image::applyOrientation()` instead transforms stored
pixels; use it when producing normalized output such as cached thumbnails. Ordinary `Image`
decoding does not automatically rotate pixels.

## eefm

Image thumbnails are enabled by default. **Configure eefm → Files → Icons** provides controls to
show thumbnails, select the cache format, save them between sessions, clear the shared cache, and opt into thumbnails for
archives and remote locations. A saved choice to disable thumbnails remains respected. Only visible
rows request images, and scrolling away cancels unused requests and releases their UI textures.

The `[files]` entry `thumbnail_cache_format` in eefm's `config.cfg` accepts `qoi`, `png`, `webp`,
and `tga`. Missing or unknown values select QOI.
