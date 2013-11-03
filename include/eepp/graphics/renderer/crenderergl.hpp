#ifndef EE_GRAPHICS_CRENDERERGL_HPP
#define EE_GRAPHICS_CRENDERERGL_HPP

#include <eepp/graphics/renderer/cgl.hpp>

namespace EE { namespace Graphics {

//! Avoid compilling the fixed pipeline renderer for GLES2, because it's not supported.
#if !defined( EE_GLES2 ) || defined( EE_GLES_BOTH )

class EE_API cRendererGL : public cGL {
	public:
		cRendererGL();
		
		~cRendererGL();

		EEGL_version Version();

		std::string VersionStr();

		void ClientActiveTexture( GLenum texture );

		void PointSize( GLfloat size );

		GLfloat PointSize();

		void PushMatrix();

		void PopMatrix();

		void LoadIdentity();

		void Translatef( GLfloat x, GLfloat y, GLfloat z );

		void Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );

		void Scalef( GLfloat x, GLfloat y, GLfloat z );

		void MatrixMode (GLenum mode);

		void Ortho ( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar );

		void LookAt( GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ );

		void Perspective ( GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar );

		void EnableClientState( GLenum array );

		void DisableClientState( GLenum array );

		void VertexPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate );

		void ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate );

		void TexCoordPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate );

		void ClipPlane( GLenum plane, const GLdouble *equation );

		void Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height );

		void Clip2DPlaneDisable();

		void TexEnvi( GLenum target, GLenum pname, GLint param );

		void MultMatrixf ( const GLfloat *m );

		void LoadMatrixf( const GLfloat *m );

		void Frustum( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val );

		void GetCurrentMatrix( GLenum mode, GLfloat * m );

		GLenum GetCurrentMatrixMode();

		GLint Project( GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *winx, GLfloat *winy, GLfloat *winz );

		GLint UnProject( GLfloat winx, GLfloat winy, GLfloat winz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *objx, GLfloat *objy, GLfloat *objz );
	protected:	
		
};

#endif

}}

#endif
