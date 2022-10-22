#ifndef ECODE_IGNOREMATCHER_HPP
#define ECODE_IGNOREMATCHER_HPP

#include <eepp/system/filesystem.hpp>
#include <memory>
#include <string>
#include <vector>

using namespace EE;
using namespace EE::System;

namespace ecode {

class IgnoreMatcher {
  public:
	IgnoreMatcher( const std::string& rootPath );

	virtual ~IgnoreMatcher();

	virtual bool canMatch() = 0;

	virtual bool match( const std::string& value ) const = 0;

	virtual std::string findRepositoryRootPath() const = 0;

	const std::string& getPath() const { return mPath; }

  protected:
	std::string mPath;

	virtual bool parse() = 0;
};

class GitIgnoreMatcher : public IgnoreMatcher {
  public:
	GitIgnoreMatcher( const std::string& rootPath );

	bool canMatch() override;

	bool match( const std::string& value ) const override;

	std::string findRepositoryRootPath() const override;

  protected:
	bool parse() override;

	std::vector<std::pair<std::string, bool>> mPatterns;
};

class IgnoreMatcherManager {
  public:
	IgnoreMatcherManager( IgnoreMatcherManager&& ignoreMatcher );

	IgnoreMatcherManager( std::string rootPath );

	IgnoreMatcherManager& operator=( IgnoreMatcherManager&& other );

	virtual ~IgnoreMatcherManager();

	bool foundMatch() const;

	bool match( const std::string& dir, const std::string& value ) const;

	std::string findRepositoryRootPath() const;

	const std::string& getPath() const;

	const std::string& getRootPath() const;

	void addChild( IgnoreMatcher* child );

	void removeChild( IgnoreMatcher* child );

	size_t matchersCount() { return mMatchers.size(); }

	IgnoreMatcher* popMatcher( size_t index );

	const std::vector<IgnoreMatcher*>& getMatchers() const;

  protected:
	std::string mRootPath;
	std::vector<IgnoreMatcher*> mMatchers;


};

} // namespace ecode

#endif // ECODE_IGNOREMATCHER_HPP
