#ifndef EE_GRAPHICSCSHADERPROGRAM_H
#define EE_GRAPHICSCSHADERPROGRAM_H

#include "base.hpp"
#include "cshader.hpp"

namespace EE { namespace Graphics {

/** @brief The Shader Program Class.
	@short Program is a GPU-executed program that is ready to be used for manipulating geometry and colors.
    * Program can encapsulate vertex and fragment shaders or just one of them. If only either vertex or fragment shader is used, then traditional fixed-function pipeline is used for the other stage.
*/
class EE_API cShaderProgram {
	public:
		cShaderProgram( const std::string& name = "" );

		/** Construct a program shader with a vector of shaders and link them. */
		cShaderProgram( const std::vector<cShader*>& Shaders, const std::string& name = "" );

		/** Constructor that creates a VertexShader from file and a Fragment Shader from file, and Link them. */
		cShaderProgram( const std::string& VertexShaderFile, const std::string& FragmentShaderFile, const std::string& name = "" );

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
		bool SetUniform( const std::string& Name, eeFloat Value );

		/** @overload */
		bool SetUniform( const std::string& Name, eeVector2f Value );

		/** @overload */
		bool SetUniform( const std::string& Name, eeVector3f Value );

		/** @overload */
		bool SetUniform( const std::string& Name, Int32 Value );

		/** @return The id of the program (the handle) */
		Uint32 GetId() const { return mGLId; }

		/** Reloads the shaders */
		void Reload();

		/** @return Name of the shader program */
		const std::string& Name() const;

		/** Set the name of the shader program */
		void Name( const std::string& name );
    protected:
		std::string mName;
		Uint32 mGLId;
		bool mValid;
		std::string mLinkLog;

		std::vector<cShader*> mShaders;
		std::map<std::string, Int32> mUniformLocations;
		std::map<std::string, Int32> mAttributeLocations;

		void Init();

		void AddToManager( const std::string& name );

		void RemoveFromManager();
};

}}

#endif
