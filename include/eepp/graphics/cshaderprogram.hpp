#ifndef EE_GRAPHICSCSHADERPROGRAM_H
#define EE_GRAPHICSCSHADERPROGRAM_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cshader.hpp>
#include <eepp/math/vector3.hpp>

namespace EE { namespace Graphics {

/** @brief The Shader Program Class.
	@short Program is a GPU-executed program that is ready to be used for manipulating geometry and colors.
    * Program can encapsulate vertex and fragment shaders or just one of them. If only either vertex or fragment shader is used, then traditional fixed-function pipeline is used for the other stage.
*/
class EE_API cShaderProgram {
	public:
		typedef cb::Callback1<void, cShaderProgram*> ShaderProgramReloadCb;

		cShaderProgram( const std::string& name = "" );

		/** Construct a program shader with a vector of shaders and link them. */
		cShaderProgram( const std::vector<cShader*>& Shaders, const std::string& name = "" );

		/** Constructor that creates a VertexShader from file and a Fragment Shader from file, and link them. */
		cShaderProgram( const std::string& VertexShaderFile, const std::string& FragmentShaderFile, const std::string& name = "" );

		/** Constructor that creates a VertexShader from memory and a Fragment Shader from memory, and link them. */
		cShaderProgram( const char * VertexShaderData, const Uint32& VertexShaderDataSize, const char * FragmentShaderData, const Uint32& FragmentShaderDataSize, const std::string& name = "" );

		/** Constructor that creates the vertex shader and fragment shader from two files inside a pack */
		cShaderProgram( cPack * Pack, const std::string& VertexShaderPath, const std::string& FragmentShaderPath, const std::string& name = "" );

		cShaderProgram( const char ** VertexShaderData, const Uint32& NumLinesVS, const char ** FragmentShaderData, const Uint32& NumLinesFS, const std::string& name = "" );

		virtual ~cShaderProgram();

		/** Add a new shader */
		void AddShader( cShader* Shader );

		/** Add a vector of shaders */
		void AddShaders( const std::vector<cShader*>& Shaders );

		virtual bool Link();

		/** @return If the shader program is valid */
		bool IsValid() const { return mValid; }

		/** @return THe link log */
		std::string GetLinkLog() const { return mLinkLog; }

		/** Binds the shader program so that it will be used for anything that is rendered */
		virtual void Bind() const;

		/**  Unbind the program. Anything rendered after Unbind() call will be rendered using the fixed-function pipeline */
		virtual void Unbind() const;

		/** @return The location of the location name */
		Int32 UniformLocation( const std::string& Name );

		/** @return The location of the attribute name */
		Int32 AttributeLocation( const std::string& Name );

		/** Clear the locations */
		void InvalidateLocations();

		/** Sets the uniform with the given name to the given value and returns true.
		* If there is no uniform with such name then false is returned.
		* Note that the program has to be bound before this method can be used.
		*/
		bool SetUniform( const std::string& Name, float Value );

		/** @overload */
		bool SetUniform( const std::string& Name, eeVector2ff Value );

		/** @overload */
		bool SetUniform( const std::string& Name, eeVector3ff Value );

		/** @overload */
		bool SetUniform( const std::string& Name, float x, float y, float z, float w );

		/** @overload */
		bool SetUniform( const std::string& Name, Int32 Value );

		/** @overload */
		bool SetUniform( const Int32& Location, Int32 Value );

		/** @overload */
		bool SetUniform( const Int32& Location, float Value );

		/** @overload */
		bool SetUniform( const Int32& Location, eeVector2ff Value );

		/** @overload */
		bool SetUniform( const Int32& Location, eeVector3ff Value );

		/** @overload */
		bool SetUniform( const Int32& Location, float x, float y, float z, float w );

		bool SetUniformMatrix( const std::string Name, const float * Value );

		bool SetUniformMatrix( const Int32& Location, const float * Value );

		/** @return The id of the program (the handle) */
		const Uint32& Handler() const { return mHandler; }

		/** @return The Id of the program ( hash of the program name ) */
		const Uint32& Id() const { return mId; }

		/** Reloads the shaders */
		void Reload();

		/** @return Name of the shader program */
		const std::string& Name() const;

		/** Set the name of the shader program */
		void Name( const std::string& name );

		/** Set a reload callback ( needed to reset shader states ). */
		void SetReloadCb( ShaderProgramReloadCb Cb );

		/** Enable a vertex attribute array */
		void EnableVertexAttribArray( const std::string& Name );

		/** Enable a vertex attribute array */
		void EnableVertexAttribArray( const Int32& Location );

		/** Disable a vertex attribute array */
		void DisableVertexAttribArray( const std::string& Name );

		/** Disable a vertex attribute array */
		void DisableVertexAttribArray( const Int32& Location );
	protected:
		std::string mName;
		Uint32 mHandler;
		Uint32 mId;

		bool mValid;
		std::string mLinkLog;

		std::vector<cShader*> mShaders;
		std::map<std::string, Int32> mUniformLocations;
		std::map<std::string, Int32> mAttributeLocations;

		ShaderProgramReloadCb mReloadCb;

		void Init();

		void AddToManager( const std::string& name );

		void RemoveFromManager();
};

}}

#endif
