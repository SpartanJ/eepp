#ifndef EE_GRAPHICSCSHADER_H
#define EE_GRAPHICSCSHADER_H

#include "base.hpp"

namespace EE { namespace Graphics {

/** @brief The basic shader class. */
class EE_API cShader {
	public:
		/** Constructor with type of shader, next you'll need to set the source and compile it. */
    	cShader( const Uint32& Type );

    	/** Create a type of shader and load the shader from a file, and compile it. */
    	cShader( const Uint32& Type, const std::string& Filename );

    	virtual ~cShader();

    	/** Set the shader source */
    	void SetSource( const std::string& Source );

    	/** Set the shader source */
    	void SetSource( const std::vector<char>& Source );

    	/** Compile the shader */
    	bool Compile();

    	/** @return If the shader is valid */
    	bool IsValid() const { return mValid; }

    	/** @return If the shader is compiled */
    	bool IsCompiled() const { return mCompiled; }

    	/** @return The log of the compilation */
    	std::string CompileLog() const { return mCompileLog; }

    	/** @return The Shader Type */
    	Uint32 GetType() const { return mType; }

    	/** @return The Shader Id */
    	Uint32 GetId() const { return mGLId; }

    	/** Reloads the Shader. */
    	void Reload();
    protected:
    	GLuint mGLId;
    	GLenum mType;
    	bool mValid;
    	bool mCompiled;
    	std::string mCompileLog;
        std::vector<char> mSource;

    	void Init( const Uint32& Type );
};

/** @brief Prebuild Vertex Shader class */
class EE_API cVertexShader : public cShader {
	public:
    	cVertexShader();
    	cVertexShader( const std::string& Filename );
};

/** @brief Prebuild Fragment Shader class */
class EE_API cFragmentShader : public cShader {
	public:
    	cFragmentShader();
    	cFragmentShader( const std::string& Filename );
};

}}

#endif



