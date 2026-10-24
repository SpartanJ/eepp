#include <eepp/audio/music.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/tools/uiaudioplayer.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <utility>

namespace EE::UI::Tools {

static const auto AUDIO_PLAYER_LAYOUT = R"xml(
<style>
.audio_player {
	layout-height: 44dp;
	layout-gravity: center;
	background-color: var(--button-back);
	border-color: var(--button-border);
	border-radius: 22dp;
	padding-left: 8dp;
	padding-right: 8dp;
	border-width: 1.5dp;
}
.audio_player > * {
	layout-gravity: center;
}
.audio_player > .audio_btn {
	lw: 24dp;
	lh: 24dp;
	background-color: transparent;
	border-radius: 12dp;
	foreground-position: center;
	foreground-tint: var(--font);
	transition: all 0.1s;
}
.audio_player > .audio_btn:hover {
	background-color: light-dark(var(--scrollbar-hback-hover), var(--primary));
	cursor: hand;
}
.audio_player > .play_btn {
	foreground-image: icon("play-filled", 16dp);
}
.audio_player > .pause_btn {
	foreground-image: icon("pause-fill", 16dp);
}
.audio_player > .stop_btn {
	foreground-image: icon("stop", 16dp);
}
.audio_player > .volume_btn {
	foreground-image: icon("volume-up-fill", 16dp);
}
.audio_player > .volume_btn.mute {
	foreground-image: icon("volume-mute-fill", 16dp);
}
.audio_player > .progressbar {
	layout-width: 0dp;
	layout-weight: 1;
	layout-height: wrap_content;
	progress: 0;
	margin-left: 4dp;
	margin-right: 4dp;
	background-color: var(--scrollbar-hback-hover);
	border-radius: 4dp;
}
.audio_player > .time_progress {
	margin-left: 4dp;
	margin-right: 4dp;
}
</style>
<hbox class="audio_player">
	<Widget class="audio_btn status_btn play_btn" />
	<TextView class="time_progress" text="0:00 / 0:00" />
	<ProgressBar class="progressbar" />
	<Widget class="audio_btn volume_btn" />
</hbox>
	)xml";

static std::string formatSingleTime( const Time& t ) {
	long totalSeconds = static_cast<long>( t.asSeconds() );
	long minutes = totalSeconds / 60;
	long seconds = totalSeconds % 60;
	return String::format( "%ld:%02ld", minutes, seconds );
}

static std::string toTimeString( const Time& t, const Time& t2 ) {
	return String::format( "%s / %s", formatSingleTime( t ), formatSingleTime( t2 ) );
}

UIAudioPlayer* UIAudioPlayer::New() {
	return eeNew( UIAudioPlayer, () );
}

UIAudioPlayer::UIAudioPlayer() : UIRelativeLayout( "audioplayer" ) {
	getUISceneNode()->loadLayoutFromString( AUDIO_PLAYER_LAYOUT, this,
											String::hash( "audioplayer" ) );
	mProgressBar = findByClass<UIProgressBar>( "progressbar" );
	mStatusBtn = findByClass( "status_btn" );
	mVolumeBtn = findByClass( "volume_btn" );
	mTimeView = findByClass<UITextView>( "time_progress" );
	mStatusBtn->onClick( [this]( auto ) { control( Command::Toggle ); } );
	mVolumeBtn->onClick( [this]( auto ) {
		if ( mVolumeBtn->hasClass( "mute" ) ) {
			mMusic->setVolume( 100.f );
			mVolumeBtn->removeClass( "mute" );
		} else {
			mMusic->setVolume( 0.f );
			mVolumeBtn->addClass( "mute" );
		}
	} );
	const auto offsetFn = [this]( const Event* event ) {
		auto mev = event->asMouseEvent();
		if ( mev->getFlags() & EE_BUTTON_LMASK ) {
			auto pos( mProgressBar->convertToNodeSpace( mev->getPosition().asFloat() ) );
			const Float newProgress =
				eeclamp( pos.x / mProgressBar->getPixelsSize().getWidth(), 0.f, 100.f );
			control( Command::Seek, newProgress * mMusic->getDuration() );
		}
	};
	mProgressBar->on( Event::MouseDown, offsetFn );
	mProgressBar->getFiller()->on( Event::MouseDown, offsetFn );
	mMusic = std::make_shared<Music>();
	subscribeScheduledUpdate();
}

Uint32 UIAudioPlayer::getType() const {
	return UI_TYPE_AUDIO_PLAYER;
}

bool UIAudioPlayer::isType( const Uint32& type ) const {
	return UIAudioPlayer::getType() == type ? true : UIWidget::isType( type );
}

void UIAudioPlayer::loadFromPath( const std::string& path, bool autoPlay ) {
	auto audio = std::make_unique<Music>();
	if ( !audio->openFromFile( path ) ) {
		reset();
		return;
	}
	setAudio( std::move( audio ), autoPlay );
	mFilePath = path;
}

bool UIAudioPlayer::loadFromStream( std::unique_ptr<IOStream> stream, bool autoPlay ) {
	auto audio = std::make_unique<Music>();
	if ( !audio->openFromStream( std::move( stream ) ) ) {
		reset();
		return false;
	}
	setAudio( std::move( audio ), autoPlay );
	return true;
}

void UIAudioPlayer::setAudio( std::unique_ptr<Music> audio, bool autoPlay ) {
	if ( !audio )
		return;
	const Float volume = mMusic->getVolume();
	// Move the last player-owned reference into the job too: a fast stop must not leave the
	// UI thread responsible for the decoder's destructor and its final stream seek/cleanup.
	auto previous = std::exchange( mMusic, std::shared_ptr<Music>( std::move( audio ) ) );
	enqueue( [audio = std::move( previous )] { audio->stop(); } );
	mMusic->setVolume( volume );
	mFilePath.clear();
	if ( autoPlay )
		play();
}

void UIAudioPlayer::reset() {
	setAudio( std::make_unique<Music>(), false );
}

template <typename Action> void UIAudioPlayer::enqueue( Action&& action ) {
	if ( getUISceneNode()->hasThreadPool() ) {
		if ( !mControls )
			mControls.emplace( makeReadyTask() );
		*mControls =
			std::move( *mControls )
				.then( *getUISceneNode()->getThreadPool(), std::forward<Action>( action ) );
	} else {
		action();
	}
}

void UIAudioPlayer::control( Command command, Time offset ) {
	enqueue( [audio = mMusic, command, offset] {
		switch ( command ) {
			case Command::Toggle:
				if ( audio->getStatus() == Audio::SoundSource::Playing ) {
					audio->pause();
					break;
				}
				[[fallthrough]];
			case Command::Play:
				if ( audio->getStatus() == Audio::SoundSource::Stopped )
					audio->setPlayingOffset( Time::Zero );
				audio->play();
				break;
			case Command::Pause:
				audio->pause();
				break;
			case Command::Stop:
				audio->stop();
				break;
			case Command::Seek:
				audio->setPlayingOffset( offset );
				break;
		}
	} );
}

void UIAudioPlayer::play() {
	control( Command::Play );
}

void UIAudioPlayer::pause() {
	control( Command::Pause );
}

void UIAudioPlayer::stop() {
	control( Command::Stop );
}

void UIAudioPlayer::scheduledUpdate( const Time& time ) {
	if ( !isVisible() )
		return;
	static std::vector<std::string> PLAY_CLASSES = { "audio_btn", "status_btn", "play_btn" };
	static std::vector<std::string> PAUSE_CLASSES = { "audio_btn", "status_btn", "pause_btn" };

	switch ( mMusic->getStatus() ) {
		case Audio::SoundSource::Playing: {
			mStatusBtn->setClasses( PAUSE_CLASSES );
			break;
		}
		case Audio::SoundSource::Paused: {
			mStatusBtn->setClasses( PLAY_CLASSES );
			break;
		}
		case Audio::SoundSource::Stopped: {
			mStatusBtn->setClasses( PLAY_CLASSES );
			break;
		}
	}
	const Time duration = mMusic->getDuration();
	const Time offset = mMusic->getPlayingOffset();
	const Float progress =
		duration > Time::Zero ? offset.asSeconds() / duration.asSeconds() * 100.f : 0.f;
	mProgressBar->setProgress( eeceil( progress ) );
	mTimeView->setText( toTimeString( offset, duration ) );
}

UIAudioPlayer::~UIAudioPlayer() {
	enqueue( [audio = std::move( mMusic )] { audio->stop(); } );
}

Music* UIAudioPlayer::getAudio() const {
	return mMusic.get();
}

const std::string& UIAudioPlayer::getFilePath() const {
	return mFilePath;
}

} // namespace EE::UI::Tools
