#ifndef EE_GRAPHICS_CGL_HPP
#define EE_GRAPHICS_CGL_HPP

#include <eepp/graphics/renderer/base.hpp>
#include <eepp/graphics/shaderprogram.hpp>
#include <eepp/graphics/primitivetype.hpp>
#include <eepp/graphics/renderer/rendererhelper.hpp>
#include <eepp/graphics/renderer/clippingmask.hpp>

namespace EE { namespace Graphics {

#ifndef EE_MAX_PLANES
#define EE_MAX_PLANES 6
#endif

class RendererGL;
class RendererGL3;
class RendererGL3CP;
class RendererGLES2;

/** @brief This class is an abstraction of some OpenGL functionality.
*	eepp have 4 different rendering pipelines: OpenGL 2, OpenGL 3, OpenGL 3 Core Profile and OpenGL ES 2. This abstraction is to encapsulate this pipelines.
*	eepp implements its own state machine to simulate fixed-pipeline commands with OpenGL 3 and OpenGL ES 2.
*	Most of the commands can be found in the OpenGL documentation.
*	This is only useful for advanced users that want some control of the OpenGL pipeline. It's mostly used internally by the engine.
*/
class EE_API Renderer {
	public:
		static Renderer * createSingleton( EEGL_version ver );

		static Renderer * createSingleton();

		static Renderer * existsSingleton();

		static Renderer * instance();

		static void destroySingleton();

		Renderer();

		virtual ~Renderer();

		virtual void init();

		/** @return The company responsible for this GL implementation. */
		std::string getVendor();

		/** @return The name of the renderer.\n This name is typically specific to a particular configuration of a hardware platform. */
		std::string getRenderer();

		/** @return A GL version or release number. */
		std::string getVersion();

		/** @return The shading language version */
		std::string getShadingLanguageVersion();

		/** @return If the extension passed is supported by the GPU */
		bool isExtension( const std::string& name );

		/** @return If the extension from the EEGL_extensions is present on the GPU. */
		bool isExtension( EEGL_extensions name );

		bool pointSpriteSupported();

		bool shadersSupported();

		void clear ( unsigned int mask );

		void clearColor ( float red, float green, float blue, float alpha );

		void scissor ( int x, int y, int width, int height );

		void polygonMode( unsigned int face, unsigned int mode );

		std::string getExtensions();

		const char * getString( unsigned int name );

		void drawArrays ( unsigned int mode, int first, int count );

		void drawElements( unsigned int mode, int count, unsigned int type, const void *indices );

		void bindTexture( unsigned int target, unsigned int texture );

		void activeTexture( unsigned int texture );

		void blendFunc( unsigned int sfactor, unsigned int dfactor );

		void blendFuncSeparate( unsigned int sfactorRGB, unsigned int dfactorRGB, unsigned int sfactorAlpha, unsigned int dfactorAlpha );

		void blendEquationSeparate( unsigned int modeRGB, unsigned int modeAlpha );

		void blitFrameBuffer( int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, unsigned int mask, unsigned int filter );

		void viewport( int x, int y, int width, int height );

		void lineSmooth( const bool& enable );

		void lineWidth( float width );

		/** Reapply the line smooth state */
		void lineSmooth();

		bool isLineSmooth();

		/** Set the polygon fill mode ( wireframe or filled ) */
		void polygonMode( const PrimitiveFillMode& Mode );

		/** Reapply the polygon mode */
		void polygonMode();

		void pixelStorei (unsigned int pname, int param);

		RendererGL * getRendererGL();

		RendererGL3 * getRendererGL3();

		RendererGL3CP * getRendererGL3CP();

		RendererGLES2 * getRendererGLES2();

		virtual void pointSize( float size ) = 0;

		virtual float pointSize() = 0;

		virtual void clientActiveTexture( unsigned int texture ) = 0;

		virtual void disable( unsigned int cap );

		virtual void enable( unsigned int cap );

		virtual EEGL_version version() = 0;

		virtual std::string versionStr() = 0;

		virtual void pushMatrix() = 0;

		virtual void popMatrix() = 0;

		virtual void loadIdentity() = 0;

		virtual void translatef( float x, float y, float z ) = 0;

		virtual void rotatef( float angle, float x, float y, float z ) = 0;

		virtual void scalef( float x, float y, float z ) = 0;

		virtual void matrixMode ( unsigned int mode ) = 0;

		virtual void ortho( float left, float right, float bottom, float top, float zNear, float zFar ) = 0;

		virtual void lookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ ) = 0;

		virtual void perspective( float fovy, float aspect, float zNear, float zFar ) = 0;

		virtual void enableClientState( unsigned int array ) = 0;

		virtual void disableClientState( unsigned int array ) = 0;

		virtual void vertexPointer( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) = 0;

		virtual void colorPointer( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) = 0;

		virtual void texCoordPointer( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) = 0;

		virtual void setShader( ShaderProgram * Shader );

		virtual void clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) = 0;

		virtual void clip2DPlaneDisable() = 0;

		virtual void multMatrixf( const float *m ) = 0;

		virtual void clipPlane( unsigned int plane, const double *equation ) = 0;

		virtual void loadMatrixf( const float *m ) = 0;

		virtual void frustum( float left, float right, float bottom, float top, float near_val, float far_val ) = 0;

		virtual void getCurrentMatrix( unsigned int mode, float * m ) = 0;

		virtual unsigned int getCurrentMatrixMode() = 0;

		void getViewport( int * viewport );

		virtual int project( float objx, float objy, float objz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *winx, float *winy, float *winz ) = 0;

		virtual int unProject( float winx, float winy, float winz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *objx, float *objy, float *objz ) = 0;

		Vector3f projectCurrent( const Vector3f& point );

		Vector3f unProjectCurrent( const Vector3f& point );

		void stencilFunc( unsigned int func, int ref, unsigned int mask );

		void stencilOp( unsigned int fail, unsigned int zfail, unsigned int zpass );

		void stencilMask( unsigned int mask );

		void colorMask( Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha );

		void bindVertexArray( unsigned int array );

		void deleteVertexArrays( int n, const unsigned int *arrays );

		void genVertexArrays( int n, unsigned int *arrays );

		const bool& quadsSupported() const;

		const int& quadVertexs() const;

		ClippingMask * getClippingMask() const;

		void genFramebuffers( int n, unsigned int* framebuffers );

		void deleteFramebuffers( int n, const unsigned int* framebuffers );

		void bindFramebuffer( unsigned int target, unsigned int framebuffer );

		void framebufferTexture2D( unsigned int target, unsigned int attachment, unsigned int textarget, unsigned int texture, int level );

		void genRenderbuffers( int n, unsigned int * renderbuffers );

		void deleteRenderbuffers( int n, const unsigned int* renderbuffers );

		void bindRenderbuffer( unsigned int target, unsigned int renderbuffer);

		void renderbufferStorage( unsigned int target, unsigned int internalformat, int width, int height );

		void framebufferRenderbuffer( unsigned int target, unsigned int attachment, unsigned int renderbuffertarget, unsigned int renderbuffer );

		unsigned int checkFramebufferStatus( unsigned int target );

		void discardFramebuffer( unsigned int target, int numAttachments, const unsigned int* attachments );

		void * getProcAddress( std::string proc );
	protected:
		static Renderer * sSingleton;

		enum RendererStateFlags {
			RSF_LINE_SMOOTH	= 0,
			RSF_POLYGON_MODE
		};

		Uint32	mExtensions;
		Uint32	mStateFlags;
		bool	mQuadsSupported;
		int		mQuadVertexs;
		float mLineWidth;
		unsigned int	mCurVAO;

		ClippingMask * mClippingMask;
	private:
		void writeExtension( Uint8 Pos, Uint32 BitWrite );
};

extern EE_API Renderer * GLi;

}}

#endif
