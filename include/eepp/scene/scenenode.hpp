#ifndef EE_SCENENODE_HPP
#define EE_SCENENODE_HPP

#include <eepp/scene/node.hpp>

namespace EE { namespace Graphics {
class FrameBuffer;
}}

namespace EE { namespace Window {
class Window;
}}

namespace EE { namespace Scene {

class EE_API SceneNode : public Node {
	public:
		static SceneNode * New( EE::Window::Window * window = NULL );

		SceneNode( EE::Window::Window * window = NULL );

		~SceneNode();

		void enableFrameBuffer();

		void disableFrameBuffer();

		virtual void draw();

		virtual void update( const Time& elapsed );

		bool invalidated();

		void enableDrawInvalidation();

		void disableDrawInvalidation();
	protected:
		friend class Node;

		EE::Window::Window * mWindow;
		FrameBuffer * mFrameBuffer;
		std::list<Node*>	mCloseList;
		bool mFrameBufferBound;
		bool mUseInvalidation;

		virtual void onSizeChange();

		virtual void matrixSet();

		virtual void matrixUnset();

		void addToCloseQueue( Node * Ctrl );

		void checkClose();

		void createFrameBuffer();

		void drawFrameBuffer();

		Sizei getFrameBufferSize();
};

}}

#endif
