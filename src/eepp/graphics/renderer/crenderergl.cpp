#include <eepp/graphics/renderer/crenderergl.hpp>

/*
 *  Some of this code is based on the implementation of the functions from
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

namespace EE { namespace Graphics {

#if !defined( EE_GLES2 ) || defined( EE_GLES_BOTH )

#ifdef EE_GLES1_LATE_INCLUDE
	#if EE_PLATFORM == EE_PLATFORM_IOS
		#include <OpenGLES/ES1/gl.h>
		#include <OpenGLES/ES1/glext.h>
	#else
		#include <GLES/gl.h>

		#ifndef GL_GLEXT_PROTOTYPES
			#define GL_GLEXT_PROTOTYPES
		#endif

		#include <GLES/glext.h>
	#endif
#endif

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
	return "OpenGL ES 1";
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

static void MakeIdentityf( GLfloat* m )
{
	m[0] = 1;	m[4] = 0;	m[8] = 0;	m[12] = 0;
	m[1] = 0;	m[5] = 1;	m[9] = 0;	m[13] = 0;
	m[2] = 0;	m[6] = 0;	m[10] = 1;	m[14] = 0;
	m[3] = 0;	m[7] = 0;	m[11] = 0;	m[15] = 1;
}

void cRendererGL::Perspective ( GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar ) {
	GLfloat m[16];
	float sine, cotangent, deltaZ;
	float radians = fovy / 2 * EE_PI_180;

	deltaZ = zFar - zNear;
	sine = eesin( radians );

	if ( (deltaZ == 0) || (sine == 0) || (aspect == 0) ) {
		return;
	}

	cotangent = eecos(radians) / sine;

	MakeIdentityf( &m[0] );

	m[0]	= cotangent / aspect;
	m[5]	= cotangent;
	m[10]	= -(zFar + zNear) / deltaZ;
	m[11]	= -1;
	m[14]	= -2 * zNear * zFar / deltaZ;
	m[15]	= 0;

	glMultMatrixf( &m[0] );
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


static int __gluInvertMatrixd( const GLfloat m[16], GLfloat invOut[16] ) {
	float inv[16], det;
	int i;

	inv[0] =   m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
			 + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
	inv[4] =  -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
			 - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
	inv[8] =   m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
			 + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
	inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
			 - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
	inv[1] =  -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
			 - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
	inv[5] =   m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
			 + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
	inv[9] =  -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
			 - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
	inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
			 + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
	inv[2] =   m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
			 + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
	inv[6] =  -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
			 - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
	inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
			 + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
	inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
			 - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
	inv[3] =  -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
			 - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
	inv[7] =   m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
			 + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
	inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
			 - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
	inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
			 + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

	det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
	if (det == 0)
		return GL_FALSE;

	det = 1.0 / det;

	for (i = 0; i < 16; i++)
		invOut[i] = inv[i] * det;

	return GL_TRUE;
}

static void __gluMultMatricesd(const GLfloat a[16], const GLfloat b[16], GLfloat r[16] ) {
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			r[i*4+j] =
			a[i*4+0]*b[0*4+j] +
			a[i*4+1]*b[1*4+j] +
			a[i*4+2]*b[2*4+j] +
			a[i*4+3]*b[3*4+j];
		}
	}
}

static void __gluMultMatrixVecd( const GLfloat matrix[16], const GLfloat in[4], GLfloat out[4] ) {
	int i;
	for (i=0; i<4; i++) {
		out[i] =
			in[0] * matrix[0*4+i] +
			in[1] * matrix[1*4+i] +
			in[2] * matrix[2*4+i] +
			in[3] * matrix[3*4+i];
	}
}

GLint cRendererGL::Project( GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *winx, GLfloat *winy, GLfloat *winz ) {
	float in[4];
	float out[4];

	in[0]=objx;
	in[1]=objy;
	in[2]=objz;
	in[3]=1.0;
	__gluMultMatrixVecd(modelMatrix, in, out);
	__gluMultMatrixVecd(projMatrix, out, in);
	if (in[3] == 0.0) return GL_FALSE;
	in[0] /= in[3];
	in[1] /= in[3];
	in[2] /= in[3];
	/* Map x, y and z to range 0-1 */
	in[0] = in[0] * 0.5 + 0.5;
	in[1] = in[1] * 0.5 + 0.5;
	in[2] = in[2] * 0.5 + 0.5;

	/* Map x,y to viewport */
	in[0] = in[0] * viewport[2] + viewport[0];
	in[1] = in[1] * viewport[3] + viewport[1];

	*winx=in[0];
	*winy=in[1];
	*winz=in[2];
	return GL_TRUE;
}

GLint cRendererGL::UnProject( GLfloat winx, GLfloat winy, GLfloat winz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *objx, GLfloat *objy, GLfloat *objz ) {
	float finalMatrix[16];
	float in[4];
	float out[4];

	__gluMultMatricesd(modelMatrix, projMatrix, finalMatrix);
	if (!__gluInvertMatrixd(finalMatrix, finalMatrix)) return(GL_FALSE);

	in[0]=winx;
	in[1]=winy;
	in[2]=winz;
	in[3]=1.0;

	/* Map x and y from window coordinates */
	in[0] = (in[0] - viewport[0]) / viewport[2];
	in[1] = (in[1] - viewport[1]) / viewport[3];

	/* Map to range -1 to 1 */
	in[0] = in[0] * 2 - 1;
	in[1] = in[1] * 2 - 1;
	in[2] = in[2] * 2 - 1;

	__gluMultMatrixVecd(finalMatrix, in, out);
	if (out[3] == 0.0) return GL_FALSE;
	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];
	*objx = out[0];
	*objy = out[1];
	*objz = out[2];
	return GL_TRUE;
}

#endif

}}
