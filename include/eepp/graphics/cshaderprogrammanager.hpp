#ifndef EE_GRAPHICSSHADERPROGRAMANAGER_HPP
#define EE_GRAPHICSSHADERPROGRAMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cshaderprogram.hpp>

namespace EE { namespace Graphics {

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
