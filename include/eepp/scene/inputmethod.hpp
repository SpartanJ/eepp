#ifndef EE_SCENE_IMESTATE_HPP
#define EE_SCENE_IMESTATE_HPP

#include <eepp/core/string.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/math/rect.hpp>

using namespace EE::Math;
using namespace EE::Graphics;

namespace EE { namespace Scene {

class SceneNode;

class InputMethod {
  public:
	struct State {
		String text;
		Int32 start{ 0 };
		Int32 length{ 0 };
	};

	void setLocation( Rect rect );

	bool isEditing() const;

	void reset();

	void stop();

	void onTextEditing( const String& text, const Int32& start, const Int32& length );

	const InputMethod::State& getState() const;

	void draw( const Vector2f& screenPos, const Float& lineHeight, const FontStyleConfig& fontStyle,
			   const Color& backgroundColor = Color::Transparent,
			   const Color& lineColor = Color::Transparent );

  protected:
	friend class SceneNode;

	explicit InputMethod( SceneNode* sceneNode ) : mSceneNode( sceneNode ) {}

	SceneNode* mSceneNode{ nullptr };
	InputMethod::State mState;
	bool mEditing{ false };
};

}} // namespace EE::Scene

#endif // EE_SCENE_IMESTATE_HPP
