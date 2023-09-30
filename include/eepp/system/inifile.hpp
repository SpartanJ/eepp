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
// Spartan: I made many modifications to the class. So, this is not the original.
//////////////////////////////////////////////////////////////////////

#ifndef EE_SYSTEM_INIFILE_HPP
#define EE_SYSTEM_INIFILE_HPP

#include <eepp/system/iostream.hpp>
#include <map>
#include <string_view>
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
	IniFile( const std::string& iniPath = "", bool autoLoad = true );

	/** Initialize and load the ini file from memory  */
	IniFile( const Uint8* RAWData, const Uint32& size, bool autoLoad = true );

	/** Initialize and load the ini file from a pack file  */
	IniFile( Pack* Pack, const std::string& iniPackPath, bool autoLoad = true );

	/** Initialize and load the ini file from a stream  */
	IniFile( IOStream& stream, bool autoLoad = true );

	virtual ~IniFile() {}

	/** Loads an ini file from path */
	bool loadFromFile( const std::string& iniPath );

	/** Loads an ini file from memory */
	bool loadFromMemory( const Uint8* RAWData, const Uint32& size );

	/** Loads an ini file from a pack file */
	bool loadFromPack( Pack* Pack, const std::string& iniPackPath );

	/** Loads an ini file from a stream */
	bool loadFromStream( IOStream& stream );

	/** Sets mPath of ini file to read and write from. */
	void path( const std::string& newPath ) { mPath = newPath; }

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
	long findKey( const std::string& keyname ) const;

	long findKey( const std::string_view& keyname ) const;

	/** @return index of specified value, in the specified key, or noID if not found. */
	long findValue( unsigned const keyID, const std::string& valuename ) const;

	long findValue( unsigned const keyID, const std::string_view& valuename ) const;

	/** @return number of Keys currently in the ini. */
	unsigned getNumKeys() const { return (unsigned int)mNames.size(); }

	/** Add a key name. */
	unsigned addKeyName( const std::string& keyname );

	unsigned addKeyName( const std::string_view& keyname );

	/** @return key Names by index. */
	std::string getKeyName( unsigned const keyID ) const;

	/** @return number of values stored for specified key. */
	unsigned getNumValues( unsigned const keyID );

	/** @return number of values stored for specified key from its name. */
	unsigned getNumValues( const std::string& keyname );

	/** @return value name by index for a given keyname or keyID. */
	std::string getValueName( unsigned const keyID, unsigned const valueID ) const;

	/** @return A value name from keyname and valueId */
	std::string getValueName( const std::string& keyname, unsigned const valueID ) const;

	/** Gets value of [keyname] valuename =.
	** Overloaded to return std::string, int, and double.
	** @return defValue if key/value not found otherwise the value obtained. */
	std::string getValue( unsigned const keyID, unsigned const valueID,
						  const std::string& defValue = "" ) const;

	/** Gets a value from a keyname and valuename */
	std::string getValue( const std::string& keyname, const std::string& valuename,
						  const std::string& defValue = "" ) const;

	/** Gets the value as integer */
	int getValueI( const std::string& keyname, const std::string& valuename,
				   int const defValue = 0 ) const;

	/** Gets the value as an unsigned long */
	unsigned long getValueU( const std::string& keyname, const std::string& valuename,
							 unsigned long const defValue = 0 ) const;

	/** Gets the value as boolean */
	bool getValueB( const std::string& keyname, const std::string& valuename,
					bool const defValue = false ) const;

	/** Gets the value as double */
	double getValueF( const std::string& keyname, const std::string& valuename,
					  double const defValue = 0.0 ) const;

	/** Sets value of [keyname] valuename =.
	** Specify the optional paramter as false (0) if you do not want it to create
	** the key if it doesn't exist. @return true if data entered, false otherwise.
	** Overloaded to accept std::string, int, and double. */
	bool setValue( unsigned const keyID, unsigned const valueID, const std::string& value );

	/** Sets the value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValue( const std::string& keyname, const std::string& valuename,
				   const std::string& value, bool create = true );

	bool setValue( const std::string_view& keyname, const std::string_view& valuename,
				   const std::string_view& value, bool create = true );

	/** Sets a integer value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueI( const std::string& keyname, const std::string& valuename, int const value,
					bool create = true );

	/** Sets a unsigned long value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueU( const std::string& keyname, const std::string& valuename,
					unsigned long const value, bool create = true );

	/** Sets a boolean value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueB( const std::string& keyname, const std::string& valuename, bool const value,
					bool create = true ) {
		return setValueI( keyname, valuename, int( value ), create );
	}

	/** Sets a double value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueF( const std::string& keyname, const std::string& valuename, double const value,
					bool create = true );

	/** Sets a formated value from a keyname and a valuename */
	bool setValueV( const std::string& keyname, const std::string& valuename, char* format, ... );

	/** Deletes specified value.
	** @return true if value existed and deleted, false otherwise. */
	bool deleteValue( const std::string& keyname, const std::string& valuename );

	/** Deletes specified key and all values contained within.
	** @return true if key existed and deleted, false otherwise. */
	bool deleteKey( const std::string& keyname );

	/** Header comment functions.
	** Header comments are those comments before the first key.
	** Number of header comments.*/
	unsigned numHeaderComments() { return (unsigned int)mComments.size(); }

	/** Add a header comment. */
	void addHeaderComment( const std::string& comment );

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
	unsigned getNumKeyComments( const std::string& keyname ) const;

	/** Add a key comment.*/
	bool addKeyComment( unsigned const keyID, const std::string& comment );
	bool addKeyComment( unsigned const keyID, const std::string_view& comment );
	bool addKeyComment( const std::string& keyname, const std::string& comment );
	bool addKeyComment( const std::string_view& keyname, const std::string_view& comment );

	/** Return a key comment. */
	std::string getKeyComment( unsigned const keyID, unsigned const commentID ) const;
	std::string getKeyComment( const std::string& keyname, unsigned const commentID ) const;

	/** Delete a key comment. */
	bool deleteKeyComment( unsigned const keyID, unsigned const commentID );
	bool deleteKeyComment( const std::string& keyname, unsigned const commentID );

	/** Delete all comments for a key. */
	bool deleteKeyComments( unsigned const keyID );
	bool deleteKeyComments( const std::string& keyname );

	bool iniParsed() { return mIniReaded; }

	bool keyExists( const std::string& keyname ) const;

	bool keyValueExists( const std::string& keyname, const std::string& valuename ) const;

  private:
	bool mIniReaded{ false };
	std::string mPath;
	std::string mBuffer;
	struct key {
		std::vector<std::string> names;
		std::vector<std::string> values;
		std::vector<std::string> comments;
	};
	std::vector<key> mKeys;
	std::vector<std::string> mNames;
	std::vector<std::string> mComments;
};

}} // namespace EE::System

#endif
