#ifndef SPREADSHEET_HPP
#define SPREADSHEET_HPP

#include <eepp/core/string.hpp>
#include <eepp/ui/models/model.hpp>

#include <atomic>
#include <optional>

using namespace EE;
using namespace EE::UI::Models;
struct Formula;
class Spreadsheet;

class Observer;

class Observable {
  public:
	void addObserver( Observer* observer ) { mObservers.insert( observer ); }

	void clearChanged() { mChanged = false; }

	void setChanged() { mChanged = true; }

	int countObservers() { return mObservers.size(); }

	void deleteObserver( Observer* observer ) { mObservers.erase( observer ); }

	void deleteObservers() { mObservers.clear(); }

	bool hasChanged() { return mChanged; }

	void notifyObservers();

  protected:
	std::unordered_set<Observer*> mObservers;
	std::atomic<bool> mChanged{ false };
};

class Observer {
  public:
	virtual void update() = 0;
};

class Cell : public Observer, public Observable {
  public:
	Cell( std::string val, Spreadsheet& sheet );

	virtual ~Cell() {}

	void setData( std::string&& data );

	std::optional<double> eval() const;

	const std::string& getValue() const;

	const std::string& getDisplayValue() const;

	void calc();

	void update();

  protected:
	std::string value;
	std::string displayValue;
	std::shared_ptr<Formula> formula;
	Spreadsheet& sheet;

	void parseFormula( const std::string& formulaStr );
};

class Spreadsheet : public Model {
  public:
	Spreadsheet( int cols = 26, int rows = 100 );

	virtual ~Spreadsheet() {}

	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const { return mCells[0].size(); }

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const { return mCells.size(); }

	virtual std::string columnName( const size_t& column ) const {
		return String::format( "%c", column + 'A' );
	}

	virtual Variant data( const ModelIndex& index, ModelRole role ) const;

	void createCell( int col, int row );

	Cell& cell( int col, int row );

	Cell& cell( const ModelIndex& index );

	const Cell& cell( const ModelIndex& index ) const;

	virtual void setData( const ModelIndex& index, const Variant& data );

	virtual bool isEditable( const ModelIndex& ) const { return true; }

  protected:
	std::vector<std::vector<std::unique_ptr<Cell>>> mCells;
	std::unique_ptr<Cell> mEmptyCell;
};

#endif // SPREADSHEET_HPP
