#pragma once

#include <functional>
#include <string>

#include <eepp/core/string.hpp>
#include <eepp/network/http.hpp>
#include <eepp/system/iostreamstring.hpp>

using namespace EE;
using namespace EE::Network;

namespace ecode {

class LLMChatCompletionRequest {
  public:
	using StreamedResponseCb = std::function<void( const std::string& chunk, bool fromReasoning )>;

	using StreamedResponseDoneCb =
		std::function<void( const LLMChatCompletionRequest& request, Http::Response& )>;

	using StreamedResponseCancelCb = std::function<void( const LLMChatCompletionRequest& request )>;

	StreamedResponseCb streamedResponseCb;

	StreamedResponseDoneCb doneCb;

	StreamedResponseCancelCb cancelCb;

	LLMChatCompletionRequest( const std::string& uri, const std::string& auth,
							  const std::string& reqBody, const std::string& provider );

	~LLMChatCompletionRequest();

	void request();

	void requestAsync();

	void cancel();

	bool isCancelled() const;

	const std::string& getStream() const;

	const std::string& getResponse() const { return mResponse; }

	const std::string& getReasoningResponse() const { return mReasoningResponse; }

  protected:
	URI mUrl;
	std::shared_ptr<Http> mHttp;
	Http::Request mRequest;
	IOStreamString mStream;
	std::string mReasoningResponse;
	std::string mResponse;
	size_t mReadBytes{ 0 };
	bool mCancel{ false };
	bool mCancelled{ false };
	bool mFirstMessage{ true };
	bool mReasoning{ false };
	bool mHadProgress{ false };
	Uint64 mRequestId{ 0 };

	void onCancel();
};

} // namespace ecode
