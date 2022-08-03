#ifndef EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
#define EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP

#include <eepp/config.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <vector>

namespace EE { namespace UI { namespace Doc {

class EE_API SyntaxDefinitionManager {
	SINGLETON_DECLARE_HEADERS( SyntaxDefinitionManager )
  public:
	SyntaxDefinition& add( SyntaxDefinition&& syntaxStyle );

	const SyntaxDefinition& getPlainStyle() const;

	const SyntaxDefinition& getStyleByExtension( const std::string& filePath ) const;

	const SyntaxDefinition& getStyleByHeader( const std::string& header ) const;

	const SyntaxDefinition& find( const std::string& filePath, const std::string& header );

	SyntaxDefinition& getStyleByExtensionRef( const std::string& filePath );

	const SyntaxDefinition& getStyleByLanguageName( const std::string& name ) const;

	const SyntaxDefinition& getStyleByLanguageId( const String::HashType& id ) const;

	SyntaxDefinition& getStyleByLanguageNameRef( const std::string& name );

	std::vector<std::string> getLanguageNames() const;

	std::vector<std::string> getExtensionsPatternsSupported() const;

  protected:
	SyntaxDefinitionManager();

	std::vector<SyntaxDefinition> mStyles;

	void addPlainText();

	void addXML();

	void addHTML();

	void addCSS();

	void addMarkdown();

	void addC();

	void addLua();

	void addJavaScript();

	void addJSON();

	void addTypeScript();

	void addPython();

	void addBash();

	void addCPP();

	void addPHP();

	void addSQL();

	void addGLSL();

	void addIni();

	void addMakefile();

	void addCSharp();

	void addGo();

	void addRust();

	void addGDScript();

	void addD();

	void addHaskell();

	void addHLSL();

	void addLatex();

	void addMeson();

	void addAngelScript();

	void addBatchScript();

	void addDiff();

	void addJava();

	void addYAML();

	void addSwift();

	void addSolidity();

	void addObjetiveC();

	void addDart();

	void addKotlin();

	void addZig();

	void addNim();

	void addCMake();

	void addJSX();

	void addContainerfile();

	void addOdin();

	void addIgnore();

	void addPowerShell();

	void addWren();

	void addEnv();

	void addRuby();

	void addScala();

	void addSass();

	void addPO();

	void addPerl();
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
