#ifndef ECODE_LSPCLIENT_HPP
#define ECODE_LSPCLIENT_HPP

#include <eepp/system/process.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <memory>

using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Doc;

namespace ecode {

class LSPClient {
  public:
	static std::shared_ptr<LSPClient> get( const std::shared_ptr<TextDocument>& doc );

  protected:

	LSPClient();
};

} // namespace ecode

#endif // ECODE_LSPCLIENT_HPP
