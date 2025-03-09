#ifndef EE_SYSTEM_PATTERNMATCHER_HPP
#define EE_SYSTEM_PATTERNMATCHER_HPP

#include <eepp/config.hpp>
#include <string>
#include <string_view>

namespace EE { namespace System {

// Inpired in rx-cpp (https://github.com/stevedonovan/rx-cpp/).

class EE_API PatternMatcher {
  public:
	enum class PatternType { LuaPattern, PCRE };

	struct EE_API Range {
		int start{ -1 };
		int end{ -1 };
		bool isValid() { return -1 != start && -1 != end; }
	};

	class EE_API State {
	  public:
		State( PatternMatcher* pattern, bool ownPattern );

		~State();

		bool range( int index, int& start, int& end );

		bool matches( const char* string, size_t length );

		PatternMatcher* mPattern;
		Range* mRanges;
		size_t mRefCount;
		bool mOwnPattern;
	};

	class EE_API Match {
	  public:
		~Match();

		Match( PatternMatcher& r, const char* string, bool ownPattern = false );

		Match( PatternMatcher& r, const std::string& string, bool ownPattern = false );

		Match( const PatternMatcher::Match& other );

		Match& operator=( const Match& other );

		bool matches();

		bool subst( std::string& res );

		void next();

		std::string group( int idx = -1 ) const;

		std::string_view groupView( int idx = -1 ) const;

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
		PatternMatcher::State* mState{ nullptr };
		const char* mString{ nullptr };
		size_t mLength{ 0 };
	};

	PatternMatcher( PatternType type ) : mType( type ) {}

	virtual ~PatternMatcher() {}

	PatternType getType() const { return mType; }

	PatternMatcher::Match gmatch( const char* s ) &;

	PatternMatcher::Match gmatch( const char* s ) &&;

	PatternMatcher::Match gmatch( const std::string& string ) &&;

	PatternMatcher::Match gmatch( const std::string& string ) &;

	bool range( int indexGet, int& startMatch, int& endMatch,
				PatternMatcher::Range* returnedMatched ) const;

	bool find( const std::string& s, int& startMatch, int& endMatch, int offset = 0,
			   int returnedMatchIndex = 0 ) const;

	bool find( const char* stringSearch, int& startMatch, int& endMatch, int stringStartOffset = 0,
			   int stringLength = 0, int returnMatchIndex = 0 ) const;

	bool find( const std::string& s, int& startMatch, int& endMatch, int offset,
			   int returnedMatchIndex, PatternMatcher::Range* matchesBuffer ) const;

	bool find( const char* stringSearch, int& startMatch, int& endMatch, int stringStartOffset,
			   int stringLength, int returnMatchIndex, PatternMatcher::Range* matchesBuffer ) const;

	std::string gsub( const char* text, const char* replace );

	std::string gsub( const std::string& text, const std::string& replace );

	virtual const std::string_view& getPattern() const = 0;

	virtual bool matches( const char* stringSearch, int stringStartOffset,
						  PatternMatcher::Range* matchList, size_t stringLength ) const = 0;

	virtual bool matches( const std::string& str, PatternMatcher::Range* matchList = nullptr,
						  int stringStartOffset = 0 ) const = 0;

	virtual const size_t& getNumMatches() const = 0;

	virtual bool isValid() const = 0;

  protected:
	PatternType mType;
};

}} // namespace EE::System

#endif
