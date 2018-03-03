#ifndef EE_WINDOW_DISPLAYMANAGERSFML_HPP 
#define EE_WINDOW_DISPLAYMANAGERSFML_HPP

#include <eepp/window/displaymanager.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

class EE_API DisplaySFML : public Display {
	public:
		DisplaySFML( int index );

		std::string getName();
		
		Rect getBounds();
		
		Rect getUsableBounds();
		
		Float getDPI();
		
		const int& getIndex() const;
		
		DisplayMode getCurrentMode();
		
		DisplayMode getClosestDisplayMode( DisplayMode wantedMode );
		
		const std::vector<DisplayMode>& getModes() const;
};

class EE_API DisplayManagerSFML : public DisplayManager {
	public:
		int getDisplayCount();
		
		Display * getDisplayIndex( int index );
};

}}}}

#endif
