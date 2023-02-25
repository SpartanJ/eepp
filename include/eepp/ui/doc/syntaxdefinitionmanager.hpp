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

	std::optional<size_t> getLanguageIndex( const std::string& langName );

};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
