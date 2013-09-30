#include <eepp/ee.hpp>

// Get the process path to be used to load the sounds ( it is safer )
std::string AppPath = Sys::GetProcessPath();

/// Play a sound
void playSound() {
	// The sound manager class simplyfies the load of a SoundBuffer and the creation of the Sound
	// It manages the sound playing, if the sound channel is already playing, it will open a new channel to play the sound
	cSoundManager SoundManager;

	if ( SoundManager.LoadFromFile( "sound", AppPath + "assets/sounds/sound.ogg" ) ) {
		// Get the sound buffer to display the buffer information
		cSoundBuffer& buffer = SoundManager.GetBuffer( "sound" );

		// Display sound informations
		std::cout << "sound.ogg :" << std::endl;
		std::cout << " " << buffer.GetDuration().AsSeconds()	<< " seconds"       << std::endl;
		std::cout << " " << buffer.GetSampleRate()				<< " samples / sec" << std::endl;
		std::cout << " " << buffer.GetChannelCount()			<< " channels"      << std::endl;

		// Play the sound
		SoundManager.Play( "sound" );
	}
}

/// Play a music
void playMusic() {
	// Load an ogg music file
	cMusic music;

	if (!music.OpenFromFile( AppPath + "assets/sounds/music.ogg" ) )
		return;

	// Display music informations
	std::cout << "music.ogg :" << std::endl;
	std::cout << " " << music.GetDuration().AsSeconds()		<< " seconds"       << std::endl;
	std::cout << " " << music.GetSampleRate()				<< " samples / sec" << std::endl;
	std::cout << " " << music.GetChannelCount()				<< " channels"      << std::endl;

	// Play it
	music.Play();

	// Loop while the music is playing
	while ( music.State() == cSound::Playing ) {
		// Leave some CPU time for other processes
		Sys::Sleep( 100 );

		// Display the playing position
		std::cout << "\rPlaying... " << music.PlayingOffset().AsSeconds() << " sec   ";
		std::cout << std::flush;
	}

	std::cout << std::endl;
}

/// Entry point of application
EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Play a sound
	playSound();

	// Play a music
	playMusic();

	// Wait until the user presses 'enter' key
	std::cout << "Press enter to exit..." << std::endl;
	std::cin.ignore(10000, '\n');

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::ShowResults();

	return EXIT_SUCCESS;
}
