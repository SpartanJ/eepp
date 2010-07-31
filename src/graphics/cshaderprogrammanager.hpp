#ifndef EE_GRAPHICSSHADERPROGRAMANAGER_HPP
#define EE_GRAPHICSSHADERPROGRAMANAGER_HPP

#include "base.hpp"
#include "cshaderprogram.hpp"
#include "../system/tresourcemanager.hpp"

namespace EE { namespace Graphics {

class EE_API cShaderProgramManager : public tResourceManager<cShaderProgram>, public cSingleton<cShaderProgramManager> {
	friend class cSingleton<cShaderProgramManager>;
	public:
		cShaderProgramManager();

		virtual ~cShaderProgramManager();

		void Reload();
	protected:
};

}}

#endif
