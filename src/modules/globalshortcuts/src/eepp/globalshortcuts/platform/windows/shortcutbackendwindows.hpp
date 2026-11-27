#ifndef EE_GLOBALSHORTCUTS_SHORTCUTBACKENDWINDOWS_HPP
#define EE_GLOBALSHORTCUTS_SHORTCUTBACKENDWINDOWS_HPP

#include <eepp/globalshortcuts/shortcutbackend.hpp>
#include <memory>

namespace EE::System::GlobalShortcuts::Private {

std::unique_ptr<ShortcutBackend> createWindowsShortcutBackend();

} // namespace EE::System::GlobalShortcuts::Private

#endif
