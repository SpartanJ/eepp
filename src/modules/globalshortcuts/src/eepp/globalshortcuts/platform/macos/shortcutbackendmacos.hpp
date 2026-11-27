#ifndef EE_GLOBALSHORTCUTS_SHORTCUTBACKENDMACOS_HPP
#define EE_GLOBALSHORTCUTS_SHORTCUTBACKENDMACOS_HPP

#include <eepp/globalshortcuts/shortcutbackend.hpp>

namespace EE::System::GlobalShortcuts::Private {

std::unique_ptr<ShortcutBackend> createMacOSShortcutBackend();

} // namespace EE::System::GlobalShortcuts::Private

#endif
