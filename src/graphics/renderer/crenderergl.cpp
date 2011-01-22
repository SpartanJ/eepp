#include "crenderergl.hpp"

namespace EE { namespace Graphics {

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

}}
