#ifndef EE_GRAPHICSCSHADER_H
#define EE_GRAPHICSCSHADER_H

#include <eepp/graphics/base.hpp>

namespace EE { namespace Graphics {

/** @brief The basic shader class. */
class EE_API Shader {
	public:
		/** Activates/Deactivates shader convertion from fixed pipeline to programmable pipeline ( activated by default ) */
		static void Ensure( bool ensure );

		/** @return If automatic Shader conversion is activated */
		static bool Ensure();

		/** Constructor with type of shader, next you'll need to set the source and compile it. */
		Shader( const Uint32& Type );

		/** Create a type of shader and load the shader from a file, and compile it. */
		Shader( const Uint32& Type, const std::string& Filename );

		/** Create a type of shader from memory, and compile it. */
		Shader( const Uint32& Type, const char * Data, const Uint32& DataSize );

		/** Create a type of shader loaded from a pack file */
		Shader( const Uint32& Type, Pack * Pack, const std::string& Filename );

		/** Create a type of shader from memory, and compile it. */
		Shader( const Uint32& Type, const char ** Data, const Uint32& NumLines );

		virtual ~Shader();

		/** Set the shader source */
		void SetSource( const std::string& Source );

		/** Set the shader source */
		void SetSource( const std::vector<Uint8>& Source );

		/** Set the shader source */
		void SetSource( const char * Data, const Uint32& DataSize );

		/** Set the shader source */
		void SetSource( const char** Data, const Uint32& NumLines );

		/** Compile the shader */
		bool Compile();

		/** @return If the shader is valid */
		bool IsValid() const;

		/** @return If the shader is compiled */
		bool IsCompiled() const;

		/** @return The log of the compilation */
		std::string CompileLog() const;

		/** @return The Shader Type */
		Uint32 GetType() const;

		/** @return The Shader Id */
		Uint32 GetId() const;

		/** Reloads the Shader. */
		void Reload();
	protected:
		friend class RendererGL3;
		static bool			sEnsure;
		Uint32 				mGLId;
		Uint32 				mType;
		std::string			mFilename;
		std::string 		mCompileLog;
		std::string         mSource;
		bool 				mValid;
		bool 				mCompiled;

		void Init( const Uint32& Type );

		std::string GetName();

		void EnsureVersion();
};

/** @brief Prebuild Vertex Shader class */
class EE_API cVertexShader : public Shader {
	public:
		cVertexShader();
		cVertexShader( const std::string& Filename );
		cVertexShader( const char * Data, const Uint32& DataSize );
		cVertexShader( Pack * Pack, const std::string& Filename );
		cVertexShader( const char ** Data, const Uint32& NumLines );
};

/** @brief Prebuild Fragment Shader class */
class EE_API cFragmentShader : public Shader {
	public:
		cFragmentShader();
		cFragmentShader( const std::string& Filename );
		cFragmentShader( const char * Data, const Uint32& DataSize );
		cFragmentShader( Pack * Pack, const std::string& Filename );
		cFragmentShader( const char ** Data, const Uint32& NumLines );
};

}}

#endif



