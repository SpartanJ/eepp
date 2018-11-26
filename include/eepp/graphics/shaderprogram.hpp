#ifndef EE_GRAPHICSCSHADERPROGRAM_H
#define EE_GRAPHICSCSHADERPROGRAM_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/shader.hpp>

namespace EE { namespace Graphics {

/** @brief The Shader Program Class.
	@short Program is a GPU-executed program that is ready to be used for manipulating geometry and colors.
*	ShaderProgram can encapsulate vertex and fragment shaders or just one of them. If only either vertex or fragment shader is used, then traditional fixed-function pipeline is used for the other stage.
*/
class EE_API ShaderProgram {
	public:
		/** Creates an empty shader program */
		static ShaderProgram * New( const std::string& Name = "" );

		/** Creates a program shader with a vector of shaders and link them. */
		static ShaderProgram * New( const std::vector<Shader*>& Shaders, const std::string& Name = "" );

		/** Creates a VertexShader from file and a Fragment Shader from file, and link them. */
		static ShaderProgram * New( const std::string& VertexShaderFile, const std::string& FragmentShaderFile, const std::string& Name = "" );

		/** Creates a VertexShader from memory and a Fragment Shader from memory, and link them. */
		static ShaderProgram * New( const char * VertexShaderData, const Uint32& VertexShaderDataSize, const char * FragmentShaderData, const Uint32& FragmentShaderDataSize, const std::string& Name = "" );

		/** Creates the vertex shader and fragment shader from two files inside a pack */
		static ShaderProgram * New( Pack * Pack, const std::string& VertexShaderPath, const std::string& FragmentShaderPath, const std::string& Name = "" );

		/** Creates the vertex and fragment shader from an array of strings */
		static ShaderProgram * New( const char ** VertexShaderData, const Uint32& NumLinesVS, const char ** FragmentShaderData, const Uint32& NumLinesFS, const std::string& Name = "" );

		typedef std::function<void( ShaderProgram* )> ShaderProgramReloadCb;

		virtual ~ShaderProgram();

		/** Add a new shader */
		void addShader( Shader* Shader );

		/** Add a vector of shaders */
		void addShaders( const std::vector<Shader*>& Shaders );

		virtual bool link();

		/** @return If the shader program is valid */
		bool isValid() const { return mValid; }

		/** @return THe link log */
		std::string getLinkLog() const { return mLinkLog; }

		/** Binds the shader program so that it will be used for anything that is rendered */
		virtual void bind() const;

		/**  Unbind the program. Anything rendered after Unbind() call will be rendered using the fixed-function pipeline */
		virtual void unbind() const;

		/** @return The location of the location name */
		Int32 getUniformLocation( const std::string& Name );

		/** @return The location of the attribute name */
		Int32 getAttributeLocation( const std::string& Name );

		/** Clear the locations */
		void invalidateLocations();

		/** Sets the uniform with the given name to the given value and returns true.
		* If there is no uniform with such name then false is returned.
		* Note that the program has to be bound before this method can be used.
		*/
		bool setUniform( const std::string& Name, float Value );

		/** @overload */
		bool setUniform( const std::string& Name, Vector2ff Value );

		/** @overload */
		bool setUniform( const std::string& Name, Vector3ff Value );

		/** @overload */
		bool setUniform( const std::string& Name, float x, float y, float z, float w );

		/** @overload */
		bool setUniform( const std::string& Name, Int32 Value );

		/** @overload */
		bool setUniform( const Int32& Location, Int32 Value );

		/** @overload */
		bool setUniform( const Int32& Location, float Value );

		/** @overload */
		bool setUniform( const Int32& Location, Vector2ff Value );

		/** @overload */
		bool setUniform( const Int32& Location, Vector3ff Value );

		/** @overload */
		bool setUniform( const Int32& Location, float x, float y, float z, float w );

		/** Sets an uniform matrix from its name. */
		bool setUniformMatrix( const std::string Name, const float * Value );

		/** Sets an uniform matrix from its location. */
		bool setUniformMatrix( const Int32& Location, const float * Value );

		/** @return The id of the program (the handle) */
		const Uint32& getHandler() const { return mHandler; }

		/** @return The Id of the program ( hash of the program name ) */
		const Uint32& getId() const { return mId; }

		/** Reloads the shaders */
		void reload();

		/** @return Name of the shader program */
		const std::string& getName() const;

		/** Set the name of the shader program */
		void setName( const std::string& Name );

		/** Set a reload callback ( needed to reset shader states ). */
		void setReloadCb( ShaderProgramReloadCb Cb );

		/** Enable a vertex attribute array */
		void enableVertexAttribArray( const std::string& Name );

		/** Enable a vertex attribute array */
		void enableVertexAttribArray( const Int32& Location );

		/** Disable a vertex attribute array */
		void disableVertexAttribArray( const std::string& Name );

		/** Disable a vertex attribute array */
		void disableVertexAttribArray( const Int32& Location );
	protected:
		std::string mName;
		Uint32 mHandler;
		Uint32 mId;

		bool mValid;
		std::string mLinkLog;

		std::vector<Shader*> mShaders;
		std::map<std::string, Int32> mUniformLocations;
		std::map<std::string, Int32> mAttributeLocations;

		ShaderProgramReloadCb mReloadCb;

		void init();

		void addToManager( const std::string& Name );

		void removeFromManager();

		/** Creates an empty shader program */
		ShaderProgram( const std::string& Name = "" );

		/** Construct a program shader with a vector of shaders and link them. */
		ShaderProgram( const std::vector<Shader*>& Shaders, const std::string& Name = "" );

		/** Constructor that creates a VertexShader from file and a Fragment Shader from file, and link them. */
		ShaderProgram( const std::string& VertexShaderFile, const std::string& FragmentShaderFile, const std::string& Name = "" );

		/** Constructor that creates a VertexShader from memory and a Fragment Shader from memory, and link them. */
		ShaderProgram( const char * VertexShaderData, const Uint32& VertexShaderDataSize, const char * FragmentShaderData, const Uint32& FragmentShaderDataSize, const std::string& Name = "" );

		/** Constructor that creates the vertex shader and fragment shader from two files inside a pack */
		ShaderProgram( Pack * Pack, const std::string& VertexShaderPath, const std::string& FragmentShaderPath, const std::string& Name = "" );

		/** Constructor that creates the vertex and fragment shader from an array of strings */
		ShaderProgram( const char ** VertexShaderData, const Uint32& NumLinesVS, const char ** FragmentShaderData, const Uint32& NumLinesFS, const std::string& Name = "" );
};

}}

#endif
