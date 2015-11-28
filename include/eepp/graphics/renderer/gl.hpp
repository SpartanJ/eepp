#ifndef EE_GRAPHICS_CGL_HPP
#define EE_GRAPHICS_CGL_HPP

#include <eepp/graphics/renderer/base.hpp>
#include <eepp/graphics/shaderprogram.hpp>
#include <eepp/graphics/glhelper.hpp>

namespace EE { namespace Graphics {

#ifndef EE_MAX_PLANES
#define EE_MAX_PLANES 6
#endif

class RendererGL;
class RendererGL3;
class RendererGL3CP;
class RendererGLES2;

/** @brief This class is an abstraction of some OpenGL functionality.
*	eepp have 3 different rendering pipelines: OpenGL 2, OpenGL 3 and OpenGL ES 2. This abstraction is to encapsulate this pipelines.
*	eepp implements its own state machine to simulate fixed-pipeline commands with OpenGL 3 and OpenGL ES 2.
*	Most of the commands can be found in the OpenGL documentation.
*	This is only useful for advanced users that want some control of the OpenGL pipeline. It's mostly used internally by the engine.
*/
class EE_API cGL {
	static cGL * ms_singleton;
	public:
		static cGL * CreateSingleton( EEGL_version ver );

		static cGL * CreateSingleton();

		static cGL * ExistsSingleton();

		static cGL * instance();

		static void DestroySingleton();

		cGL();

		virtual ~cGL();

		virtual void Init();

		/** @return The company responsible for this GL implementation. */
		std::string GetVendor();

		/** @return The name of the renderer.\n This name is typically specific to a particular configuration of a hardware platform. */
		std::string GetRenderer();

		/** @return A GL version or release number. */
		std::string GetVersion();

		/** @return The shading language version */
		std::string GetShadingLanguageVersion();

		/** @return If the extension passed is supported by the GPU */
		bool IsExtension( const std::string& name );

		/** @return If the extension from the EEGL_extensions is present on the GPU. */
		bool IsExtension( EEGL_extensions name );

		bool PointSpriteSupported();

		bool ShadersSupported();

		Uint32 GetTextureParamEnum( const EE_TEXTURE_PARAM& Type );

		Uint32 GetTextureFuncEnum( const EE_TEXTURE_FUNC& Type );

		Uint32 GetTextureSourceEnum( const EE_TEXTURE_SOURCE& Type );

		Uint32 GetTextureOpEnum( const EE_TEXTURE_OP& Type );

		void Clear ( unsigned int mask );

		void ClearColor ( float red, float green, float blue, float alpha );

		void Scissor ( int x, int y, int width, int height );

		void PolygonMode( unsigned int face, unsigned int mode );

		std::string GetExtensions();

		const char * GetString( unsigned int name );

		void DrawArrays ( unsigned int mode, int first, int count );

		void DrawElements( unsigned int mode, int count, unsigned int type, const void *indices );

		void BindTexture ( unsigned int target, unsigned int texture );

		void ActiveTexture( unsigned int texture );

		void BlendFunc ( unsigned int sfactor, unsigned int dfactor );

		void Viewport ( int x, int y, int width, int height );

		void LineSmooth( const bool& Enable );

		void LineWidth ( float width );

		/** Reapply the line smooth state */
		void LineSmooth();

		bool IsLineSmooth();

		/** Set the polygon fill mode ( wireframe or filled ) */
		void PolygonMode( const EE_FILL_MODE& Mode );

		/** Reapply the polygon mode */
		void PolygonMode();

		void PixelStorei (unsigned int pname, int param);

		RendererGL * GetRendererGL();

		RendererGL3 * GetRendererGL3();

		RendererGL3CP * GetRendererGL3CP();

		RendererGLES2 * GetRendererGLES2();

		virtual void PointSize( float size ) = 0;

		virtual float PointSize() = 0;

		virtual void ClientActiveTexture( unsigned int texture ) = 0;

		virtual void Disable ( unsigned int cap );

		virtual void Enable( unsigned int cap );

		virtual EEGL_version Version() = 0;

		virtual std::string VersionStr() = 0;

		virtual void PushMatrix() = 0;

		virtual void PopMatrix() = 0;

		virtual void LoadIdentity() = 0;

		virtual void Translatef( float x, float y, float z ) = 0;

		virtual void Rotatef( float angle, float x, float y, float z ) = 0;

		virtual void Scalef( float x, float y, float z ) = 0;

		virtual void MatrixMode ( unsigned int mode ) = 0;

		virtual void Ortho ( float left, float right, float bottom, float top, float zNear, float zFar ) = 0;

		virtual void LookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ ) = 0;

		virtual void Perspective ( float fovy, float aspect, float zNear, float zFar ) = 0;

		virtual void EnableClientState( unsigned int array ) = 0;

		virtual void DisableClientState( unsigned int array ) = 0;

		virtual void VertexPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) = 0;

		virtual void ColorPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) = 0;

		virtual void TexCoordPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) = 0;

		virtual void SetShader( ShaderProgram * Shader );

		virtual void Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) = 0;

		virtual void Clip2DPlaneDisable() = 0;

		virtual void MultMatrixf ( const float *m ) = 0;

		virtual void ClipPlane( unsigned int plane, const double *equation ) = 0;

		virtual void TexEnvi( unsigned int target, unsigned int pname, int param ) = 0;

		virtual void LoadMatrixf( const float *m ) = 0;

		virtual void Frustum( float left, float right, float bottom, float top, float near_val, float far_val ) = 0;

		virtual void GetCurrentMatrix( unsigned int mode, float * m ) = 0;

		virtual unsigned int GetCurrentMatrixMode() = 0;

		void GetViewport( int * viewport );

		virtual int Project( float objx, float objy, float objz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *winx, float *winy, float *winz ) = 0;

		virtual int UnProject( float winx, float winy, float winz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *objx, float *objy, float *objz ) = 0;

		Vector3f ProjectCurrent( const Vector3f& point );

		Vector3f UnProjectCurrent( const Vector3f& point );

		void StencilFunc( unsigned int func, int ref, unsigned int mask );

		void StencilOp( unsigned int fail, unsigned int zfail, unsigned int zpass );

		void StencilMask ( unsigned int mask );

		void ColorMask ( Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha );

		void BindVertexArray ( unsigned int array );

		void DeleteVertexArrays ( int n, const unsigned int *arrays );

		void GenVertexArrays ( int n, unsigned int *arrays );

		const bool& QuadsSupported() const;

		const int& QuadVertexs() const;
	protected:
		enum GLStateFlags {
			GLSF_LINE_SMOOTH	= 0,
			GLSF_POLYGON_MODE
		};

		Uint32	mExtensions;
		Uint32	mStateFlags;
		bool	mPushClip;
		bool	mQuadsSupported;
		bool	mBlendEnabled;
		int		mQuadVertexs;
		float mLineWidth;
		unsigned int	mCurVAO;

		std::list<Rectf> mPlanesClipped;
	private:
		void WriteExtension( Uint8 Pos, Uint32 BitWrite );
};

extern EE_API cGL * GLi;

}}

#endif
