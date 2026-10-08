#ifndef ECODE_USERSNIPPETSTORE_HPP
#define ECODE_USERSNIPPETSTORE_HPP

#include <eepp/core/containers.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace EE;
using namespace EE::System;

namespace ecode {

enum class UserSnippetSource { User, VSCodeProject, EcodeProject };

struct UserSnippetDefinition {
	std::string name;
	SmallVector<std::string, 2> prefixes;
	std::string body;
	std::string description;
	SmallVector<std::string, 2> scopes;
	SmallVector<std::string, 2> includePatterns;
	SmallVector<std::string, 2> excludePatterns;
	std::string sourcePath;
	UserSnippetSource source{ UserSnippetSource::User };
};

struct UserSnippetParseResult {
	std::vector<UserSnippetDefinition> snippets;
	std::vector<std::string> diagnostics;
	bool valid{ false };
};

struct UserSnippetMatch {
	UserSnippetDefinition snippet;
	std::string matchedPrefix;
	std::string matchedInput;
	int score{ 0 };
};

class UserSnippetStore {
  public:
	static UserSnippetParseResult parseFile( std::string_view contents, std::string sourcePath,
											 UserSnippetSource source,
											 std::string defaultScope = {} );

	bool updateFile( std::string_view contents, std::string sourcePath, UserSnippetSource source,
					 std::string defaultScope = {},
					 std::vector<std::string>* diagnostics = nullptr );

	bool removeFile( std::string_view sourcePath );

	void removeSource( UserSnippetSource source );

	void clear();

	std::vector<UserSnippetMatch> find( std::string_view language, std::string_view pattern,
										size_t maxResults, std::string_view filePath = {} ) const;

	std::vector<UserSnippetMatch> findForLocator( std::string_view language,
												  std::string_view pattern, size_t maxResults,
												  std::string_view filePath = {} ) const;

	size_t size() const;

  private:
	struct SourceFile {
		UserSnippetSource source{ UserSnippetSource::User };
		String::HashType hash{ 0 };
		std::vector<UserSnippetDefinition> snippets;
	};

	struct Snapshot {
		std::vector<UserSnippetDefinition> snippets;
		std::vector<size_t> global;
		UnorderedMap<std::string, std::vector<size_t>> byLanguage;
	};

	mutable Mutex mMutex;
	UnorderedMap<std::string, SourceFile> mFiles;
	std::shared_ptr<const Snapshot> mSnapshot{ std::make_shared<Snapshot>() };

	void rebuildSnapshot();
};

} // namespace ecode

#endif // ECODE_USERSNIPPETSTORE_HPP
