#include <eepp/ee.hpp>
#include <iostream>

/// Play a sound
void playSound() {
	// The sound manager class simplyfies the load of a SoundBuffer and the creation of the Sound
	// It manages the sound playing, if the sound channel is already playing, it will open a new
	// channel to play the sound
	SoundManager soundManager;

	if ( soundManager.loadFromFile( "sound", "assets/sounds/sound.ogg" ) ) {
		// Get the sound buffer to display the buffer information
		SoundBuffer& buffer = soundManager.getBuffer( "sound" );

		// Display sound information
		std::cout << "sound.ogg :" << std::endl;
		std::cout << " " << buffer.getDuration().asSeconds() << " seconds" << std::endl;
		std::cout << " " << buffer.getSampleRate() << " samples / sec" << std::endl;
		std::cout << " " << buffer.getChannelCount() << " channels" << std::endl;

		// Play the sound
		Sound* sound = soundManager.play( "sound" );

		while ( sound->getStatus() == Sound::Playing ) {
			Sys::sleep( Milliseconds( 100 ) );

			// Display the playing position
			std::cout << "\rPlaying... " << sound->getPlayingOffset().asSeconds() << " sec        ";
			std::cout << std::flush;
		}
	}
}

/// Play a music
void playMusic( std::string path = "assets/sounds/music.ogg" ) {
	// Load an ogg music file
	Music music;

	if ( !music.openFromFile( path ) )
		return;

	// Display music information
	std::cout << FileSystem::fileNameFromPath( path ) << " :" << std::endl;
	std::cout << " " << music.getDuration().asSeconds() << " seconds" << std::endl;
	std::cout << " " << music.getSampleRate() << " samples / sec" << std::endl;
	std::cout << " " << music.getChannelCount() << " channels" << std::endl;

	// Play it
	music.play();

	// Loop while the music is playing
	while ( music.getStatus() == Sound::Playing ) {
		// Leave some CPU time for other processes
		Sys::sleep( Milliseconds( 100 ) );

		// Display the playing position
		std::cout << "\rPlaying... " << music.getPlayingOffset().asSeconds() << " sec   ";
		std::cout << std::flush;
	}

	std::cout << std::endl;
}

/// Entry point of application
EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	if ( argc >= 2 ) {
		playMusic( argv[1] );
	} else {
		// Play a sound
		playSound();

		// Play a music
		playMusic();
	}

	// Wait until the user presses 'enter' key
	std::cout << "Press enter to exit..." << std::endl;
	std::cin.ignore( 10000, '\n' );

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
