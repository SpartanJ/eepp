#ifndef EE_GRAPHICS_CRENDERERGL3_HPP
#define EE_GRAPHICS_CRENDERERGL3_HPP

#ifdef EE_GL3_ENABLED

#include "cgl.hpp"

#include <glm/glm.hpp> //OpenGL Mathematics (GLM).  A C++ mathematics library for 3D graphics.
#include <glm/ext.hpp>

namespace EE { namespace Graphics {

class cRendererGL3 : public cGL {
	public:
		cRendererGL3();

		~cRendererGL3();

		EEGL_version Version();

		void Init();

		void PushMatrix();

		void PopMatrix();

		void LoadIdentity();

		void Translatef( GLfloat x, GLfloat y, GLfloat z );

		void Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );

		void Scalef( GLfloat x, GLfloat y, GLfloat z );

		void MatrixMode (GLenum mode);

		void Ortho ( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
	protected:
		GLuint					shader_id;
		std::stack<glm::mat4>	glm_ProjectionMatrix;		// cpu-side
		GLint					glm_ProjectionMatrix_id;	// cpu-side hook to shader uniform
		std::stack<glm::mat4>	glm_ModelViewMatrix;		// cpu-side
		GLint					glm_ModelViewMatrix_id;		// cpu-side hook to shader uniform
		GLenum					mCurrentMode;
		std::stack<glm::mat4>*	mCurMatrix;

		void UpdateMatrix();
};

}}

#endif

#endif
