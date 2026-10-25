#ifndef EE_GRAPHICS_PRIVATE_TURBOJPEG_HPP
#define EE_GRAPHICS_PRIVATE_TURBOJPEG_HPP

#include <eepp/graphics/image.hpp>
#include <eepp/system/iostream.hpp>

#if defined( _MSC_VER )
#define EE_TJ_CALL __cdecl
#elif defined( _WIN32 ) && defined( __i386__ )
#define EE_TJ_CALL __attribute__( ( cdecl ) )
#else
#define EE_TJ_CALL
#endif

namespace EE::Graphics::Private {

using namespace EE::System;

/** Optional C ABI adapter. Intentionally independent of TurboJPEG headers and import libraries. */
class TurboJpeg {
  public:
	/** Process-wide bindings, initialized once per library path (empty selects auto-detection).
	 * Decoder handles remain local to each decode, so concurrent calls are independent. */
	static const TurboJpeg& instance( const std::string& libraryPath = {} );

	~TurboJpeg();

	TurboJpeg( const TurboJpeg& ) = delete;

	TurboJpeg& operator=( const TurboJpeg& ) = delete;

	bool available() const { return mLibrary != nullptr; }

	Image decode( IOStream& stream, Sizei target, Uint64 maxPixels ) const;

  private:
	explicit TurboJpeg( const std::string& libraryPath );

	struct ScalingFactor {
		int numerator, denominator;
	};
	using Init = void*(EE_TJ_CALL*)();
	using Header = int( EE_TJ_CALL* )( void*, const unsigned char*, unsigned long, int*, int*, int*,
									   int* );
	using Scales = ScalingFactor*(EE_TJ_CALL*)( int* );
	using Decode = int( EE_TJ_CALL* )( void*, const unsigned char*, unsigned long, unsigned char*,
									   int, int, int, int, int );
	using Destroy = int( EE_TJ_CALL* )( void* );

	bool load( const std::string& path );

	void* mLibrary{ nullptr };
	Init mInit{ nullptr };
	Header mHeader{ nullptr };
	Scales mScales{ nullptr };
	Decode mDecode{ nullptr };
	Destroy mDestroy{ nullptr };
};

} // namespace EE::Graphics::Private
#undef EE_TJ_CALL
#endif
