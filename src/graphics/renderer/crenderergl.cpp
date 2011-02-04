#include "crenderergl.hpp"

namespace EE { namespace Graphics {

#ifndef EE_GLES2

cRendererGL::cRendererGL() {
}

cRendererGL::~cRendererGL() {
}

EEGL_version cRendererGL::Version() {
	return GLv_2;
}

void cRendererGL::PushMatrix() {
	glPushMatrix();
}

void cRendererGL::PopMatrix() {
	glPopMatrix();
}

void cRendererGL::LoadIdentity() {
	glLoadIdentity();
}

void cRendererGL::Translatef( GLfloat x, GLfloat y, GLfloat z ) {
	glTranslatef( x, y, z );
}

void cRendererGL::Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
	glRotatef( angle, x, y, z );
}

void cRendererGL::Scalef( GLfloat x, GLfloat y, GLfloat z ) {
	glScalef( x,y, z );
}

void cRendererGL::MatrixMode(GLenum mode) {
	glMatrixMode( mode );
}

void cRendererGL::Ortho( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar ) {
	glOrtho( left, right, bottom, top, zNear, zFar );
}

void cRendererGL::LookAt( GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ ) {
	gluLookAt( eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ );
}

void cRendererGL::Perspective ( GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar ) {
	gluPerspective( fovy, aspect, zNear, zFar );
}

void cRendererGL::EnableClientState( GLenum array ) {
	glEnableClientState( array );
}

void cRendererGL::DisableClientState( GLenum array ) {
	glDisableClientState( array );
}

void cRendererGL::VertexPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	glVertexPointer( size, type, stride, pointer );
}

void cRendererGL::ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	glColorPointer( size, type, stride, pointer );
}

void cRendererGL::TexCoordPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	glTexCoordPointer( size, type, stride, pointer );
}

void cRendererGL::ClientActiveTexture( GLenum texture ) {
	glClientActiveTexture( texture );
}

void cRendererGL::PointSize( GLfloat size ) {
	glPointSize( size );
}

void cRendererGL::Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	GLdouble tX = (GLdouble)x;
	GLdouble tY = (GLdouble)y;
	GLdouble tW = (GLdouble)Width;
	GLdouble tH = (GLdouble)Height;

	GLdouble clip_left[] 	= { 1.0	, 0.0	, 0.0, -tX 		};
	GLdouble clip_right[] 	= { -1.0, 0.0	, 0.0, tX + tW 	};
	GLdouble clip_top[] 	= { 0.0	, 1.0	, 0.0, -tY 		};
	GLdouble clip_bottom[] 	= { 0.0	, -1.0	, 0.0, tY + tH 	};

	GLi->Enable(GL_CLIP_PLANE0);
	GLi->Enable(GL_CLIP_PLANE1);
	GLi->Enable(GL_CLIP_PLANE2);
	GLi->Enable(GL_CLIP_PLANE3);

	ClipPlane(GL_CLIP_PLANE0, clip_left);
	ClipPlane(GL_CLIP_PLANE1, clip_right);
	ClipPlane(GL_CLIP_PLANE2, clip_top);
	ClipPlane(GL_CLIP_PLANE3, clip_bottom);
}

void cRendererGL::Clip2DPlaneDisable() {
	GLi->Disable(GL_CLIP_PLANE0);
	GLi->Disable(GL_CLIP_PLANE1);
	GLi->Disable(GL_CLIP_PLANE2);
	GLi->Disable(GL_CLIP_PLANE3);
}

void cRendererGL::ClipPlane( GLenum plane, const GLdouble *equation ) {
	glClipPlane( plane, equation );
}

void cRendererGL::MultMatrixf ( const GLfloat *m ) {
	glMultMatrixf( m );
}

void cRendererGL::TexEnvi( GLenum target, GLenum pname, GLint param ) {
	glTexEnvi( target, pname, param );
}

GLfloat cRendererGL::PointSize() {
	float ps = 1;

	glGetFloatv( GL_POINT_SIZE, &ps );

	return ps;
}

#endif

}}
