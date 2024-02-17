#include "formula.hpp"

std::unordered_map<String::HashType, SheetFunction::Op> SheetFunction::opTable =
	std::unordered_map<String::HashType, SheetFunction::Op>();

void SheetFunction::initOpTable() {
	if ( !opTable.empty() )
		return;
	opTable.insert(
		{ String::hash( "ADD" ), []( const std::vector<double>& vals ) -> std::optional<double> {
			 if ( vals.size() == 2 )
				 return vals[0] + vals[1];
			 return std::nullopt;
		 } } );
	opTable.insert(
		{ String::hash( "SUB" ), []( const std::vector<double>& vals ) -> std::optional<double> {
			 if ( vals.size() == 2 )
				 return vals[0] - vals[1];
			 return std::nullopt;
		 } } );
	opTable.insert(
		{ String::hash( "DIV" ), []( const std::vector<double>& vals ) -> std::optional<double> {
			 if ( vals.size() == 2 && vals[1] != 0 )
				 return vals[0] / vals[1];
			 return std::nullopt;
		 } } );
	opTable.insert(
		{ String::hash( "MUL" ), []( const std::vector<double>& vals ) -> std::optional<double> {
			 if ( vals.size() == 2 )
				 return vals[0] * vals[1];
			 return std::nullopt;
		 } } );
	opTable.insert(
		{ String::hash( "MOD" ), []( const std::vector<double>& vals ) -> std::optional<double> {
			 if ( vals.size() == 2 )
				 return std::fmod( vals[0], vals[1] );
			 return std::nullopt;
		 } } );
	opTable.insert( { String::hash( "SUM" ), []( const std::vector<double>& vals ) {
						 double tot = 0;
						 for ( const auto& val : vals )
							 tot += val;
						 return tot;
					 } } );
	opTable.insert( { String::hash( "PROD" ), []( const std::vector<double>& vals ) {
						 double tot = 0;
						 for ( const auto& val : vals )
							 tot *= val;
						 return tot;
					 } } );
	opTable.insert( { String::hash( "AVG" ), []( const std::vector<double>& vals ) {
						 double tot = 0;
						 for ( const auto& val : vals )
							 tot += val;
						 return vals.empty() ? 0 : tot / vals.size();
					 } } );
}

std::vector<double> SheetFunction::evalList( const std::vector<std::shared_ptr<Formula>>& args,
											 Spreadsheet& env ) {
	std::vector<double> result;
	for ( const auto& f : args ) {
		if ( !f )
			return result;
		if ( f->type() == FormulaType::RangeReference ) {
			auto refs( f->getReferences( env ) );
			for ( Cell* c : refs ) {
				auto res( c->eval() );
				if ( res )
					result.push_back( *res );
			}
		} else {
			auto res( f->eval( env ) );
			if ( res )
				result.push_back( *res );
		}
	}
	return result;
}

std::optional<double> SheetFunction::eval( Spreadsheet& env ) const {
	initOpTable();
	std::vector<double> argvals = evalList( arguments, env );
	auto func = opTable.find( functionId );
	if ( func != opTable.end() )
		return func->second( argvals );
	return {};
}

std::vector<Cell*> SheetFunction::getReferences( Spreadsheet& env ) const {
	std::vector<Cell*> result;
	for ( const auto& argument : arguments ) {
		if ( !argument )
			continue;
		auto refs( argument->getReferences( env ) );
		result.insert( result.end(), refs.begin(), refs.end() );
	}
	return result;
}
