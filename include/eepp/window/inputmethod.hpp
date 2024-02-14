#ifndef EE_WINDOW_IMESTATE_HPP
#define EE_WINDOW_IMESTATE_HPP

#include <eepp/core/string.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/math/rect.hpp>
#include <map>

using namespace EE::Math;
using namespace EE::Graphics;

namespace EE { namespace Window {

class Window;

class EE_API InputMethod {
  public:
	typedef std::function<void( const String&, Int32, Int32 )> TextEditingCb;

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
			   const Color& lineColor = Color::Transparent,
			   const Color& backgroundColor = Color::Transparent, bool drawText = false );

	Uint32 addTextEditingCb( TextEditingCb cb );

	void removeTextEditingCb( Uint32 cbId );

  protected:
	friend class Window;

	explicit InputMethod( EE::Window::Window* window );

	EE::Window::Window* mWindow{ nullptr };
	InputMethod::State mState;
	bool mEditing{ false };
	Rect mLastLocation;
	Uint32 mLastCb{ 0 };
	std::map<Uint32, TextEditingCb> mEditingCbs;

	void sendTextEditing( const String& txt, Int32 start, Int32 length );
};

}} // namespace EE::Window

#endif // EE_WINDOW_IMESTATE_HPP
