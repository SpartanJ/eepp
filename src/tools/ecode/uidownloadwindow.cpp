#include "uidownloadwindow.hpp"
#include "ecode.hpp"

namespace ecode {

void UIDownloadWindow::downloadFileWeb( const std::string& url ) {
	static const char* DOWNLOAD_WINDOW = R"xml(
<DownloadWindow width="300dp" height="120dp" padding="8dp" window-title="@string(downloading_file, Downloading File)" window-flags="default|shadow|maximize|draggable" window-min-size="300dp 80dp">
	<vbox lw="mp" lh="mp">
		<TextView text="@string(download_file, Downloading file:)" margin-bottom="8dp" />
		<TextView class="download_url" lw="mp" lh="wc" text="" margin-bottom="8dp" text-overflow="ellipsis" />
		<ProgressBar lw="mp" lh="12dp" progress="0" displayPercent="true" />
	</vbox>
</DownloadWindow>
	)xml";
	UIWidgetCreator::registerWidget( "downloadwindow", UIDownloadWindow::New );
	SceneManager::instance()
		->getUISceneNode()
		->loadLayoutFromString( DOWNLOAD_WINDOW )
		->asType<UIDownloadWindow>()
		->startDownload( url )
		->center();
}

UIDownloadWindow* UIDownloadWindow::New() {
	return eeNew( UIDownloadWindow, () );
}

UIDownloadWindow::UIDownloadWindow() : UIWindow() {
	setId( "window-" + UUID().toString() );
}

UIDownloadWindow::~UIDownloadWindow() {
	if ( mStatus == Status::Running && Http::Pool::getGlobal().exists( mURI, mProxyURI ) ) {
		auto http = Http::Pool::getGlobal().get( mURI, mProxyURI );
		http->setCancelRequest( mReqId );
	}
}

UIDownloadWindow* UIDownloadWindow::startDownload( const std::string& url ) {
	findByClass( "download_url" )->asType<UITextView>()->setText( url );
	URI uri( url );
	mURI = uri;
	mProxyURI = Http::getEnvProxyURI();
	mStatus = Status::Running;
	auto id = getId();
	mReqId = Http::getAsync(
		[uri, id]( const Http&, Http::Request&, Http::Response& response ) {
			std::string path( URI::getTempPathFromURI( uri ) );
			FileSystem::fileWrite( path, (const Uint8*)response.getBody().c_str(),
								   response.getBody().size() );
			if ( App::instance() && !App::instance()->isDestroyingApp() ) {
				App::instance()->getUISceneNode()->runOnMainThread( [path] {
					if ( App::instance() && !App::instance()->isDestroyingApp() )
						App::instance()->loadFileFromPath( path, true, nullptr, nullptr, false,
														   true );
				} );
			}

			if ( SceneManager::isActive() ) {
				auto nodeWin = SceneManager::instance()->getUISceneNode()->getRoot()->find( id );
				if ( nodeWin ) {
					auto me = nodeWin->asType<UIDownloadWindow>();
					me->mStatus = Status::Completed;
					me->closeWindow();
				}
			}
		},
		url, Seconds( 10 ),
		[id]( const Http& http, const Http::Request& request, const Http::Response& response,
			  const Http::Request::Status& status, std::size_t totalBytes,
			  std::size_t currentBytes ) {
			if ( SceneManager::isActive() ) {
				auto nodeWin = SceneManager::instance()->getUISceneNode()->getRoot()->find( id );
				if ( nodeWin ) {
					auto me = nodeWin->asType<UIDownloadWindow>();
					auto progress = me->findByType<UIProgressBar>( UI_TYPE_PROGRESSBAR );
					if ( status == Http::Request::Status::HeaderReceived ||
						 status == Http::Request::Status::ContentReceived ) {
						progress->runOnMainThread( [progress, currentBytes, totalBytes] {
							progress->setProgress(
								totalBytes > 0
									? ( static_cast<Float>( currentBytes ) / totalBytes ) * 100.f
									: 50 );
						} );
					}
				}
			}
			return true;
		},
		{}, "", true, mProxyURI );
	return this;
}

} // namespace ecode
