#ifndef EE_GRAPHICS_CRENDERERGL_HPP
#define EE_GRAPHICS_CRENDERERGL_HPP

#include <eepp/graphics/renderer/gl.hpp>

namespace EE { namespace Graphics {

//! Avoid compilling the fixed pipeline renderer for GLES2, because it's not supported.
#if !defined( EE_GLES2 ) || defined( EE_GLES_BOTH )

class EE_API RendererGL : public cGL {
	public:
		RendererGL();
		
		~RendererGL();

		EEGL_version Version();

		std::string VersionStr();

		void ClientActiveTexture( unsigned int texture );

		void PointSize( float size );

		float PointSize();

		void PushMatrix();

		void PopMatrix();

		void LoadIdentity();

		void Translatef( float x, float y, float z );

		void Rotatef( float angle, float x, float y, float z );

		void Scalef( float x, float y, float z );

		void MatrixMode (unsigned int mode);

		void Ortho ( float left, float right, float bottom, float top, float zNear, float zFar );

		void LookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ );

		void Perspective ( float fovy, float aspect, float zNear, float zFar );

		void EnableClientState( unsigned int array );

		void DisableClientState( unsigned int array );

		void VertexPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate );

		void ColorPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate );

		void TexCoordPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate );

		void ClipPlane( unsigned int plane, const double *equation );

		void Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height );

		void Clip2DPlaneDisable();

		void TexEnvi( unsigned int target, unsigned int pname, int param );

		void MultMatrixf ( const float *m );

		void LoadMatrixf( const float *m );

		void Frustum( float left, float right, float bottom, float top, float near_val, float far_val );

		void GetCurrentMatrix( unsigned int mode, float * m );

		unsigned int GetCurrentMatrixMode();

		int Project( float objx, float objy, float objz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *winx, float *winy, float *winz );

		int UnProject( float winx, float winy, float winz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *objx, float *objy, float *objz );
	protected:	
		
};

#endif

}}

#endif
