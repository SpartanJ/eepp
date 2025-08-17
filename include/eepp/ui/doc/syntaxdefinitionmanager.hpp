#ifndef EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
#define EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP

#include <eepp/config.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/ui/doc/hextlanguagetype.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <optional>
#include <vector>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

class EE_API SyntaxDefinitionManager {
	SINGLETON_DECLARE_HEADERS( SyntaxDefinitionManager )
  public:
	static SyntaxDefinitionManager* createSingleton( std::size_t reserveSpaceForLanguages );

	static std::pair<std::string, std::string> toCPP( const SyntaxDefinition& def );

	std::size_t count() const;

	SyntaxPreDefinition& addPreDefinition( SyntaxPreDefinition&& preDefinition );

	SyntaxDefinition& add( SyntaxDefinition&& syntaxStyle );

	const SyntaxDefinition& getPlainDefinition() const;

	std::vector<const SyntaxDefinition*>
	languagesThatSupportExtension( std::string extension ) const;

	bool extensionCanRepresentManyLanguages( std::string extension ) const;

	const SyntaxDefinition& getByExtension( const std::string& filePath ) const;

	const SyntaxDefinition&
	getByHeader( const std::string& header, const std::string& filePath = "",
				 HExtLanguageType hLangType = HExtLanguageType::AutoDetect ) const;

	const SyntaxDefinition& find( const std::string& filePath, const std::string& header,
								  HExtLanguageType hLangType = HExtLanguageType::AutoDetect );

	const SyntaxDefinition& findFromString( const std::string_view& str ) const;

	SyntaxDefinition& getByExtensionRef( const std::string& filePath );

	const SyntaxDefinition& getByLanguageName( const std::string_view& name ) const;

	const SyntaxDefinition& getByLanguageIndex( const Uint32& index ) const;

	const SyntaxDefinition& getByLanguageNameInsensitive( const std::string_view& name ) const;

	const SyntaxDefinition& getByLSPName( const std::string_view& name ) const;

	std::vector<std::string> getLanguageNames() const;

	std::vector<std::string> getExtensionsPatternsSupported() const;

	const SyntaxDefinition* getPtrByLSPName( const std::string& name ) const;

	bool loadFromStream( IOStream& stream, std::vector<std::string>* addedLangs );

	bool loadFromStream( IOStream& stream );

	bool loadFromFile( const std::string& fpath );

	bool loadFromMemory( const Uint8* data, const Uint32& dataSize );

	bool loadFromPack( Pack* Pack, const std::string& filePackPath );

	void loadFromFolder( const std::string& folderPath );

	const std::vector<std::shared_ptr<SyntaxDefinition>>& getDefinitions() const;

	const std::vector<SyntaxPreDefinition>& getPreDefinitions() const;

	/* empty = all */
	bool save( const std::string& path, const std::vector<SyntaxDefinition>& def = {} );

	void setLanguageExtensionsPriority( const std::map<std::string, std::string>& priorities );

	const std::map<std::string, std::string>& getLanguageExtensionsPriority();

  protected:
	SyntaxDefinitionManager( std::size_t reserveSpaceForLanguages = 12 );

	std::vector<std::shared_ptr<SyntaxDefinition>> mDefinitions;
	std::vector<SyntaxPreDefinition> mPreDefinitions;
	std::map<std::string, std::string> mPriorities;
	mutable Mutex mMutex;

	std::optional<size_t> getLanguageIndex( const std::string& langName );

	const SyntaxDefinition* needsHFallback( HExtLanguageType langType, const std::string& lspName,
											const std::string& ext,
											const std::string& buffer ) const;
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
