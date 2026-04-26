#ifndef EE_SYSTEM_FUNCTIONSTRING_HPP
#define EE_SYSTEM_FUNCTIONSTRING_HPP

#include <eepp/config.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>

#include <concepts>
#include <string>

template <typename T>
concept AllowedFunctionString =
	std::same_as<T, std::string_view> || std::same_as<T, EE::String::View>;

namespace EE { namespace System {

class EE_API FunctionString {
  public:
	using Parameters = SmallVector<std::string, 4>;
	using TypeStringVector = SmallVector<bool, 4>;

	static FunctionString parse( std::string_view function );

	static FunctionString parse( String::View function );

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

	template <AllowedFunctionString StringType> static FunctionString parse( StringType function );
};

}} // namespace EE::System

#endif
