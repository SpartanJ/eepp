#ifndef EE_WINDOWCJOYSTICKMANAGERSDL3_HPP
#define EE_WINDOWCJOYSTICKMANAGERSDL3_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL3/base.hpp>

#ifdef EE_BACKEND_SDL3

#include <eepp/system/thread.hpp>
#include <eepp/window/joystickmanager.hpp>
#include <unordered_map>
#include <vector>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

class EE_API JoystickManagerSDL : public JoystickManager {
  public:
	JoystickManagerSDL();

	virtual ~JoystickManagerSDL();

	void update();

	void close();

	void open( OpenCb openCb = nullptr );

	void addJoystick( SDL_JoystickID id );

	void removeJoystick( SDL_JoystickID id );

	void rescan() override;

  protected:
	void create( const Uint32& index ) override;

	void openAsync();

	Thread mAsyncInit;
	bool mInit{ false };
	Uint32 mCount{ 0 };
	std::vector<SDL_JoystickID> mIds;
	std::unordered_map<SDL_JoystickID, Uint32> mIdToIndex;

public:
	Uint32 getIndexFromID( SDL_JoystickID id ) const;
};

}}}} // namespace EE::Window::Backend::SDL3

#endif
#endif
