#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderergl.hpp>

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

RendererGL::RendererGL() {
#ifdef EE_GLES1
	mQuadsSupported = false;
	mQuadVertexs = 6;
#endif
}

RendererGL::~RendererGL() {}

GraphicsLibraryVersion RendererGL::version() {
#ifndef EE_GLES1
	return GLv_2;
#else
	return GLv_ES1;
#endif
}

std::string RendererGL::versionStr() {
#ifndef EE_GLES1
	return "OpenGL 2";
#else
	return "OpenGL ES 1";
#endif
}

void RendererGL::pushMatrix() {
	glPushMatrix();
}

void RendererGL::popMatrix() {
	glPopMatrix();
}

void RendererGL::loadIdentity() {
	glLoadIdentity();
}

void RendererGL::translatef( float x, float y, float z ) {
	glTranslatef( x, y, z );
}

void RendererGL::rotatef( float angle, float x, float y, float z ) {
	glRotatef( angle, x, y, z );
}

void RendererGL::scalef( float x, float y, float z ) {
	glScalef( x, y, z );
}

void RendererGL::matrixMode( unsigned int mode ) {
	glMatrixMode( mode );
}

void RendererGL::ortho( float left, float right, float bottom, float top, float zNear,
						float zFar ) {
	glOrtho( left, right, bottom, top, zNear, zFar );
}

void RendererGL::lookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY,
						 float centerZ, float upX, float upY, float upZ ) {
	float m[16];
	float x[3], y[3], z[3];
	float mag;

	/* Make rotation matrix */

	/* Z vector */
	z[0] = eyeX - centerX;
	z[1] = eyeY - centerY;
	z[2] = eyeZ - centerZ;
	mag = eesqrt( z[0] * z[0] + z[1] * z[1] + z[2] * z[2] );
	if ( mag ) { /* mpichler, 19950515 */
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

	mag = eesqrt( x[0] * x[0] + x[1] * x[1] + x[2] * x[2] );
	if ( mag ) {
		x[0] /= mag;
		x[1] /= mag;
		x[2] /= mag;
	}

	mag = eesqrt( y[0] * y[0] + y[1] * y[1] + y[2] * y[2] );
	if ( mag ) {
		y[0] /= mag;
		y[1] /= mag;
		y[2] /= mag;
	}

#define M( row, col ) m[col * 4 + row]
	M( 0, 0 ) = x[0];
	M( 0, 1 ) = x[1];
	M( 0, 2 ) = x[2];
	M( 0, 3 ) = 0.0;
	M( 1, 0 ) = y[0];
	M( 1, 1 ) = y[1];
	M( 1, 2 ) = y[2];
	M( 1, 3 ) = 0.0;
	M( 2, 0 ) = z[0];
	M( 2, 1 ) = z[1];
	M( 2, 2 ) = z[2];
	M( 2, 3 ) = 0.0;
	M( 3, 0 ) = 0.0;
	M( 3, 1 ) = 0.0;
	M( 3, 2 ) = 0.0;
	M( 3, 3 ) = 1.0;
#undef M

	glMultMatrixf( m );

	/* Translate Eye to Origin */
	glTranslatef( -eyeX, -eyeY, -eyeZ );
}

void RendererGL::perspective( float fovy, float aspect, float zNear, float zFar ) {
	double xmin, xmax, ymin, ymax;

	ymax = zNear * eetan( fovy * EE_360_PI );
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	frustum( xmin, xmax, ymin, ymax, zNear, zFar );
}

void RendererGL::enableClientState( unsigned int array ) {
	glEnableClientState( array );
}

void RendererGL::disableClientState( unsigned int array ) {
	glDisableClientState( array );
}

void RendererGL::vertexPointer( int size, unsigned int type, int stride, const void* pointer,
								unsigned int allocate ) {
	glVertexPointer( size, type, stride, pointer );
}

void RendererGL::colorPointer( int size, unsigned int type, int stride, const void* pointer,
							   unsigned int allocate ) {
	glColorPointer( size, type, stride, pointer );
}

void RendererGL::texCoordPointer( int size, unsigned int type, int stride, const void* pointer,
								  unsigned int allocate ) {
	glTexCoordPointer( size, type, stride, pointer );
}

void RendererGL::clientActiveTexture( unsigned int texture ) {
	glClientActiveTexture( texture );
}

void RendererGL::pointSize( float size ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	glPointSize( size );
#endif
}

void RendererGL::clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width,
									const Int32& Height ) {
	Rectf r( x, y, x + Width, y + Height );

	double clip_left[] = {1.0, 0.0, 0.0, -r.Left};
	double clip_right[] = {-1.0, 0.0, 0.0, r.Right};
	double clip_top[] = {0.0, 1.0, 0.0, -r.Top};
	double clip_bottom[] = {0.0, -1.0, 0.0, r.Bottom};

	GLi->enable( GL_CLIP_PLANE0 );
	GLi->enable( GL_CLIP_PLANE1 );
	GLi->enable( GL_CLIP_PLANE2 );
	GLi->enable( GL_CLIP_PLANE3 );

	clipPlane( GL_CLIP_PLANE0, clip_left );
	clipPlane( GL_CLIP_PLANE1, clip_right );
	clipPlane( GL_CLIP_PLANE2, clip_top );
	clipPlane( GL_CLIP_PLANE3, clip_bottom );
}

void RendererGL::clip2DPlaneDisable() {
	GLi->disable( GL_CLIP_PLANE0 );
	GLi->disable( GL_CLIP_PLANE1 );
	GLi->disable( GL_CLIP_PLANE2 );
	GLi->disable( GL_CLIP_PLANE3 );
}

void RendererGL::clipPlane( unsigned int plane, const double* equation ) {
#ifdef EE_GLES1
	float clip[] = {(float)equation[0], (float)equation[1], (float)equation[2], (float)equation[3]};

	glClipPlane( plane, clip );
#else
	glClipPlane( plane, equation );
#endif
}

void RendererGL::multMatrixf( const float* m ) {
	glMultMatrixf( m );
}

void RendererGL::loadMatrixf( const float* m ) {
	glLoadMatrixf( m );
}

float RendererGL::pointSize() {
	float ps = 1;

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	glGetFloatv( GL_POINT_SIZE, &ps );
#endif

	return ps;
}

void RendererGL::frustum( float left, float right, float bottom, float top, float near_val,
						  float far_val ) {
	glFrustum( left, right, bottom, top, near_val, far_val );
}

void RendererGL::getCurrentMatrix( unsigned int mode, float* m ) {
	glGetFloatv( mode, m );
}

unsigned int RendererGL::getCurrentMatrixMode() {
	int mode;

	glGetIntegerv( GL_MATRIX_MODE, &mode );

	return (unsigned int)mode;
}

static int __gluInvertMatrixd( const float m[16], float invOut[16] ) {
	float inv[16], det;
	int i;

	inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
			 m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
	inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
			 m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
	inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
			 m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
	inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
			  m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
	inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] -
			 m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
	inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] +
			 m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
	inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] -
			 m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
	inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] +
			  m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
	inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
			 m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
	inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] -
			 m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
	inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] +
			  m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
	inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] -
			  m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
	inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
			 m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
	inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
			 m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
	inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
			  m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
	inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] +
			  m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	if ( det == 0 )
		return GL_FALSE;

	det = 1.0 / det;

	for ( i = 0; i < 16; i++ )
		invOut[i] = inv[i] * det;

	return GL_TRUE;
}

static void __gluMultMatricesd( const float a[16], const float b[16], float r[16] ) {
	int i, j;
	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			r[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] + a[i * 4 + 1] * b[1 * 4 + j] +
						   a[i * 4 + 2] * b[2 * 4 + j] + a[i * 4 + 3] * b[3 * 4 + j];
		}
	}
}

static void __gluMultMatrixVecd( const float matrix[16], const float in[4], float out[4] ) {
	int i;
	for ( i = 0; i < 4; i++ ) {
		out[i] = in[0] * matrix[0 * 4 + i] + in[1] * matrix[1 * 4 + i] + in[2] * matrix[2 * 4 + i] +
				 in[3] * matrix[3 * 4 + i];
	}
}

int RendererGL::project( float objx, float objy, float objz, const float modelMatrix[16],
						 const float projMatrix[16], const int viewport[4], float* winx,
						 float* winy, float* winz ) {
	float in[4];
	float out[4];

	in[0] = objx;
	in[1] = objy;
	in[2] = objz;
	in[3] = 1.0;
	__gluMultMatrixVecd( modelMatrix, in, out );
	__gluMultMatrixVecd( projMatrix, out, in );
	if ( in[3] == 0.0 )
		return GL_FALSE;
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

	*winx = in[0];
	*winy = in[1];
	*winz = in[2];
	return GL_TRUE;
}

int RendererGL::unProject( float winx, float winy, float winz, const float modelMatrix[16],
						   const float projMatrix[16], const int viewport[4], float* objx,
						   float* objy, float* objz ) {
	float finalMatrix[16];
	float in[4];
	float out[4];

	__gluMultMatricesd( modelMatrix, projMatrix, finalMatrix );
	if ( !__gluInvertMatrixd( finalMatrix, finalMatrix ) )
		return ( GL_FALSE );

	in[0] = winx;
	in[1] = winy;
	in[2] = winz;
	in[3] = 1.0;

	/* Map x and y from window coordinates */
	in[0] = ( in[0] - viewport[0] ) / viewport[2];
	in[1] = ( in[1] - viewport[1] ) / viewport[3];

	/* Map to range -1 to 1 */
	in[0] = in[0] * 2 - 1;
	in[1] = in[1] * 2 - 1;
	in[2] = in[2] * 2 - 1;

	__gluMultMatrixVecd( finalMatrix, in, out );
	if ( out[3] == 0.0 )
		return GL_FALSE;
	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];
	*objx = out[0];
	*objy = out[1];
	*objz = out[2];
	return GL_TRUE;
}

#endif

}} // namespace EE::Graphics
