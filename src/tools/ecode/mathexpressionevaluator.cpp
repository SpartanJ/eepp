#include "mathexpressionevaluator.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <eepp/core/string.hpp>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string_view>
#include <tinyexpr/tinyexpr.h>
#include <vector>

namespace ecode {

namespace {

static constexpr size_t MAX_EXPRESSION_LENGTH = 2048;
static constexpr double PI = 3.141592653589793238462643383279502884;
static constexpr double E = 2.718281828459045235360287471352662498;
static constexpr double TAU = 6.283185307179586476925286766559005768;
static constexpr double PHI = 1.618033988749894848204586834365638118;
static constexpr double SQRT2 = 1.414213562373095048801688724209698079;
static constexpr double LN2 = 0.693147180559945309417232121458176568;
static constexpr double LN10 = 2.302585092994045684017991454684364208;
static constexpr double LOG2E = 1.442695040888963407359924681001892137;
static constexpr double LOG10E = 0.434294481903251827651128918916605082;

enum class TokenKind { None, Number, Identifier, OpenParen, CloseParen, Operator, Comma };

struct Assignment {
	std::string variableName;
	std::string expression;
};

std::string trim( const std::string& str ) {
	const auto first = std::find_if_not( str.begin(), str.end(),
										 []( unsigned char c ) { return std::isspace( c ); } );
	if ( first == str.end() )
		return {};

	const auto last = std::find_if_not( str.rbegin(), str.rend(), []( unsigned char c ) {
						  return std::isspace( c );
					  } ).base();

	return std::string( first, last );
}

bool isValidVariableName( const std::string& name ) {
	if ( name.empty() || !std::isalpha( static_cast<unsigned char>( name[0] ) ) )
		return false;

	return std::all_of( name.begin() + 1, name.end(), []( unsigned char c ) {
		return std::isalpha( c ) || std::isdigit( c ) || c == '_';
	} );
}

bool tokenCanEndOperand( TokenKind kind ) {
	return kind == TokenKind::Number || kind == TokenKind::Identifier ||
		   kind == TokenKind::CloseParen;
}

bool tokenCanStartOperand( TokenKind kind ) {
	return kind == TokenKind::Number || kind == TokenKind::Identifier ||
		   kind == TokenKind::OpenParen;
}

bool isUnaryFunction( const std::string& identifier ) {
	static constexpr std::array<std::string_view, 18> unaryFunctions{
		"abs",	 "acos", "asin", "atan",  "ceil", "cos",  "cosh", "exp", "fac",
		"floor", "ln",	 "log",	 "log10", "sin",  "sinh", "sqrt", "tan", "tanh",
	};
	return std::find( unaryFunctions.begin(), unaryFunctions.end(), identifier ) !=
		   unaryFunctions.end();
}

bool needsImplicitMultiplication( TokenKind previousKind, TokenKind currentKind,
								  const std::string& previousIdentifier ) {
	if ( !tokenCanEndOperand( previousKind ) || !tokenCanStartOperand( currentKind ) )
		return false;

	if ( previousKind == TokenKind::Identifier ) {
		if ( currentKind == TokenKind::OpenParen )
			return false;
		if ( isUnaryFunction( previousIdentifier ) )
			return false;
	}

	return true;
}

std::string normalizeExpression( const std::string& expression ) {
	std::string normalized;
	normalized.reserve( expression.size() + 8 );
	TokenKind previousKind = TokenKind::None;
	std::string previousIdentifier;

	const auto appendToken = [&]( TokenKind kind, const std::string& token ) {
		if ( needsImplicitMultiplication( previousKind, kind, previousIdentifier ) )
			normalized += '*';
		normalized += token;
		previousKind = kind;
		previousIdentifier = kind == TokenKind::Identifier ? EE::String::toLower( token ) : "";
	};

	for ( size_t i = 0; i < expression.size(); ) {
		const unsigned char cur = static_cast<unsigned char>( expression[i] );

		if ( std::isspace( cur ) ) {
			normalized += expression[i++];
			continue;
		}

		if ( std::isdigit( cur ) || expression[i] == '.' ) {
			const size_t start = i;
			bool hasDot = expression[i] == '.';
			i++;

			while ( i < expression.size() ) {
				const unsigned char numCur = static_cast<unsigned char>( expression[i] );
				if ( std::isdigit( numCur ) ) {
					i++;
				} else if ( expression[i] == '.' && !hasDot ) {
					hasDot = true;
					i++;
				} else {
					break;
				}
			}

			if ( i < expression.size() && ( expression[i] == 'e' || expression[i] == 'E' ) ) {
				size_t exponent = i + 1;
				if ( exponent < expression.size() &&
					 ( expression[exponent] == '+' || expression[exponent] == '-' ) )
					exponent++;
				if ( exponent < expression.size() &&
					 std::isdigit( static_cast<unsigned char>( expression[exponent] ) ) ) {
					i = exponent + 1;
					while ( i < expression.size() &&
							std::isdigit( static_cast<unsigned char>( expression[i] ) ) )
						i++;
				}
			}

			appendToken( TokenKind::Number, expression.substr( start, i - start ) );
			continue;
		}

		if ( std::isalpha( cur ) ) {
			const size_t start = i++;
			while ( i < expression.size() &&
					( std::isalpha( static_cast<unsigned char>( expression[i] ) ) ||
					  std::isdigit( static_cast<unsigned char>( expression[i] ) ) ||
					  expression[i] == '_' ) )
				i++;
			appendToken( TokenKind::Identifier, expression.substr( start, i - start ) );
			continue;
		}

		switch ( expression[i] ) {
			case '(':
				appendToken( TokenKind::OpenParen, "(" );
				break;
			case ')':
				normalized += expression[i];
				previousKind = TokenKind::CloseParen;
				previousIdentifier.clear();
				break;
			case ',':
				normalized += expression[i];
				previousKind = TokenKind::Comma;
				previousIdentifier.clear();
				break;
			default:
				normalized += expression[i];
				previousKind = TokenKind::Operator;
				previousIdentifier.clear();
				break;
		}
		i++;
	}

	return normalized;
}

std::optional<Assignment> parseAssignment( const std::string& expression ) {
	const size_t separator = expression.find( '=' );
	if ( separator == std::string::npos )
		return {};

	Assignment assignment{ trim( expression.substr( 0, separator ) ),
						   trim( expression.substr( separator + 1 ) ) };
	if ( !isValidVariableName( assignment.variableName ) || assignment.expression.empty() )
		return Assignment{};

	return assignment;
}

std::string formatResult( double result ) {
	if ( std::isnan( result ) )
		return "nan";
	if ( std::isinf( result ) )
		return result > 0 ? "inf" : "-inf";

	std::ostringstream ss;
	ss << std::setprecision( 15 ) << result;
	std::string formatted( ss.str() );
	EE::String::numberCleanInPlace( formatted );
	return formatted;
}

MathExpressionEvaluator::Result
evaluateNormalizedExpression( const std::string& expression,
							  const MathExpressionEvaluator::Variables& variables ) {
	MathExpressionEvaluator::Result eval;
	const std::string normalizedExpression( normalizeExpression( expression ) );

	std::vector<te_variable> bindings;
	bindings.reserve( variables.size() );
	for ( const auto& variable : variables )
		bindings.push_back( { variable.first.c_str(), &variable.second, TE_VARIABLE, nullptr } );

	int error = 0;
	te_expr* compiled = te_compile( normalizedExpression.c_str(), bindings.data(),
									static_cast<int>( bindings.size() ), &error );

	if ( !compiled ) {
		eval.value = "Invalid expression";
		eval.detail = "Error at character " + EE::String::toString( error );
		return eval;
	}

	const double result = te_eval( compiled );
	te_free( compiled );

	if ( std::isnan( result ) ) {
		eval.value = "Invalid expression";
		eval.detail = "Result is not a number";
		return eval;
	}

	eval.success = true;
	eval.number = result;
	eval.value = formatResult( result );
	eval.detail = normalizedExpression == expression ? expression : normalizedExpression;
	return eval;
}

MathExpressionEvaluator::Result evaluateImpl( const std::string& expression,
											  const MathExpressionEvaluator::Variables& variables,
											  MathExpressionEvaluator::Variables* variableTarget ) {
	MathExpressionEvaluator::Result eval;
	const std::string cleanExpression( trim( expression ) );
	if ( cleanExpression.empty() )
		return eval;

	if ( cleanExpression.size() > MAX_EXPRESSION_LENGTH ) {
		eval.value = "Invalid expression";
		eval.detail = "Expression is too long";
		return eval;
	}

	if ( auto assignment = parseAssignment( cleanExpression ) ) {
		if ( assignment->variableName.empty() ) {
			eval.value = "Invalid expression";
			eval.detail = "Invalid variable assignment";
			return eval;
		}

		eval = evaluateNormalizedExpression( assignment->expression, variables );
		if ( !eval.success )
			return eval;

		eval.assignment = true;
		eval.variableName = assignment->variableName;
		eval.detail = assignment->variableName + " = " + eval.detail;
		eval.value = assignment->variableName + " = " + eval.value;
		if ( variableTarget )
			( *variableTarget )[assignment->variableName] = eval.number;
		return eval;
	}

	return evaluateNormalizedExpression( cleanExpression, variables );
}

} // namespace

MathExpressionEvaluator::Result MathExpressionEvaluator::evaluate( const std::string& expression ) {
	static const Variables defaultVars = defaultVariables();
	return evaluate( expression, defaultVars );
}

MathExpressionEvaluator::Result MathExpressionEvaluator::evaluate( const std::string& expression,
																   const Variables& variables ) {
	return evaluateImpl( expression, variables, nullptr );
}

MathExpressionEvaluator::Result
MathExpressionEvaluator::evaluateAndAssign( const std::string& expression, Variables& variables ) {
	return evaluateImpl( expression, variables, &variables );
}

MathExpressionEvaluator::Variables MathExpressionEvaluator::defaultVariables() {
	return {
		{ "pi", PI },		  { "PI", PI },			{ "e", E },			{ "E", E },
		{ "tau", TAU },		  { "TAU", TAU },		{ "phi", PHI },		{ "PHI", PHI },
		{ "sqrt2", SQRT2 },	  { "SQRT2", SQRT2 },	{ "ln2", LN2 },		{ "LN2", LN2 },
		{ "ln10", LN10 },	  { "LN10", LN10 },		{ "log2e", LOG2E }, { "LOG2E", LOG2E },
		{ "log10e", LOG10E }, { "LOG10E", LOG10E },
	};
}

} // namespace ecode
