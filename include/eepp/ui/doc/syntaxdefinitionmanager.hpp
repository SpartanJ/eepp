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
	static std::pair<std::string, std::string> toCPP( const SyntaxDefinition& def );

	SyntaxDefinition& add( SyntaxDefinition&& syntaxStyle );

	const SyntaxDefinition& getPlainDefinition() const;

	std::vector<const SyntaxDefinition*>
	languagesThatSupportExtension( std::string extension ) const;

	bool extensionCanRepresentManyLanguages( std::string extension ) const;

	const SyntaxDefinition& getByExtension( const std::string& filePath,
											bool hFileAsCPP = false ) const;

	const SyntaxDefinition& getByHeader( const std::string& header, bool hFileAsCPP = false ) const;

	const SyntaxDefinition& find( const std::string& filePath, const std::string& header,
								  bool hFileAsCPP = false );

	const SyntaxDefinition& findFromString( const std::string& str ) const;

	SyntaxDefinition& getByExtensionRef( const std::string& filePath );

	const SyntaxDefinition& getByLanguageName( const std::string& name ) const;

	const SyntaxDefinition& getByLanguageIndex( const Uint32& index ) const;

	const SyntaxDefinition& getByLanguageNameInsensitive( std::string name ) const;

	const SyntaxDefinition& getByLSPName( const std::string& name ) const;

	const SyntaxDefinition& getByLanguageId( const String::HashType& id ) const;

	SyntaxDefinition& getByLanguageNameRef( const std::string& name );

	std::vector<std::string> getLanguageNames() const;

	std::vector<std::string> getExtensionsPatternsSupported() const;

	const SyntaxDefinition* getPtrByLanguageName( const std::string& name ) const;

	const SyntaxDefinition* getPtrByLSPName( const std::string& name ) const;

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

	void setLanguageExtensionsPriority( const std::map<std::string, std::string>& priorities );

	const std::map<std::string, std::string>& getLanguageExtensionsPriority() {
		return mPriorities;
	}

  protected:
	SyntaxDefinitionManager();

	std::vector<SyntaxDefinition> mDefinitions;
	std::map<std::string, std::string> mPriorities;

	std::optional<size_t> getLanguageIndex( const std::string& langName );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
