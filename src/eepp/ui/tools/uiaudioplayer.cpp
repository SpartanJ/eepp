#include <eepp/audio/music.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/tools/uiaudioplayer.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uiscenenode.hpp>

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
	getUISceneNode()->loadLayoutFromString( AUDIO_PLAYER_LAYOUT, this );
	mProgressBar = findByClass<UIProgressBar>( "progressbar" );
	mStatusBtn = findByClass( "status_btn" );
	mVolumeBtn = findByClass( "volume_btn" );
	mTimeView = findByClass<UITextView>( "time_progress" );
	mStatusBtn->onClick( [this]( auto ) {
		if ( mMusic->getStatus() == Audio::SoundSource::Playing ) {
			mMusic->pause();
		} else if ( mMusic->getStatus() == Audio::SoundSource::Paused ) {
			mMusic->play();
		} else if ( mMusic->getStatus() == Audio::SoundSource::Stopped ) {
			mMusic->setPlayingOffset( Time::Zero );
			mMusic->play();
		}
	} );
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
			mMusic->setPlayingOffset( newProgress * mMusic->getDuration() );
		}
	};
	mProgressBar->on( Event::MouseDown, offsetFn );
	mProgressBar->getFiller()->on( Event::MouseDown, offsetFn );
	mMusic = Music::New();
	subscribeScheduledUpdate();
}

Uint32 UIAudioPlayer::getType() const {
	return UI_TYPE_AUDIO_PLAYER;
}

bool UIAudioPlayer::isType( const Uint32& type ) const {
	return UIAudioPlayer::getType() == type ? true : UIWidget::isType( type );
}

void UIAudioPlayer::loadFromPath( const std::string& path, bool autoPlay ) {
	mFilePath = path;
	mMusic->openFromFile( path );
	if ( autoPlay )
		mMusic->play();
}

void UIAudioPlayer::play() {
	mMusic->play();
}

void UIAudioPlayer::pause() {
	mMusic->pause();
}

void UIAudioPlayer::stop() {
	mMusic->stop();
}

void UIAudioPlayer::scheduledUpdate( const Time& time ) {
	static std::vector<std::string> PLAY_CLASSES = { "audio_btn", "status_btn", "play_btn" };
	static std::vector<std::string> PAUSE_CLASSES = { "audio_btn", "status_btn", "pause_btn" };

	switch ( mMusic->getStatus() ) {
		case Audio::SoundSource::Playing: {
			mStatusBtn->setClasses( PAUSE_CLASSES );
			Time offset = mMusic->getPlayingOffset();
			Time duration = mMusic->getDuration();
			Float progress = offset.asMilliseconds() / duration.asMilliseconds() * 100;
			mProgressBar->setProgress( eeceil( progress ) );
			String str( toTimeString( offset, duration ) );
			mTimeView->setText( std::move( str ) );
			break;
		}
		case Audio::SoundSource::Paused: {
			mStatusBtn->setClasses( PLAY_CLASSES );
			break;
		}
		case Audio::SoundSource::Stopped: {
			mStatusBtn->setClasses( PLAY_CLASSES );
			mProgressBar->setProgress( 100.f );
			break;
		}
	}
}

UIAudioPlayer::~UIAudioPlayer() {
	eeSAFE_DELETE( mMusic );
}

Music* UIAudioPlayer::getAudio() const {
	return mMusic;
}

const std::string& UIAudioPlayer::getFilePath() const {
	return mFilePath;
}

} // namespace EE::UI::Tools
