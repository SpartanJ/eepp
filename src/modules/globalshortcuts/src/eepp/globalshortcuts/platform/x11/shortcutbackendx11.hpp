#ifndef EE_GLOBALSHORTCUTS_SHORTCUTBACKENDX11_HPP
#define EE_GLOBALSHORTCUTS_SHORTCUTBACKENDX11_HPP

#include <eepp/globalshortcuts/shortcutbackend.hpp>
#include <memory>

namespace EE::System::GlobalShortcuts::Private {

std::unique_ptr<ShortcutBackend> createX11ShortcutBackend();

} // namespace EE::System::GlobalShortcuts::Private

#endif
