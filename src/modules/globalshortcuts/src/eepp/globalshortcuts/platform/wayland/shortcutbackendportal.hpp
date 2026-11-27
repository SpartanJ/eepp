#ifndef EE_GLOBALSHORTCUTS_SHORTCUTBACKENDPORTAL_HPP
#define EE_GLOBALSHORTCUTS_SHORTCUTBACKENDPORTAL_HPP

#include <eepp/globalshortcuts/shortcutbackend.hpp>

namespace EE::System::GlobalShortcuts::Private {

bool isWaylandShortcutSession();

std::unique_ptr<ShortcutBackend> createPortalShortcutBackend( Options options );

} // namespace EE::System::GlobalShortcuts::Private

#endif
