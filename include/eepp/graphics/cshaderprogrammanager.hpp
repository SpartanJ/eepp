#ifndef EE_GRAPHICSSHADERPROGRAMANAGER_HPP
#define EE_GRAPHICSSHADERPROGRAMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cshaderprogram.hpp>

namespace EE { namespace Graphics {

/** @brief The Shader Program Manager is a singleton class that manages all the instance of Shader Programs instanciated.
	Releases the Shader Program instances automatically. So the user doesn't need to release any Shader Program instance. */
class EE_API cShaderProgramManager : public tResourceManager<cShaderProgram> {
	SINGLETON_DECLARE_HEADERS(cShaderProgramManager)

	public:
		cShaderProgramManager();

		virtual ~cShaderProgramManager();

		void Reload();
	protected:
};

}}

#endif
