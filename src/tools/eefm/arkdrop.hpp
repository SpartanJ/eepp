#ifndef EEFM_ARKDROP_HPP
#define EEFM_ARKDROP_HPP

#include <eepp/window/draganddrop.hpp>
#include <string>
#include <vector>

namespace eefm {

/** @brief Entries dragged out of an archive open in KDE's Ark. The drag carries no files: it
 * names a D-Bus object of the Ark instance, which the drop target asks to extract the dragged
 * entries into a folder (as Dolphin does). */
class ArkDrop {
  public:
	ArkDrop() = delete;

	/** The types of an Ark drag, to register with DragAndDrop::setDataDropHandler(). */
	static const std::vector<std::string>& mimeTypes();

	/** Whether drop came from Ark. */
	static bool isArkDrop( const EE::Window::DataDrop& drop );

	/** Asks the Ark instance that started drop to extract its dragged entries into folder (a
	 * local path). A blocking D-Bus call: Ark extracts on its own afterwards and shows its own
	 * progress and overwrite questions. Returns an error message, empty on success. */
	static std::string extract( const EE::Window::DataDrop& drop, const std::string& folder );
};

} // namespace eefm

#endif
