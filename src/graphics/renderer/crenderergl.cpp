#include "crenderergl.hpp"

namespace EE { namespace Graphics {

#ifndef EE_GLES2

cRendererGL::cRendererGL() {
}

cRendererGL::~cRendererGL() {
}

EEGL_version cRendererGL::Version() {
	#ifndef EE_GLES1
	return GLv_2;
	#else
	return GLv_ES1;
	#endif
}

std::string cRendererGL::VersionStr() {
	#ifndef EE_GLES1
	return "OpenGL 2";
	#else
	return "OpenGL ES1";
	#endif
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

/*
 *  This is a modified version of the function of the same name from
 *  the Mesa3D project ( http://mesa3d.org/ ), which is  licensed
 *  under the MIT license, which allows use, modification, and
 *  redistribution
 *
 *  Out of respect for the original authors, this is licensed under
 *  the Mesa (MIT) license. Original license follows:
 *
 *  -----------------------------------------------------------------------
 *
 *  Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included
 *  in all copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 *  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
void cRendererGL::LookAt( GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ ) {
	GLfloat m[16];
	GLfloat x[3], y[3], z[3];
	GLfloat mag;

	/* Make rotation matrix */

	/* Z vector */
	z[0] = eyeX - centerX;
	z[1] = eyeY - centerY;
	z[2] = eyeZ - centerZ;
	mag = eesqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
	if (mag) {          /* mpichler, 19950515 */
		z[0] /= mag;
		z[1] /= mag;
		z[2] /= mag;
	}

	/* Y vector */
	y[0] = upX;
	y[1] = upY;
	y[2] = upZ;

	/* X vector = Y cross Z */
	x[0] = y[1] * z[2] - y[2] * z[1];
	x[1] = -y[0] * z[2] + y[2] * z[0];
	x[2] = y[0] * z[1] - y[1] * z[0];

	/* Recompute Y = Z cross X */
	y[0] = z[1] * x[2] - z[2] * x[1];
	y[1] = -z[0] * x[2] + z[2] * x[0];
	y[2] = z[0] * x[1] - z[1] * x[0];

	/* mpichler, 19950515 */
	/* cross product gives area of parallelogram, which is < 1.0 for
	 * non-perpendicular unit-length vectors; so normalize x, y here
	 */

	mag = eesqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
	if (mag) {
		x[0] /= mag;
		x[1] /= mag;
		x[2] /= mag;
	}

	mag = eesqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
	if (mag) {
		y[0] /= mag;
		y[1] /= mag;
		y[2] /= mag;
	}

	#define M(row,col)  m[col*4+row]
	M(0, 0) = x[0];
	M(0, 1) = x[1];
	M(0, 2) = x[2];
	M(0, 3) = 0.0;
	M(1, 0) = y[0];
	M(1, 1) = y[1];
	M(1, 2) = y[2];
	M(1, 3) = 0.0;
	M(2, 0) = z[0];
	M(2, 1) = z[1];
	M(2, 2) = z[2];
	M(2, 3) = 0.0;
	M(3, 0) = 0.0;
	M(3, 1) = 0.0;
	M(3, 2) = 0.0;
	M(3, 3) = 1.0;
	#undef M

	glMultMatrixf(m);

	/* Translate Eye to Origin */
	glTranslatef(-eyeX, -eyeY, -eyeZ);
}

static void MakeIdentityf( GLfloat m[16] )
{
	m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
	m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
	m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
	m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}

void cRendererGL::Perspective ( GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar ) {
	GLfloat m[4][4];
	float sine, cotangent, deltaZ;
	float radians = fovy / 2 * EE_PI_180;

	deltaZ = zFar - zNear;
	sine = eesin( radians );

	if ( (deltaZ == 0) || (sine == 0) || (aspect == 0) ) {
		return;
	}

	cotangent = eecos(radians) / sine;

	MakeIdentityf( &m[0][0] );

	m[0][0] = cotangent / aspect;
	m[1][1] = cotangent;
	m[2][2] = -(zFar + zNear) / deltaZ;
	m[2][3] = -1;
	m[3][2] = -2 * zNear * zFar / deltaZ;
	m[3][3] = 0;

	glMultMatrixf( &m[0][0] );
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

void cRendererGL::LoadMatrixf( const GLfloat *m ) {
	glLoadMatrixf( m );
}

void cRendererGL::TexEnvi( GLenum target, GLenum pname, GLint param ) {
	glTexEnvi( target, pname, param );
}

GLfloat cRendererGL::PointSize() {
	float ps = 1;

	glGetFloatv( GL_POINT_SIZE, &ps );

	return ps;
}

void cRendererGL::Frustum( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val ) {
	glFrustum( left, right, bottom, top, near_val, far_val );
}

void cRendererGL::GetCurrentMatrix( GLenum mode, GLfloat * m ) {
	glGetFloatv( mode, m );
}

GLenum cRendererGL::GetCurrentMatrixMode() {
	GLint mode;

	glGetIntegerv( GL_MATRIX_MODE, &mode );

	return (GLenum)mode;
}

#endif

}}
