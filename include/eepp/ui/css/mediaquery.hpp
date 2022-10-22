#ifndef EE_UI_CSS_MEDIAQUERY_HPP
#define EE_UI_CSS_MEDIAQUERY_HPP

#include <eepp/core.hpp>
#include <memory>

namespace EE { namespace UI { namespace CSS {

/** Media Queries support is based on the litehtml implementation:
 * https://github.com/litehtml/litehtml licensed under the New BSD License.
 */

enum MediaOrientation {
	media_orientation_portrait,
	media_orientation_landscape,
};

enum MediaFeature {
	media_feature_none,

	media_feature_width,
	media_feature_min_width,
	media_feature_max_width,

	media_feature_height,
	media_feature_min_height,
	media_feature_max_height,

	media_feature_device_width,
	media_feature_min_device_width,
	media_feature_max_device_width,

	media_feature_device_height,
	media_feature_min_device_height,
	media_feature_max_device_height,

	media_feature_orientation,

	media_feature_aspect_ratio,
	media_feature_min_aspect_ratio,
	media_feature_max_aspect_ratio,

	media_feature_device_aspect_ratio,
	media_feature_min_device_aspect_ratio,
	media_feature_max_device_aspect_ratio,

	media_feature_color,
	media_feature_min_color,
	media_feature_max_color,

	media_feature_color_index,
	media_feature_min_color_index,
	media_feature_max_color_index,

	media_feature_monochrome,
	media_feature_min_monochrome,
	media_feature_max_monochrome,

	media_feature_resolution,
	media_feature_min_resolution,
	media_feature_max_resolution,

	media_feature_pixel_density,
	media_feature_min_pixel_density,
	media_feature_max_pixel_density,

	media_feature_prefers_color_scheme,
};

enum MediaType {
	media_type_none,
	media_type_all,
	media_type_screen,
	media_type_print,
	media_type_braille,
	media_type_embossed,
	media_type_handheld,
	media_type_projection,
	media_type_speech,
	media_type_tty,
	media_type_tv,
};

struct MediaFeatures {
	MediaType type;
	int width;	// (pixels) For continuous media, this is the width of the viewport including the
				// size of a rendered scroll bar (if any). For paged media, this is the width of the
				// page box.
	int height; // (pixels) The height of the targeted display area of the output device. For
				// continuous media, this is the height of the viewport including the size of a
				// rendered scroll bar (if any). For paged media, this is the height of the page
				// box.
	int deviceWidth;  // (pixels) The width of the rendering surface of the output device. For
					  // continuous media, this is the width of the screen. For paged media, this is
					  // the width of the page sheet size.
	int deviceHeight; // (pixels) The height of the rendering surface of the output device. For
					  // continuous media, this is the height of the screen. For paged media, this
					  // is the height of the page sheet size.
	int color; // The number of bits per color component of the output device. If the device is not
			   // a color device, the value is zero.
	int colorIndex; // The number of entries in the color lookup table of the output device. If the
					// device does not use a color lookup table, the value is zero.
	int monochrome; // The number of bits per pixel in a monochrome frame buffer. If the device is
					// not a monochrome device, the output device value will be 0.
	int resolution;							  // The resolution of the output device (in DPI)
	float pixelDensity;						  // Screen pixel density
	std::string prefersColorScheme{ "dark" }; // Color Scheme Default Preference
};

struct EE_API MediaQueryExpression {
	typedef std::vector<MediaQueryExpression> vector;

	MediaFeature feature;
	int val{ 0 };
	int val2{ 0 };
	float fval{ 0 };
	float fval2{ 0 };
	bool checkAsBool{ false };
	std::string valStr;

	MediaQueryExpression();

	bool check( const MediaFeatures& features ) const;
};

class EE_API MediaQuery {
  public:
	typedef std::shared_ptr<MediaQuery> ptr;
	typedef std::vector<MediaQuery::ptr> vector;

	MediaQuery();

	MediaQuery( const MediaQuery& val );

	static MediaQuery::ptr parse( const std::string& str );

	bool check( const MediaFeatures& features ) const;

  private:
	MediaQueryExpression::vector mExpressions;
	bool mNot;
	MediaType mMediaType;
};

class EE_API MediaQueryList {
  public:
	typedef std::shared_ptr<MediaQueryList> ptr;
	typedef std::vector<MediaQueryList::ptr> vector;

	MediaQueryList();

	MediaQueryList( const MediaQueryList& val );

	static MediaQueryList::ptr parse( const std::string& str );

	bool isUsed() const;

	bool applyMediaFeatures( const MediaFeatures& features ); // returns true if the isUsed changed

	const Uint32& getMarker() const;

	void setMarker( const Uint32& marker );

	const std::string& getQueryString() const { return mQueryStr; }

  private:
	Uint32 mMarker{ 0 };
	MediaQuery::vector mQueries;
	bool mUsed;
	std::string mQueryStr;
};

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_MEDIAQUERY_HPP
