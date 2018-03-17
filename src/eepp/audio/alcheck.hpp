#ifndef EE_ALCHECK_HPP
#define EE_ALCHECK_HPP

#include <eepp/config.hpp>

#include <AL/al.h>
#include <AL/alc.h>

namespace EE { namespace Audio {

#ifdef EE_DEBUG

	// If in debug mode, perform a test on every call
	// The do-while loop is needed so that alCheck can be used as a single statement in if/else branches
	#define alCheck(expr) do { expr; EE::Audio::alCheckError(__FILE__, __LINE__, #expr); } while (false)

#else

	// Else, we don't add any overhead
	#define alCheck(expr) (expr)

#endif

void alCheckError(const char* file, unsigned int line, const char* expression);

}}

#endif
