#ifndef EE_GRAPHICSSHADERPROGRAMANAGER_HPP
#define EE_GRAPHICSSHADERPROGRAMANAGER_HPP

#include "base.hpp"
#include "cshaderprogram.hpp"

namespace EE { namespace Graphics {

class EE_API cShaderProgramManager : public cSingleton<cShaderProgramManager> {
	friend class cSingleton<cShaderProgramManager>;
	public:
		cShaderProgramManager();

		virtual ~cShaderProgramManager();

		void Add( cShaderProgram * ShaderProgram );

		void Remove( cShaderProgram * ShaderProgram );

		cShaderProgram * GetByName( const std::string& Name );

		cShaderProgram * GetById( Uint32 id );

		eeUint Count();

		void Reload();

		Uint32 ExistsName( const std::string& name );
	protected:
		std::map<std::string, cShaderProgram*> mShaders;
};

}}

#endif
