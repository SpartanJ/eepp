#ifndef EE_GRAPHICS_CRENDERERGL_HPP
#define EE_GRAPHICS_CRENDERERGL_HPP

#include "cgl.hpp"

namespace EE { namespace Graphics {

//! Avoid compilling the fixed pipeline renderer for GLES2, because it's not supported.
#ifndef EE_GLES2

class cRendererGL : public cGL {
	public:
		cRendererGL();
		
		~cRendererGL();

		EEGL_version Version();

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
	protected:	
		
};

#endif

}}

#endif
