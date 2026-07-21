#ifndef ECODE_MATHEXPRESSIONEVALUATOR_HPP
#define ECODE_MATHEXPRESSIONEVALUATOR_HPP

#include <string>
#include <unordered_map>

namespace ecode {

class MathExpressionEvaluator {
  public:
	using Variables = std::unordered_map<std::string, double>;

	struct Result {
		bool success{ false };
		bool assignment{ false };
		double number{ 0 };
		std::string value;
		std::string detail;
		std::string variableName;
	};

	static Result evaluate( const std::string& expression );
	static Result evaluate( const std::string& expression, const Variables& variables );
	static Result evaluateAndAssign( const std::string& expression, Variables& variables );
	static Variables defaultVariables();
};

} // namespace ecode

#endif // ECODE_MATHEXPRESSIONEVALUATOR_HPP
