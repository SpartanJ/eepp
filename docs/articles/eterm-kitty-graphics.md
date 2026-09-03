# eTerm Kitty graphics protocol

eTerm implements the terminal-receiver side of the Kitty graphics protocol. Protocol parsing,
decoding, image storage, placement state, animation, and terminal lifecycle integration run on the
terminal worker. The UI thread consumes immutable placement metadata and an ordered pixel-update
stream, and exclusively owns the OpenGL texture cache.

## Supported features

- direct RGB and RGBA transmission, including chunking and zlib compression;
- POSIX shared-memory transmission, including byte offsets and explicit sizes;
- PNG transmission;
- image IDs, image numbers, placement IDs, source cropping, cell geometry, and pixel offsets;
- root-image rectangle updates, animation frames, frame composition, and animation control;
- deletion selectors, quiet modes, capability queries, storage quotas, usage hints, and eviction;
- primary/alternate-screen state, reset, clear, scrolling, margins, scrollback, and resize;
- Unicode placeholder placements (U+10EEEE and Kitty's combining-diacritic encoding);
- relative placements, including parent validation, cycle detection, and descendant movement;
- terminal pixel-size queries and SGR pixel mouse coordinates.

Local file and temporary-file transfer media are intentionally not enabled. POSIX shared-memory
objects are opened read-only and immediately unlinked as required by the protocol. Direct
transmission remains the portable path and works through SSH.

## Resource and threading behavior

Decoded root images are stored as premultiplied-independent RGBA8 CPU buffers on the worker. GPU
textures exist only on the UI/GL thread. Incremental updates carry monotonically increasing sequence
numbers; a bounded queue gap triggers a complete graphics resynchronization. Image count, placement
count, APC size, decoded transfer size, and total image storage are bounded. Unreferenced images are
evicted oldest-first when storage is needed.

Graphics follow the same presentation cadence and DEC synchronized-update boundary as text. A
placement anchored in normal-screen history moves with terminal scrolling and is projected into the
current scrollback viewport. Margin scrolling clips or removes only placements wholly participating
in the scrolled region. Resize preserves screen-coordinate placements and reflows Unicode placeholder
metadata with its text cells.

## Compatibility testing

The eTerm unit suite covers fragmented APC input, malformed/fuzzed commands, raw/zlib/PNG transfer,
independent image namespaces, replacement, partial updates, placement geometry, scrolling and
scrollback, screen lifecycle, animation, Unicode placeholders, relative placement graphs, ordered
worker/UI delivery, queue overflow, and resynchronization.
