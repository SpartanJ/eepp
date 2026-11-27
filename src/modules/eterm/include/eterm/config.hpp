#ifndef EE_ETERM_MODULE_CONFIG_HPP
#define EE_ETERM_MODULE_CONFIG_HPP

#include <eepp/config.hpp>

#if defined( EE_ETERM_STATIC )
#define EE_ETERM_API
#elif defined( EE_ETERM_EXPORTS )
#define EE_ETERM_API EE_API_EXPORT
#else
#define EE_ETERM_API EE_API_IMPORT
#endif

#endif
