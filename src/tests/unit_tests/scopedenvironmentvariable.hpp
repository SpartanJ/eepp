#ifndef EE_TESTS_SCOPEDENVIRONMENTVARIABLE_HPP
#define EE_TESTS_SCOPEDENVIRONMENTVARIABLE_HPP

#include <cstdlib>
#include <eepp/config.hpp>
#include <string>

/** Sets an environment variable for the current scope. Restores the previous value, or removes
 * the variable when it was not set before. */
class ScopedEnvironmentVariable {
  public:
	ScopedEnvironmentVariable( const char* name, const std::string& value ) : mName( name ) {
		if ( const char* currentValue = std::getenv( name ) ) {
			mHadValue = true;
			mValue = currentValue;
		}
		set( value.c_str() );
	}

	~ScopedEnvironmentVariable() {
		if ( mHadValue )
			set( mValue.c_str() );
		else
			unset();
	}

	ScopedEnvironmentVariable( const ScopedEnvironmentVariable& ) = delete;

	ScopedEnvironmentVariable& operator=( const ScopedEnvironmentVariable& ) = delete;

  private:
	void set( const char* value ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
		_putenv_s( mName, value );
#else
		setenv( mName, value, 1 );
#endif
	}

	void unset() {
#if EE_PLATFORM == EE_PLATFORM_WIN
		// An empty value removes the variable on Windows.
		_putenv_s( mName, "" );
#else
		unsetenv( mName );
#endif
	}

	const char* mName;
	std::string mValue;
	bool mHadValue{ false };
};

#endif
