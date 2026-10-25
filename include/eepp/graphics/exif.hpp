#ifndef EE_GRAPHICS_EXIF_HPP
#define EE_GRAPHICS_EXIF_HPP

#include <eepp/graphics/image.hpp>
#include <eepp/system/iostream.hpp>

namespace EE::Graphics {

using namespace EE::System;

/** Bounded EXIF reader for JPEG APP1, PNG eXIf, WebP EXIF and TIFF metadata.
 * Reading preserves the stream position. Missing/malformed metadata yields identity orientation;
 * an invalid thumbnail is ignored independently of an otherwise valid primary orientation. */
class EE_API Exif {
  public:
	struct Metadata {
		Uint64 thumbnailOffset{ 0 };
		Uint64 thumbnailLength{ 0 };
		ImageOrientation orientation{ ImageOrientation::Normal };
		/** IFD1 orientation when present, otherwise the primary image's orientation. */
		ImageOrientation thumbnailOrientation{ ImageOrientation::Normal };

		bool hasThumbnail() const { return thumbnailLength != 0; }
	};

	static Metadata read( IOStream& stream );
};

} // namespace EE::Graphics
#endif
