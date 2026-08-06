#ifndef EE_SYSTEM_FUNCTIONSTRING_HPP
#define EE_SYSTEM_FUNCTIONSTRING_HPP

#include <eepp/config.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>

#include <string>
#include <utility>

namespace EE { namespace System {

class EE_API FunctionString {
  public:
	using Parameters = SmallVector<std::string, 4>;
	using TypeStringVector = SmallVector<bool, 4>;

	class View {
	  public:
		View() = default;

		std::string_view getName() const { return mName; }

		bool isEmpty() const { return mName.empty(); }

		template <typename Callback> bool forEachParameter( Callback&& callback ) const {
			return FunctionString::forEachParameter( mParameters,
													 std::forward<Callback>( callback ) );
		}

	  private:
		friend class FunctionString;

		View( std::string_view name, std::string_view parameters ) :
			mName( name ), mParameters( parameters ) {}

		std::string_view mName;
		std::string_view mParameters;
	};

	static View parseView( std::string_view function );

	/** Iterates a comma-separated parameter list without allocating. Nested parentheses and
	 * quoted strings protect commas. Quoted parameters are reported without their outer quotes;
	 * escaped quotes remain escaped because the returned views reference the original input.
	 * The callback receives (parameter, parameterWasString) and returns whether to continue. */
	template <typename Callback>
	static bool forEachParameter( std::string_view parameters, Callback&& callback ) {
		std::size_t start = 0;
		int parenthesisDepth = 0;
		char quote = 0;
		bool escaped = false;

		auto emit = [&]( std::size_t end ) {
			std::string_view parameter =
				String::trim( parameters.substr( start, end - start ), " \t\n\r\f\v" );
			bool wasString = parameter.size() >= 2 &&
							 ( parameter.front() == '\'' || parameter.front() == '"' ) &&
							 parameter.back() == parameter.front();
			if ( wasString )
				parameter = parameter.substr( 1, parameter.size() - 2 );
			if ( parameter.empty() && !wasString )
				return true;
			return callback( parameter, wasString );
		};

		for ( std::size_t i = 0; i < parameters.size(); ++i ) {
			const char character = parameters[i];
			if ( quote ) {
				if ( escaped ) {
					escaped = false;
				} else if ( character == '\\' ) {
					escaped = true;
				} else if ( character == quote ) {
					quote = 0;
				}
				continue;
			}
			if ( character == '\'' || character == '"' ) {
				quote = character;
			} else if ( character == '(' ) {
				++parenthesisDepth;
			} else if ( character == ')' && parenthesisDepth > 0 ) {
				--parenthesisDepth;
			} else if ( character == ',' && parenthesisDepth == 0 ) {
				if ( !emit( i ) )
					return false;
				start = i + 1;
			}
		}
		return emit( parameters.size() );
	}

	static FunctionString parse( const std::string& function );

	static FunctionString parse( std::string_view function );

	static FunctionString parse( String::View function );

	FunctionString() {}

	FunctionString( const std::string& name, const Parameters& parameters,
					const TypeStringVector& typeStringData );

	FunctionString( const std::string& name, Parameters&& parameters,
					TypeStringVector&& typeStringData );

	const std::string& getName() const;

	const Parameters& getParameters() const;

	bool parameterWasString( Uint32 index ) const;

	bool isEmpty() const;

  protected:
	std::string name;
	Parameters parameters;
	TypeStringVector typeStringData;

	static FunctionString parseOwned( std::string_view function );
};

}} // namespace EE::System

#endif
