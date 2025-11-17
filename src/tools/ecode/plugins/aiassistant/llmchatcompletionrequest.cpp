#include "llmchatcompletionrequest.hpp"

#include <nlohmann/json.hpp>

namespace ecode {

LLMChatCompletionRequest::LLMChatCompletionRequest( const std::string& uri, const std::string& auth,
													const std::string& reqBody,
													const std::string& provider ) :
	mUrl( uri ), mHttp( Http::Pool::getGlobal().get( URI( uri ) ) ) {
	mRequest.setUri( mUrl.getPathEtc() );
	mRequest.setHeader( "Content-Type", "application/json" );
	if ( provider == "anthropic" ) {
		mRequest.setHeader( "x-api-key", auth );
		mRequest.setHeader( "anthropic-version", "2023-06-01" );
	} else if ( !auth.empty() ) {
		mRequest.setHeader( "Authorization", String::format( "Bearer %s", auth ) );
	}
	mRequest.setBody( reqBody );
	mRequest.setFollowRedirect( true );
	mRequest.setMethod( Http::Request::Method::Post );
	mRequest.setCancelCallback( [this]( const Http&, const Http::Request& ) { onCancel(); } );
	mRequest.setProgressCallback( [this]( const Http&, const Http::Request&, const Http::Response&,
										  const Http::Request::Status& status, size_t, size_t ) {
		if ( mCancel ) {
			onCancel();
			return false;
		}
		mHadProgress = true;
		if ( status != Http::Request::ContentReceived )
			return true;
		std::string chunk =
			mReadBytes ? mStream.getStream().substr( mReadBytes ) : mStream.getStream();
		mReadBytes = mStream.getStream().size();
		String::readBySeparator( chunk, [this]( std::string_view subchunk ) {
			if ( subchunk.empty() )
				return;
			// OpenAI format?
			if ( !String::startsWith( subchunk, "data: " ) ) {
				// ollama format?
				if ( !subchunk.empty() && subchunk[0] == '{' ) {
					nlohmann::json j = nlohmann::json::parse( subchunk.begin(), subchunk.end(),
															  nullptr, false, true );

					if ( j.contains( "message" ) && j["message"].contains( "content" ) ) {
						const auto& msg = j["message"];
						std::string delta = msg["content"];
						if ( delta.empty() )
							return;
						if ( mFirstMessage ) {
							if ( String::startsWith( delta, "<think>" ) )
								mReasoning = true;
							mFirstMessage = false;
						}

						if ( streamedResponseCb )
							streamedResponseCb( delta, mReasoning );
						if ( mReasoning ) {
							if ( String::endsWith( delta, "</think>" ) )
								mReasoning = false;
							mReasoningResponse += std::move( delta );
						} else {
							mResponse += std::move( delta );
						}
					}
				}

				return;
			}
			std::string jsonStr( subchunk.substr( 6 ) );
			if ( jsonStr == "[DONE]" )
				return;
			nlohmann::json data = nlohmann::json::parse( jsonStr, nullptr, false, true );

			// OpenAI
			if ( data.contains( "choices" ) && data["choices"].is_array() ) {
				nlohmann::json& choices = data["choices"];

				for ( const auto& choice : choices ) {
					if ( choice["delta"].contains( "reasoning_content" ) ) {
						std::string delta = choice["delta"]["reasoning_content"];
						if ( streamedResponseCb )
							streamedResponseCb( delta, true );
						mReasoningResponse += std::move( delta );
					} else if ( choice["delta"].contains( "content" ) ) {
						std::string delta = choice["delta"]["content"];
						if ( streamedResponseCb )
							streamedResponseCb( delta, false );
						mResponse += std::move( delta );
					}
				}
				// Anthropic
			} else if ( data.contains( "delta" ) &&
						data["delta"].value( "type", "" ) == "text_delta" ) {
				std::string delta = data["delta"]["text"];
				if ( streamedResponseCb )
					streamedResponseCb( delta, false );
				mResponse += std::move( delta );
			}
		} );
		return true;
	} );
}

LLMChatCompletionRequest::~LLMChatCompletionRequest() {
	cancel();
}

void LLMChatCompletionRequest::request() {
	mCancel = false;
	mCancelled = false;
	mHadProgress = false;
	Http::Response res = mHttp->downloadRequest( mRequest, mStream, Seconds( 5 ) );
	if ( doneCb )
		doneCb( *this, res );
}

void LLMChatCompletionRequest::requestAsync() {
	mCancel = false;
	mCancelled = false;
	mHadProgress = false;
	mRequestId = mHttp->downloadAsyncRequest(
		[this]( const Http&, Http::Request&, Http::Response& res ) {
			if ( doneCb )
				doneCb( *this, res );
		},
		mRequest, mStream, Seconds( 5 ) );
}

void LLMChatCompletionRequest::cancel() {
	mCancel = true;
	if ( mRequestId )
		mHttp->setCancelRequest( mRequestId );
	if ( !mHadProgress )
		onCancel();
}

const std::string& LLMChatCompletionRequest::getStream() const {
	return mStream.getStream();
}

bool LLMChatCompletionRequest::isCancelled() const {
	return mCancelled;
}

void LLMChatCompletionRequest::onCancel() {
	if ( !mCancelled && cancelCb )
		cancelCb( *this );
	mCancelled = true;
}

} // namespace ecode
