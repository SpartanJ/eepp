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
	using StreamedResponseCb = std::function<void( const std::string& chunk )>;

	using StreamedResponseDoneCb =
		std::function<void( const LLMChatCompletionRequest& request, Http::Response& )>;

	StreamedResponseCb streamedResponseCb;

	StreamedResponseDoneCb doneCb;

	LLMChatCompletionRequest( const std::string& uri, const std::string& auth,
							  const std::string& reqBody, const std::string& provider );

	void request();

	void requestAsync();

	void cancel();

	const std::string& getStream();

  protected:
	URI mUrl;
	Http mHttp;
	Http::Request mRequest;
	IOStreamString mStream;
	std::string mResponse;
	size_t mReadBytes{ 0 };
	bool mCancel{ false };
};

} // namespace ecode
