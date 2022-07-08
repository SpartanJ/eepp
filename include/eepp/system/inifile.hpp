// IniFile.cpp:  Implementation of the IniFile class.
// Written by:   Adam Clauss
// Email: cabadam@tamu.edu
// You may use this class/code as you wish in your programs.  Feel free to distribute it, and
// email suggested changes to me.
//
// Rewritten by: Shane Hill
// Date:         21/08/2001
// Email:        Shane.Hill@dsto.defence.gov.au
// Reason:       Remove dependancy on MFC. Code should compile on any
//               platform. Tested on Windows/Linux/Irix
//
// Spartan: I made some modifications to the class. So, this is not the original.
//////////////////////////////////////////////////////////////////////

#ifndef CINIFILE_H
#define CINIFILE_H

#include <eepp/system/iostream.hpp>
#include <map>
#include <unordered_map>
#include <vector>

#define MAX_KEYNAME 128
#define MAX_VALUENAME 128
#define MAX_VALUEDATA 2048

namespace EE { namespace System {

class Pack;

class EE_API IniFile {
  public:
	enum errors { noID = -1 };

	/** Initialize and load the ini file from path  */
	IniFile( std::string const iniPath = "", bool autoLoad = true );

	/** Initialize and load the ini file from memory  */
	IniFile( const Uint8* RAWData, const Uint32& size, bool autoLoad = true );

	/** Initialize and load the ini file from a pack file  */
	IniFile( Pack* Pack, std::string iniPackPath, bool autoLoad = true );

	/** Initialize and load the ini file from a stream  */
	IniFile( IOStream& stream, bool autoLoad = true );

	virtual ~IniFile() {}

	/** Loads an ini file from path */
	bool loadFromFile( const std::string& iniPath );

	/** Loads an ini file from memory */
	bool loadFromMemory( const Uint8* RAWData, const Uint32& size );

	/** Loads an ini file from a pack file */
	bool loadFromPack( Pack* Pack, std::string iniPackPath );

	/** Loads an ini file from a stream */
	bool loadFromStream( IOStream& stream );

	/** Sets whether or not keynames and valuenames should be case sensitive.
	** The default is case insensitive. */
	void caseSensitive() { mCaseInsensitive = false; }

	/** @see CaseSensitive() */
	void caseInsensitive() { mCaseInsensitive = true; }

	/** Sets mPath of ini file to read and write from. */
	void path( std::string const newPath ) { mPath = newPath; }

	/** @return The ini file path */
	std::string path() const { return mPath; }

	/** Reads ini file specified using mPath.
	 *	@return true if successful, false otherwise. */
	bool readFile();

	/** Writes data stored in class to ini file. */
	bool writeFile();

	/** Deletes all stored ini data. */
	void clear();

	/** @return index of specified key, or noID if not found. */
	long findKey( std::string const keyname ) const;

	/** @return index of specified value, in the specified key, or noID if not found. */
	long findValue( unsigned const keyID, std::string const valuename ) const;

	/** @return number of Keys currently in the ini. */
	unsigned getNumKeys() const { return (unsigned int)mNames.size(); }

	/** Add a key name. */
	unsigned addKeyName( std::string const keyname );

	/** @return key Names by index. */
	std::string getKeyName( unsigned const keyID ) const;

	/** @return number of values stored for specified key. */
	unsigned getNumValues( unsigned const keyID );

	/** @return number of values stored for specified key from its name. */
	unsigned getNumValues( std::string const keyname );

	/** @return value name by index for a given keyname or keyID. */
	std::string getValueName( unsigned const keyID, unsigned const valueID ) const;

	/** @return A value name from keyname and valueId */
	std::string getValueName( std::string const keyname, unsigned const valueID ) const;

	/** Gets value of [keyname] valuename =.
	** Overloaded to return std::string, int, and double.
	** @return defValue if key/value not found otherwise the value obtained. */
	std::string getValue( unsigned const keyID, unsigned const valueID,
						  std::string const defValue = "" ) const;

	/** Gets a value from a keyname and valuename */
	std::string getValue( std::string const keyname, std::string const valuename,
						  std::string const defValue = "" ) const;

	/** Gets the value as integer */
	int getValueI( std::string const keyname, std::string const valuename,
				   int const defValue = 0 ) const;

	/** Gets the value as an unsigned long */
	unsigned long getValueU( std::string const keyname, std::string const valuename,
							 unsigned long const defValue = 0 ) const;

	/** Gets the value as boolean */
	bool getValueB( std::string const keyname, std::string const valuename,
					bool const defValue = false ) const;

	/** Gets the value as double */
	double getValueF( std::string const keyname, std::string const valuename,
					  double const defValue = 0.0 ) const;

	/** This is a variable length formatted GetValue routine. All these voids
	** are required because there is no vsscanf() like there is a vsprintf().
	** Only a maximum of 8 variable can be read. */
	unsigned getValueV( std::string const keyname, std::string const valuename, char* format,
						void* v1 = 0, void* v2 = 0, void* v3 = 0, void* v4 = 0, void* v5 = 0,
						void* v6 = 0, void* v7 = 0, void* v8 = 0, void* v9 = 0, void* v10 = 0,
						void* v11 = 0, void* v12 = 0, void* v13 = 0, void* v14 = 0, void* v15 = 0,
						void* v16 = 0 );

	/** Sets value of [keyname] valuename =.
	** Specify the optional paramter as false (0) if you do not want it to create
	** the key if it doesn't exist. @return true if data entered, false otherwise.
	** Overloaded to accept std::string, int, and double. */
	bool setValue( unsigned const keyID, unsigned const valueID, std::string const value );

	/** Sets the value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValue( std::string const keyname, std::string const valuename, std::string const value,
				   bool create = true );

	/** Sets a integer value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueI( std::string const keyname, std::string const valuename, int const value,
					bool create = true );

	/** Sets a unsigned long value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueU( std::string const keyname, std::string const valuename,
					unsigned long const value, bool create = true );

	/** Sets a boolean value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueB( std::string const keyname, std::string const valuename, bool const value,
					bool create = true ) {
		return setValueI( keyname, valuename, int( value ), create );
	}

	/** Sets a double value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueF( std::string const keyname, std::string const valuename, double const value,
					bool create = true );

	/** Sets a formated value from a keyname and a valuename */
	bool setValueV( std::string const keyname, std::string const valuename, char* format, ... );

	/** Deletes specified value.
	** @return true if value existed and deleted, false otherwise. */
	bool deleteValue( std::string const keyname, std::string const valuename );

	/** Deletes specified key and all values contained within.
	** @return true if key existed and deleted, false otherwise. */
	bool deleteKey( std::string keyname );

	/** Header comment functions.
	** Header comments are those comments before the first key.
	** Number of header comments.*/
	unsigned numHeaderComments() { return (unsigned int)mComments.size(); }

	/** Add a header comment. */
	void addHeaderComment( std::string const comment );

	/** Return a header comment. */
	std::string getHeaderComment( unsigned const commentID ) const;

	/** Delete a header comment. */
	bool deleteHeaderComment( unsigned commentID );

	/** Delete all header comments. */
	void deleteHeaderComments() { mComments.clear(); }

	std::map<std::string, std::string> getKeyMap( const unsigned& keyID ) const;

	std::map<std::string, std::string> getKeyMap( const std::string& keyname ) const;

	std::unordered_map<std::string, std::string> getKeyUnorderedMap( const unsigned& keyID ) const;

	std::unordered_map<std::string, std::string>
	getKeyUnorderedMap( const std::string& keyname ) const;

	/** Key comment functions.
	** Key comments are those comments within a key. Any comments
	** defined within value Names will be added to this list. Therefore,
	** these comments will be moved to the top of the key definition when
	** the IniFile::writeFile() is called.
	** Number of key comments. */
	unsigned getNumKeyComments( unsigned const keyID ) const;
	unsigned getNumKeyComments( std::string const keyname ) const;

	/** Add a key comment.*/
	bool addKeyComment( unsigned const keyID, std::string const comment );
	bool addKeyComment( std::string const keyname, std::string const comment );

	/** Return a key comment. */
	std::string getKeyComment( unsigned const keyID, unsigned const commentID ) const;
	std::string getKeyComment( std::string const keyname, unsigned const commentID ) const;

	/** Delete a key comment. */
	bool deleteKeyComment( unsigned const keyID, unsigned const commentID );
	bool deleteKeyComment( std::string const keyname, unsigned const commentID );

	/** Delete all comments for a key. */
	bool deleteKeyComments( unsigned const keyID );
	bool deleteKeyComments( std::string const keyname );

	bool iniParsed() { return mIniReaded; }

	bool keyExists( const std::string& keyname ) const;

	bool keyValueExists( const std::string& keyname, const std::string& valuename ) const;

  private:
	bool mCaseInsensitive;
	bool mIniReaded;
	std::string mPath;
	struct key {
		std::vector<std::string> names;
		std::vector<std::string> values;
		std::vector<std::string> comments;
	};
	std::vector<key> mKeys;
	std::vector<std::string> mNames;
	std::vector<std::string> mComments;
	std::string checkCase( std::string s ) const;

	std::vector<std::string> mLines;
};

}} // namespace EE::System

#endif
