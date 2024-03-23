#include <eepp/ui/doc/languages/plaintext.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addPlaintext() {

SyntaxDefinitionManager::instance()->add(

{"Plain Text",
{},
{

},
{

},
"",
{},
"plaintext"
});
}

}}}} // namespace EE::UI::Doc::Language
