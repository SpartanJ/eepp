#include "spreadsheet.hpp"
#include "parser.hpp"

void Observable::notifyObservers() {
	if ( !mChanged )
		return;
	for ( auto observer : mObservers )
		observer->update();
	clearChanged();
}

void Cell::parseFormula( const std::string& formulaStr ) {
	FormulaParser parser;
	formula = parser.parseFormula( formulaStr );
}

Cell::Cell( std::string val, Spreadsheet& sheet ) : value( std::move( val ) ), sheet( sheet ) {}

void Cell::setData( std::string&& data ) {
	if ( formula ) {
		for ( const auto& ref : formula->getReferences( sheet ) )
			ref->deleteObserver( this );
	}
	value = std::move( data );
	parseFormula( value );
	calc();
	if ( formula ) {
		for ( const auto& ref : formula->getReferences( sheet ) )
			ref->addObserver( this );
	}
	setChanged();
	notifyObservers();
}

std::optional<double> Cell::eval() const {
	double val;
	if ( String::fromString( val, displayValue ) )
		return val;
	return {};
}

const std::string& Cell::getValue() const {
	return value;
}

const std::string& Cell::getDisplayValue() const {
	return displayValue;
}

void Cell::calc() {
	if ( !formula || formula->type() == FormulaType::Textual ) {
		displayValue = value;
		return;
	}
	auto res = formula->eval( sheet );
	if ( res )
		displayValue = String::fromDouble( *res );
	else
		displayValue = "!ERR";
}

void Cell::update() {
	calc();
}

Spreadsheet::Spreadsheet( int cols, int rows ) :
	Model(), mEmptyCell( std::make_unique<Cell>( "", *this ) ) {
	mCells.resize( cols );
	for ( auto& col : mCells )
		col.resize( rows );
}

Variant Spreadsheet::data( const ModelIndex& index, ModelRole role ) const {
	static const std::string EMPTY = "";
	switch ( role ) {
		case EE::UI::Models::ModelRole::Display:
			if ( nullptr == mCells[index.column()][index.row()] )
				return Variant( EMPTY.c_str() );
			return Variant( cell( index ).getDisplayValue().c_str() );
		case EE::UI::Models::ModelRole::Custom:
			if ( nullptr != mCells[index.column()][index.row()] )
				return Variant( cell( index ).getValue().c_str() );
		default:
			break;
	}
	return {};
}

void Spreadsheet::createCell( int col, int row ) {
	if ( mCells[col][row] == nullptr )
		mCells[col][row] = std::make_unique<Cell>( "", *this );
}

Cell& Spreadsheet::cell( int col, int row ) {
	createCell( col, row );
	return mCells[col][row] ? *mCells[col][row] : *mEmptyCell;
}

Cell& Spreadsheet::cell( const ModelIndex& index ) {
	return mCells[index.column()][index.row()] ? *mCells[index.column()][index.row()] : *mEmptyCell;
}

const Cell& Spreadsheet::cell( const ModelIndex& index ) const {
	return *mCells[index.column()][index.row()];
}

void Spreadsheet::setData( const ModelIndex& index, const Variant& data ) {
	createCell( index.column(), index.row() );
	cell( index ).setData( data.toString() );
}
