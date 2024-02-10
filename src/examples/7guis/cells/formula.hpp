#ifndef FORMULA_HPP
#define FORMULA_HPP

#include "spreadsheet.hpp"

enum class FormulaType { Textual, Number, CellReference, RangeReference, Application };

struct Formula {
	virtual FormulaType type() const = 0;
	virtual std::optional<double> eval( Spreadsheet& ) const { return {}; }
	virtual std::vector<Cell*> getReferences( Spreadsheet& ) const { return {}; }
};

struct Textual : public Formula {
	Textual() {}
	Textual( std::string val ) : value( std::move( val ) ) {}
	virtual ~Textual() {}
	std::string value;
	FormulaType type() const { return FormulaType::Textual; }
};

struct Number : public Formula {
	Number( double v ) : value( v ) {}
	virtual ~Number() {}
	double value;
	FormulaType type() const { return FormulaType::Number; }
	std::optional<double> eval( Spreadsheet& ) const { return value; }
};

struct CellReference : public Formula {
	CellReference( int c, int r ) : column( c ), row( r ) {}
	virtual ~CellReference() {}
	int column;
	int row;
	FormulaType type() const { return FormulaType::CellReference; }
	std::optional<double> eval( Spreadsheet& env ) const {
		double val;
		if ( String::fromString( val, env.cell( column, row ).getDisplayValue() ) )
			return val;
		return {};
	}
	std::vector<Cell*> getReferences( Spreadsheet& env ) const {
		if ( column < env.columnCount() && row < env.rowCount() )
			return { &env.cell( column, row ) };
		return {};
	}
};

struct RangeReference : public Formula {
	RangeReference( std::shared_ptr<CellReference> start, std::shared_ptr<CellReference> end ) :
		start( start ), end( end ) {}
	virtual ~RangeReference() {}
	std::shared_ptr<CellReference> start;
	std::shared_ptr<CellReference> end;
	FormulaType type() const { return FormulaType::RangeReference; }
	std::optional<double> eval( Spreadsheet& env ) const { return std::nullopt; }
	std::vector<Cell*> getReferences( Spreadsheet& env ) const {
		std::vector<Cell*> result;
		auto rowCount = env.rowCount();
		auto colCount = env.columnCount();
		for ( int r = start->row; r <= end->row; r++ ) {
			if ( r >= rowCount )
				break;
			for ( int c = start->column; c <= end->column; c++ ) {
				if ( c >= colCount )
					break;
				result.push_back( &env.cell( c, r ) );
			}
		}
		return result;
	}
};

class SheetFunction : public Formula {
  public:
	using Op = std::function<double( const std::vector<double>& )>;

	SheetFunction( std::string function, std::vector<std::shared_ptr<Formula>> arguments ) :
		function( std::move( function ) ),
		functionId( String::hash( String::toUpper( this->function ) ) ),
		arguments( std::move( arguments ) ) {}

	FormulaType type() const { return FormulaType::Application; }

	virtual ~SheetFunction() {}

	std::optional<double> eval( Spreadsheet& env ) const;

	std::vector<Cell*> getReferences( Spreadsheet& env ) const;

  protected:
	static std::unordered_map<String::HashType, Op> opTable;
	static void initOpTable();
	static std::vector<double> evalList( const std::vector<std::shared_ptr<Formula>>& args,
										 Spreadsheet& env );
	std::string function;
	String::HashType functionId;
	std::vector<std::shared_ptr<Formula>> arguments;
};

#endif // FORMULA_HPP
