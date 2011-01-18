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

void cRendererGL::Ortho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
	glOrtho( left, right, bottom, top, zNear, zFar );
}

}} 
