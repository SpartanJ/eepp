#ifndef EEFM_PLACESCONTROLLER_HPP
#define EEFM_PLACESCONTROLLER_HPP

#include "places.hpp"
#include "placespanel.hpp"

#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace eefm {

class App;

/** @brief The Places panel: the saved places and the discovered devices, their menus and the
 * place dialog. */
class PlacesController {
  public:
	/** Loads the places saved in configPath. */
	PlacesController( App& app, const std::string& configPath );

	~PlacesController();

	/** Creates the panel inside parent and lists the places. */
	void createPanel( UIWidget* parent );

	/** The panel, null before createPanel(). */
	PlacesPanel* panel() const { return mPlacesPanel; }

	/** Lists the places and devices again. */
	void refresh();

	/** Usage bars of the local devices (asynchronous). */
	void refreshDeviceUsage();

	/** Adds a place for uri (or shows it again, if it is a hidden place). */
	void add( const URI& uri );

	/** Writes the places configuration. */
	void saveConfig();

  private:
	void showPlaceMenu( const PlacesPanel::Entry& entry );

	void showSectionMenu( PlaceSection section );

	/** Items shared by the place and section menus (Add Entry, Show Hidden Places, Hide
	 * Section). */
	void addPanelItems( UIPopUpMenu* menu, PlaceSection section );

	/** @return True if id was one of addPanelItems' items. */
	bool onPanelItem( const std::string& id, PlaceSection section );

	/** Add (index empty) or edit a place: label, location and icon. */
	void showPlaceDialog( std::optional<size_t> index, Place initial );

	/** Saves the configuration and lists the places again. */
	void save();

	App& mApp;
	UISceneNode* mUI;
	std::unique_ptr<Places> mPlaces;
	std::vector<Place> mDevices;
	PlacesPanel* mPlacesPanel{ nullptr };
};

} // namespace eefm

#endif
