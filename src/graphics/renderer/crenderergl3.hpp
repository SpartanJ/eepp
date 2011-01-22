#ifndef EE_GRAPHICS_CRENDERERGL3_HPP
#define EE_GRAPHICS_CRENDERERGL3_HPP

#include "cgl.hpp"

#ifdef EE_GL3_ENABLED

// Xlib Madness
#ifdef True
#undef True
#endif

#ifdef False
#undef False
#endif

#include <glm/gtx/matrix_projection.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/transform2.hpp>

namespace EE { namespace Graphics {

enum EEGL_SHADERS_NUM {
	EEGL_SHADER_BASE_TEX,
	EEGL_SHADERS_COUNT
};

class cRendererGL3 : public cGL {
	public:
		cRendererGL3();

		~cRendererGL3();

		EEGL_version Version();

		void Init();

		void PushMatrix();

		void PopMatrix();

		void LoadIdentity();

		void Disable ( GLenum cap );

		void Enable( GLenum cap );

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

		GLuint BaseShaderId();

		void SetShader( cShaderProgram * Shader );
	protected:
		std::stack<glm::mat4>	glm_ProjectionMatrix;		// cpu-side
		GLint					glm_ProjectionMatrix_id;	// cpu-side hook to shader uniform
		std::stack<glm::mat4>	glm_ModelViewMatrix;		// cpu-side
		GLint					glm_ModelViewMatrix_id;		// cpu-side hook to shader uniform
		GLenum					mCurrentMode;
		std::stack<glm::mat4>*	mCurMatrix;
		cShaderProgram *		mShaders[ EEGL_SHADERS_COUNT ];
		cShaderProgram *		mCurShader;
		GLuint					mVAO;
		GLuint					mVBO[ EEGL_ARRAY_STATES_SIZE ];

		void UpdateMatrix();
};

}}

#endif

#endif
