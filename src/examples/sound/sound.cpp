#include <eepp/ee.hpp>

/// Play a sound
void playSound() {
	// The sound manager class simplyfies the load of a SoundBuffer and the creation of the Sound
	// It manages the sound playing, if the sound channel is already playing, it will open a new channel to play the sound
	SoundManager SoundManager;

	if ( SoundManager.loadFromFile( "sound", "assets/sounds/sound.ogg" ) ) {
		// Get the sound buffer to display the buffer information
		SoundBuffer& buffer = SoundManager.getBuffer( "sound" );

		// Display sound informations
		std::cout << "sound.ogg :" << std::endl;
		std::cout << " " << buffer.getDuration().asSeconds()	<< " seconds"		<< std::endl;
		std::cout << " " << buffer.getSampleRate()				<< " samples / sec"	<< std::endl;
		std::cout << " " << buffer.getChannelCount()			<< " channels"		<< std::endl;

		// Play the sound
		SoundManager.play( "sound" );
	}
}

/// Play a music
void playMusic() {
	// Load an ogg music file
	Music music;

	if (!music.openFromFile( "assets/sounds/music.ogg" ) )
		return;

	// Display music informations
	std::cout << "music.ogg :" << std::endl;
	std::cout << " " << music.getDuration().asSeconds()		<< " seconds"		<< std::endl;
	std::cout << " " << music.getSampleRate()				<< " samples / sec"	<< std::endl;
	std::cout << " " << music.getChannelCount()				<< " channels"		<< std::endl;

	// Play it
	music.play();

	// Loop while the music is playing
	while ( music.getState() == Sound::Playing ) {
		// Leave some CPU time for other processes
		Sys::sleep( 100 );

		// Display the playing position
		std::cout << "\rPlaying... " << music.getPlayingOffset().asSeconds() << " sec   ";
		std::cout << std::flush;
	}

	std::cout << std::endl;
}

/// Entry point of application
EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Set the application current directory path
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	// Play a sound
	playSound();

	// Play a music
	playMusic();

	// Wait until the user presses 'enter' key
	std::cout << "Press enter to exit..." << std::endl;
	std::cin.ignore(10000, '\n');

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
