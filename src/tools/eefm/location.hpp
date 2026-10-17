#ifndef EEFM_LOCATION_HPP
#define EEFM_LOCATION_HPP

#include <eepp/core/string.hpp>
#include <eepp/core/uri.hpp>
#include <string>

namespace eefm {

using namespace EE;

/** How locations are typed, shown and copied. */
class Location {
  public:
	/** A typed location: a URI, or a local path ("~" expands to the home folder). */
	static URI parse( const std::string& text );

	/** Parent location; leaving an archive root returns to the archive's containing folder. */
	static URI parent( const URI& uri );

	/** Entry to select when leaving this location (the archive's name at its root). */
	static std::string entryName( const URI& uri );

	/** The name of a folder for titles: its last segment, "/" or the host for roots. */
	static String displayName( const URI& uri );

	/** The location bar text: a local path with a trailing slash, the URI otherwise. */
	static String text( const URI& uri );

	/** A location for Copy Location: a plain path (no trailing slash) for local files and
	 * folders, the URI otherwise. Copying files (Ctrl+C) puts URIs in the clipboard instead. */
	static std::string clipboardText( const URI& uri );
};

} // namespace eefm

#endif
