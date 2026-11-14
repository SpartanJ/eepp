#ifndef EE_UI_TOOLS_FILEASSOCIATIONSSETTINGS_HPP
#define EE_UI_TOOLS_FILEASSOCIATIONSSETTINGS_HPP

#include <eepp/system/fileassociation.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>

namespace EE::UI::Tools {

/** Shared file-association settings with per-extension selection and background registration. */
class EE_API FileAssociationsSettings {
  public:
	using Translate = std::function<String( const std::string&, const String& )>;
	using Completion = std::function<void( bool success, const std::string& error )>;

	/** Describes the running application, resolving portable Unix package launchers. */
	static System::FileAssociationApplication
	applicationForCurrentProcess( const std::string& name, std::string iconPath = {} );

	/** Adds a launcher-only Desktop Integration category on XDG platforms. */
	static void addLauncherTo( UISettingsPanel* panel, const std::string& applicationName );

	/** Adds controls to an existing category. An empty extension list shows only launcher controls.
	 * Completion runs on the UI thread while panel lives.
	 */
	static void addTo( UISettingsPanel* panel, SettingDescriptor descriptor,
					   System::FileAssociationApplication application,
					   std::vector<std::string> extensions,
					   std::shared_ptr<System::ThreadPool> threadPool, Translate translate = {},
					   Completion completion = {} );

	/** Adds one checkbox that enables/disables the supplied exact MIME types together.
	 * Checking it registers the types in the background; unchecking removes the associations.
	 * Launcher visibility is preserved in both cases. Completion runs on the UI
	 * thread while the panel lives. A failed change restores the previous checkbox value. */
	static void addMimeTypesTo( UISettingsPanel* panel, SettingDescriptor descriptor,
								System::FileAssociationApplication application,
								std::vector<std::string> mimeTypes,
								std::shared_ptr<System::ThreadPool> threadPool,
								Completion completion = {} );
};

} // namespace EE::UI::Tools

#endif
