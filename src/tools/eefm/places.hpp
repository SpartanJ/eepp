#ifndef EEFM_PLACES_HPP
#define EEFM_PLACES_HPP

#include <eepp/core/uri.hpp>
#include <eepp/system/translator.hpp>
#include <optional>
#include <string>
#include <vector>

namespace eefm {

enum class PlaceSection { Places, Remote, Devices };

struct Place {
	std::string name;
	/** Places are persisted as URIs, so remote locations behave like local ones. */
	EE::URI uri;
	/** Theme icon name. */
	std::string icon;
	PlaceSection section{ PlaceSection::Places };
	/** Discovered automatically (devices, and the places with an id): can be hidden or edited
	 * but not removed. */
	bool automatic{ false };
	bool trash{ false };
	/** Stable key of a built-in place ("home", "documents", "trash", ...), empty for the user's. */
	std::string id;
	bool hidden{ false };
};

/** @brief Bookmarked and built-in locations shown in the Places panel.
 *
 * Like Dolphin, the section follows from the URI: local locations are Places, any other scheme is
 * Remote. Built-in places can be renamed, reordered and hidden; user places can also be removed.
 * Everything is saved to the config file, in display order.
 */
class Places {
  public:
	explicit Places( std::string configPath );

	/** Loads the built-in places and applies the saved configuration on top. */
	void load();

	bool save() const;

	const std::vector<Place>& places() const { return mPlaces; }

	/** Adds a user place at the end of its section, or before the place at index before when it
	 * is of the same section (ignored if the URI is already a place). @return False if ignored. */
	bool add( Place place, std::optional<size_t> before = std::nullopt );

	/** Removes a user place; built-in places are hidden instead. */
	void remove( size_t index );

	/** Changes name, location and icon. The section follows the new location. */
	void edit( size_t index, std::string name, const EE::URI& uri, std::string icon );

	void setHidden( size_t index, bool hidden );

	/** Moves a place before (delta < 0) or after the previous / next one of its section. */
	void move( size_t index, int delta );

	/** Moves a place before another of its section, or to the end of its section (nullopt). */
	void moveBefore( size_t index, std::optional<size_t> before );

	/** True if index can move by delta within its section. */
	bool canMove( size_t index, int delta ) const;

	bool isSectionHidden( PlaceSection section ) const;

	void setSectionHidden( PlaceSection section, bool hidden );

	/** Hidden places and sections are listed anyway (dimmed) so they can be shown again. */
	bool showHidden() const { return mShowHidden; }

	void setShowHidden( bool show ) { mShowHidden = show; }

	/** True if anything is hidden (places or sections). */
	bool hasHidden() const;

	/** Mounted file systems worth showing: local disks and removable media (Devices section) and
	 * network shares such as NFS, SMB or sshfs mounts (Remote section). Reads the mount table only
	 * (never stats a mount), so it is cheap and does not block on unreachable servers. */
	static std::vector<Place> devices( const EE::System::Translator& translator );

	static std::string defaultIcon( const EE::URI& uri );

	static PlaceSection sectionFor( const EE::URI& uri );

	/** Icons offered when editing a place. */
	static const std::vector<std::string>& iconNames();

	/** sftp:// locations of the concrete hosts in ~/.ssh/config, offered when adding a place. */
	static std::vector<EE::URI> sshHosts();

  private:
	/** The built-in places, in default order. */
	static std::vector<Place> builtIn();

	std::optional<size_t> neighbor( size_t index, int delta ) const;

	std::string mPath;
	std::vector<Place> mPlaces;
	/** Defaults of the built-in places, by id: only differences are saved. */
	std::vector<Place> mDefaults;
	std::vector<PlaceSection> mHiddenSections;
	bool mShowHidden{ false };
};

} // namespace eefm

#endif
