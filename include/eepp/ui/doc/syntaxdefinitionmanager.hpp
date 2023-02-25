#ifndef EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
#define EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP

#include <eepp/config.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <optional>
#include <vector>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

class EE_API SyntaxDefinitionManager {
	SINGLETON_DECLARE_HEADERS( SyntaxDefinitionManager )
  public:
	static std::string toCPP( const SyntaxDefinition& def );

	SyntaxDefinition& add( SyntaxDefinition&& syntaxStyle );

	const SyntaxDefinition& getPlainStyle() const;

	const SyntaxDefinition& getByExtension( const std::string& filePath ) const;

	const SyntaxDefinition& getByHeader( const std::string& header ) const;

	const SyntaxDefinition& find( const std::string& filePath, const std::string& header );

	SyntaxDefinition& getByExtensionRef( const std::string& filePath );

	const SyntaxDefinition& getByLanguageName( const std::string& name ) const;

	const SyntaxDefinition& getByLSPName( const std::string& name ) const;

	const SyntaxDefinition& getByLanguageId( const String::HashType& id ) const;

	SyntaxDefinition& getByLanguageNameRef( const std::string& name );

	std::vector<std::string> getLanguageNames() const;

	std::vector<std::string> getExtensionsPatternsSupported() const;

	const SyntaxDefinition* getPtrByLanguageName( const std::string& name ) const;

	const SyntaxDefinition* getPtrByLanguageId( const String::HashType& id ) const;

	bool loadFromStream( IOStream& stream, std::vector<std::string>* addedLangs );

	bool loadFromStream( IOStream& stream );

	bool loadFromFile( const std::string& fpath );

	bool loadFromMemory( const Uint8* data, const Uint32& dataSize );

	bool loadFromPack( Pack* Pack, const std::string& filePackPath );

	void loadFromFolder( const std::string& folderPath );

	const std::vector<SyntaxDefinition>& getDefinitions() const;

	/* empty = all */
	bool save( const std::string& path, const std::vector<SyntaxDefinition>& def = {} );

  protected:
	SyntaxDefinitionManager();

	std::vector<SyntaxDefinition> mDefinitions;

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

	void addxit();

	void addNelua();

	void addTeal();

	void addVue();

	void addElixir();

	void addCrystal();

	void addV();

	std::optional<size_t> getLanguageIndex( const std::string& langName );

};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
