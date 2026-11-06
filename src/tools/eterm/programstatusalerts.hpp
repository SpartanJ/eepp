#pragma once

#include <atomic>
#include <eepp/system/clock.hpp>
#include <eterm/terminal/programstatus.hpp>
#include <functional>
#include <memory>

namespace EE::Audio {

class Sound;
class SoundBuffer;

} // namespace EE::Audio

namespace EE::System {

class ThreadPool;

} // namespace EE::System

namespace eterm {

using namespace EE::Audio;
using namespace EE::System;

/** Opt-in side effects of program status. The worker owns audio and native helper calls; no
 * terminal or UI pointer crosses that boundary. Destroy before the application's audio teardown. */
class ProgramStatusAlerts {
  public:
	ProgramStatusAlerts();

	~ProgramStatusAlerts();

	void shutdown();

	/** Percentage/text refreshes aren't state changes. A new done/error revision is a new result,
	 * even when working -> done was coalesced before reaching the UI. */
	static bool isTransition( const Terminal::PresentedProgramStatus* previous,
							  const Terminal::PresentedProgramStatus& next, bool allStates ) {
		using namespace Terminal;
		if ( next.source == ProgramStatusSource::None )
			return false;
		const bool rich = next.source == ProgramStatusSource::Osc7501;
		const bool result =
			rich && ( next.state == ProgramState::Done || next.state == ProgramState::Error );
		const bool attention = rich ? next.state == ProgramState::Blocked || result
									: next.legacyState == LegacyProgressState::Error ||
										  next.legacyState == LegacyProgressState::PausedOrWarning;
		if ( !allStates && !attention )
			return false;
		return !previous || previous->source != next.source ||
			   ( rich ? previous->recordId != next.recordId || previous->state != next.state ||
							previous->kind != next.kind ||
							( result && previous->revision != next.revision )
					  : previous->legacyState != next.legacyState );
	}

	/** At most one in-flight delivery and one alert every two seconds across all terminals.
	 * The sound name is chime, bell, pulse, or a local audio file path. */
	bool deliver( std::string title, std::string message, bool notification, bool sound,
				  std::string soundName, int volume, bool preview = false,
				  std::function<void( bool, std::string )> completed = {} );

  private:
	void playSound( const std::string& name, int volume );

	std::unique_ptr<SoundBuffer> mBuffer;
	std::unique_ptr<Sound> mSound;
	std::string mLoadedSound;
	Clock mLastDelivery;
	std::atomic<bool> mBusy{ false };
	bool mDelivered{ false };
	// Last: it joins while the audio objects and busy flag still exist.
	std::unique_ptr<ThreadPool> mWorker;
};

} // namespace eterm
