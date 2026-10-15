#ifndef EE_EXAMPLE_NATIVE_DRAG_AND_DROP_HPP
#define EE_EXAMPLE_NATIVE_DRAG_AND_DROP_HPP

#include <string>
#include <vector>

namespace EE { namespace Examples {

/** Shared source-card demo; manual tests also open an application-owned receiving window. */
int runNativeDragDemo( std::vector<std::string> arguments, bool withDropTarget = false );

}} // namespace EE::Examples

#endif
