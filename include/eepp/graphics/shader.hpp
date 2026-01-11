#ifndef EE_GRAPHICSCSHADER_H
#define EE_GRAPHICSCSHADER_H

#include <eepp/graphics/base.hpp>

#include <eepp/system/pack.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief The basic shader class. */
class EE_API Shader {
  public:
	/** Activates/Deactivates shader conversion from fixed pipeline to programmable pipeline (
	 * activated by default ) */
	static void ensure( bool ensure );

	/** @return If automatic Shader conversion is activated */
	static bool ensure();

	/** Constructor with type of shader, next you'll need to set the source and compile it. */
	Shader( const Uint32& Type );

	/** Create a type of shader and load the shader from a file, and compile it. */
	Shader( const Uint32& Type, const std::string& Filename );

	/** Create a type of shader from memory, and compile it. */
	Shader( const Uint32& Type, const char* Data, const Uint32& DataSize );

	/** Create a type of shader loaded from a pack file */
	Shader( const Uint32& Type, Pack* Pack, const std::string& Filename );

	/** Create a type of shader from memory, and compile it. */
	Shader( const Uint32& Type, const char** Data, const Uint32& NumLines );

	virtual ~Shader();

	/** Set the shader source */
	void setSource( const std::string& Source );

	/** Set the shader source */
	void setSource( const std::vector<Uint8>& Source );

	/** Set the shader source */
	void setSource( const char* Data, const Uint32& DataSize );

	/** Set the shader source */
	void setSource( const char** Data, const Uint32& NumLines );

	/** Compile the shader */
	bool compile();

	/** @return If the shader is valid */
	bool isValid() const;

	/** @return If the shader is compiled */
	bool isCompiled() const;

	/** @return The log of the compilation */
	std::string compileLog() const;

	/** @return The Shader Type */
	Uint32 getType() const;

	/** @return The Shader Id */
	Uint32 getId() const;

	/** Reloads the Shader. */
	void reload();

  protected:
	friend class RendererGL3;
	static bool sEnsure;
	Uint32 mGLId;
	Uint32 mType;
	std::string mFilename;
	std::string mCompileLog;
	std::string mSource;
	bool mValid;
	bool mCompiled;

	void Init( const Uint32& Type );

	std::string getName();

	void ensureVersion();
};

/** @brief Prebuild Vertex Shader class */
class EE_API VertexShader : public Shader {
  public:
	VertexShader();
	VertexShader( const std::string& Filename );
	VertexShader( const char* Data, const Uint32& DataSize );
	VertexShader( Pack* Pack, const std::string& Filename );
	VertexShader( const char** Data, const Uint32& NumLines );
};

/** @brief Prebuild Fragment Shader class */
class EE_API FragmentShader : public Shader {
  public:
	FragmentShader();
	FragmentShader( const std::string& Filename );
	FragmentShader( const char* Data, const Uint32& DataSize );
	FragmentShader( Pack* Pack, const std::string& Filename );
	FragmentShader( const char** Data, const Uint32& NumLines );
};

}} // namespace EE::Graphics

#endif
