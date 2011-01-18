#ifndef EE_GRAPHICS_CRENDERERGL_HPP
#define EE_GRAPHICS_CRENDERERGL_HPP

#include "cgl.hpp"

namespace EE { namespace Graphics {

class cRendererGL : public cGL {
	public:
		cRendererGL();
		
		~cRendererGL();

		EEGL_version Version();

		void PushMatrix();

		void PopMatrix();

		void LoadIdentity();

		void Translatef( GLfloat x, GLfloat y, GLfloat z );

		void Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );

		void Scalef( GLfloat x, GLfloat y, GLfloat z );

		void MatrixMode (GLenum mode);

		void Ortho ( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );

	protected:	
		
};

}}

#endif
