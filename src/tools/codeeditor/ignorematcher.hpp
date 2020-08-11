#ifndef EE_TOOLS_IGNOREMATCHER_HPP
#define EE_TOOLS_IGNOREMATCHER_HPP

#include <eepp/system/filesystem.hpp>
#include <string>
#include <vector>

using namespace EE;
using namespace EE::System;

class IgnoreMatcher {
  public:
	IgnoreMatcher( const std::string& rootPath );

	virtual ~IgnoreMatcher();

	virtual bool canMatch() = 0;

	virtual bool match( const std::string& value ) = 0;

  protected:
	std::string mPath;

	virtual bool parse() = 0;
};

class GitIgnoreMatcher : public IgnoreMatcher {
  public:
	GitIgnoreMatcher( const std::string& rootPath );

	bool canMatch() override;

	bool match( const std::string& value ) override;

  protected:
	bool parse() override;

	std::vector<std::pair<std::string, bool>> mPatterns;
	bool mHasNegates{false};
};

class IgnoreMatcherManager {
  public:
	IgnoreMatcherManager( std::string rootPath );

	bool foundMatch() const;

	bool match( const std::string& value );

  protected:
	std::unique_ptr<IgnoreMatcher> mMatcher;
};

#endif // EE_TOOLS_IGNOREMATCHER_HPP
