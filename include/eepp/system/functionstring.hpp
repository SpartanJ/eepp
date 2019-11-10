#ifndef EE_SYSTEM_FUNCTIONSTRING_HPP
#define EE_SYSTEM_FUNCTIONSTRING_HPP

#include <eepp/config.hpp>
#include <string>
#include <vector>

namespace EE { namespace System {

class EE_API FunctionString {
	public:
		static FunctionString parse( const std::string& function );

		FunctionString( const std::string& name, const std::vector<std::string>& parameters );

		const std::string& getName() const;

		const std::vector<std::string>& getParameters() const;

		bool isEmpty() const;
	protected:
		std::string name;

		std::vector<std::string> parameters;
};

}}

#endif
