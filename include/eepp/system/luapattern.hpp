#ifndef EE_SYSTEM_LUAPATTERNMATCHER_HPP
#define EE_SYSTEM_LUAPATTERNMATCHER_HPP

#include <eepp/config.hpp>
#include <string>
#include <vector>

namespace EE { namespace System {

// Adapted from rx-cpp (https://github.com/stevedonovan/rx-cpp/).
// This implementation removes all the regexp related stuffs, only leaves the Lua implementation.
class EE_API LuaPattern {
  public:
	struct EE_API Range {
		int start{-1};
		int end{-1};
		bool isValid() { return -1 != start && -1 != end; }
	};

	class EE_API State {
	  public:
		bool range( int index, int& start, int& end );

		bool matches( const char* string, size_t length );

	  protected:
		friend class LuaPattern;
		LuaPattern* mPattern;
		Range* mRanges;
		size_t mRefCount;
		bool mOwnPattern;

		State( LuaPattern* pattern, bool ownPattern );

		~State();
	};

	class EE_API Match {
	  public:
		~Match();

		Match( LuaPattern& r, const char* string, bool ownPattern = false );

		Match( LuaPattern& r, const std::string& string, bool ownPattern = false );

		Match( const LuaPattern::Match& other );

		Match& operator=( const Match& other );

		bool matches();

		bool subst( std::string& res );

		void next();

		std::string group( int idx = -1 ) const;

		bool range( int idx, int& start, int& end ) const;

		std::string operator[]( int index ) const;

		class iterator {
		  public:
			iterator( Match* pm ) : mMatcher( pm ) {
				if ( mMatcher != nullptr && !mMatcher->matches() )
					mMatcher = nullptr;
			}
			bool operator!=( const iterator& other ) { return mMatcher != other.mMatcher; }
			bool operator==( const iterator& other ) { return mMatcher == other.mMatcher; }
			const Match& operator*() const { return *mMatcher; }
			iterator& operator++() {
				mMatcher->next();
				if ( !mMatcher->matches() )
					mMatcher = nullptr;
				return *this;
			}

		  protected:
			Match* mMatcher;
		};

		iterator begin() { return iterator( this ); }

		iterator end() { return iterator( nullptr ); }

	  protected:
		friend class LuaPattern;
		LuaPattern::State* mState{nullptr};
		const char* mString{nullptr};
		size_t mLength{0};
	};

	static std::string match( const std::string& string, const std::string& pattern );

	static Range find( const std::string& string, const std::string& pattern );

	LuaPattern( const std::string& pattern );

	bool matches( const char* stringSearch, int stringStartOffset, LuaPattern::Range* matchList,
				  size_t stringLength ) const;

	bool matches( const std::string& str, LuaPattern::Range* matchList = nullptr,
				  int stringStartOffset = 0 ) const;

	bool find( const char* stringSearch, int& startMatch, int& endMatch, int stringStartOffset = 0,
			   int stringLength = 0, int returnMatchIndex = 0 ) const;

	bool find( const std::string& s, int& startMatch, int& endMatch, int offset = 0,
			   int returnedMatchIndex = 0 ) const;

	const size_t& getNumMatches() const;

	bool range( int indexGet, int& startMatch, int& endMatch,
				LuaPattern::Range* returnedMatched ) const;

	const std::string& getPatern() const { return mPattern; }

	LuaPattern::Match gmatch( const char* s ) &;

	LuaPattern::Match gmatch( const char* s ) &&;

	LuaPattern::Match gmatch( const std::string& string ) &&;

	LuaPattern::Match gmatch( const std::string& string ) &;

	std::string gsub( const char* text, const char* replace );

	std::string gsub( const std::string& text, const std::string& replace );

  protected:
	mutable std::string mErr;
	std::string mPattern;
	mutable size_t mMatchNum;
};

}} // namespace EE::System

#endif // EE_SYSTEM_LUAPATTERNMATCHER_HPP
