// IniFile.cpp:  Implementation of the IniFile class.
// Written by:   Adam Clauss
// Email: cabadam@tamu.edu
// You may use this class/code as you wish in your programs.  Feel free to distribute it, and
// email suggested changes to me.
//
// Rewritten by: Shane Hill
// Date:         21/08/2001
// Email:        Shane.Hill@dsto.defence.gov.au
// Reason:       Remove dependency on MFC. Code should compile on any
//               platform. Tested on Windows/Linux/Irix
//
// Spartan: I made many modifications to the class. So, this is not the original.
//////////////////////////////////////////////////////////////////////

#ifndef EE_SYSTEM_INIFILE_HPP
#define EE_SYSTEM_INIFILE_HPP

#include <eepp/core/small_vector.hpp>
#include <eepp/system/iostream.hpp>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

namespace EE { namespace System {

class Pack;

class EE_API IniFile {
  public:
	enum errors { noID = -1 };

	/** Initialize and load the ini file from path  */
	IniFile( std::string_view iniPath = {}, bool autoLoad = true );

	/** Initialize and load the ini file from memory  */
	IniFile( const Uint8* RAWData, const Uint32& size, bool autoLoad = true );

	/** Initialize and load the ini file from a pack file  */
	IniFile( Pack* Pack, std::string_view iniPackPath, bool autoLoad = true );

	/** Initialize and load the ini file from a stream  */
	IniFile( IOStream& stream, bool autoLoad = true );

	virtual ~IniFile() {}

	/** Loads an ini file from path */
	bool loadFromFile( std::string_view iniPath );

	/** Loads an ini file from memory */
	bool loadFromMemory( const Uint8* RAWData, const Uint32& size );

	/** Loads an ini file from a pack file */
	bool loadFromPack( Pack* Pack, std::string_view iniPackPath );

	/** Loads an ini file from a stream */
	bool loadFromStream( IOStream& stream );

	/** Sets mPath of ini file to read and write from. */
	void path( std::string_view newPath ) { mPath.assign( newPath ); }

	/** @return The ini file path */
	const std::string& path() const { return mPath; }

	/** Reads ini file specified using mPath.
	 *	@return true if successful, false otherwise. */
	bool readFile();

	/** Writes data stored in class to ini file. */
	bool writeFile();

	/** Writes data stored in class to a IOStream. */
	bool writeStream( IOStream& stream );

	/** Deletes all stored ini data. */
	void clear();

	/** @return index of specified key, or noID if not found. */
	long findKey( std::string_view keyname ) const;

	/** @return index of specified value, in the specified key, or noID if not found. */
	long findValue( unsigned const keyID, std::string_view valuename ) const;

	/** @return number of Keys currently in the ini. */
	unsigned getNumKeys() const { return (unsigned int)mKeys.size(); }

	/** Add a key name. */
	unsigned addKeyName( std::string_view keyname );

	/** @return key Names by index. */
	std::string getKeyName( unsigned const keyID ) const;

	/** @return number of values stored for specified key. */
	unsigned getNumValues( unsigned const keyID );

	/** @return number of values stored for specified key from its name. */
	unsigned getNumValues( std::string_view keyname );

	/** @return value name by index for a given keyname or keyID. */
	std::string getValueName( unsigned const keyID, unsigned const valueID ) const;

	/** @return A value name from keyname and valueId */
	std::string getValueName( std::string_view keyname, unsigned const valueID ) const;

	/** Gets value of [keyname] valuename =.
	** Overloaded to return std::string, int, and double.
	** @return defValue if key/value not found otherwise the value obtained. */
	std::string getValue( unsigned const keyID, unsigned const valueID,
						  std::string_view defValue = {} ) const;

	/** Gets a value from a keyname and valuename */
	std::string getValue( std::string_view keyname, std::string_view valuename,
						  std::string_view defValue = {} ) const;

	/** Gets the value as integer */
	int getValueI( std::string_view keyname, std::string_view valuename,
				   int const defValue = 0 ) const;

	/** Gets the value as an unsigned long */
	unsigned long getValueU( std::string_view keyname, std::string_view valuename,
							 unsigned long const defValue = 0 ) const;

	/** Gets the value as boolean */
	bool getValueB( std::string_view keyname, std::string_view valuename,
					bool const defValue = false ) const;

	/** Gets the value as double */
	double getValueF( std::string_view keyname, std::string_view valuename,
					  double const defValue = 0.0 ) const;

	/** Sets value of [keyname] valuename =.
	** Specify the optional parameter as false (0) if you do not want it to create
	** the key if it doesn't exist. @return true if data entered, false otherwise.
	** Overloaded to accept std::string, int, and double. */
	bool setValue( unsigned const keyID, unsigned const valueID, std::string_view value );

	/** Sets the value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValue( std::string_view keyname, std::string_view valuename, std::string_view value,
				   bool create = true );

	/** Sets a integer value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueI( std::string_view keyname, std::string_view valuename, int const value,
					bool create = true );

	/** Sets a unsigned long value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueU( std::string_view keyname, std::string_view valuename, unsigned long const value,
					bool create = true );

	/** Sets a boolean value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueB( std::string_view keyname, std::string_view valuename, bool const value,
					bool create = true ) {
		return setValueI( keyname, valuename, int( value ), create );
	}

	/** Sets a double value from a keyname and a valuename
	 *	@param keyname The key name
	 *	@param valuename The value name
	 *	@param value The value to assign
	 *	@param create If true it will create the keyname if doesn't exists
	 */
	bool setValueF( std::string_view keyname, std::string_view valuename, double const value,
					bool create = true );

	/** Sets a formatted value from a keyname and a valuename */
	bool setValueV( std::string_view keyname, std::string_view valuename, char* format, ... );

	/** Deletes specified value.
	** @return true if value existed and deleted, false otherwise. */
	bool deleteValue( std::string_view keyname, std::string_view valuename );

	/** Deletes specified key and all values contained within.
	** @return true if key existed and deleted, false otherwise. */
	bool deleteKey( std::string_view keyname );

	/** Header comment functions.
	** Header comments are those comments before the first key.
	** Number of header comments.*/
	unsigned numHeaderComments() { return (unsigned int)mComments.size(); }

	/** Add a header comment. */
	void addHeaderComment( std::string_view comment );

	/** Return a header comment. */
	std::string getHeaderComment( unsigned const commentID ) const;

	/** Delete a header comment. */
	bool deleteHeaderComment( unsigned commentID );

	/** Delete all header comments. */
	void deleteHeaderComments() { mComments.clear(); }

	std::map<std::string, std::string> getKeyMap( const unsigned& keyID ) const;

	std::map<std::string, std::string> getKeyMap( std::string_view keyname ) const;

	std::unordered_map<std::string, std::string> getKeyUnorderedMap( const unsigned& keyID ) const;

	std::unordered_map<std::string, std::string>
	getKeyUnorderedMap( std::string_view keyname ) const;

	/** Key comment functions.
	** Key comments are those comments within a key. Any comments
	** defined within value Names will be added to this list. Therefore,
	** these comments will be moved to the top of the key definition when
	** the IniFile::writeFile() is called.
	** Number of key comments. */
	unsigned getNumKeyComments( unsigned const keyID ) const;
	unsigned getNumKeyComments( std::string_view keyname ) const;

	/** Add a key comment.*/
	bool addKeyComment( unsigned const keyID, std::string_view comment );
	bool addKeyComment( std::string_view keyname, std::string_view comment );

	/** Return a key comment. */
	std::string getKeyComment( unsigned const keyID, unsigned const commentID ) const;
	std::string getKeyComment( std::string_view keyname, unsigned const commentID ) const;

	/** Delete a key comment. */
	bool deleteKeyComment( unsigned const keyID, unsigned const commentID );
	bool deleteKeyComment( std::string_view keyname, unsigned const commentID );

	/** Delete all comments for a key. */
	bool deleteKeyComments( unsigned const keyID );
	bool deleteKeyComments( std::string_view keyname );

	bool iniParsed() { return mIniRead; }

	bool keyExists( std::string_view keyname ) const;

	bool keyValueExists( std::string_view keyname, std::string_view valuename ) const;

  private:
	bool mIniRead{ false };
	std::string mPath;
	std::string mBuffer;
	struct value {
		std::string name;
		std::string data;
	};
	struct key {
		std::string name;
		SmallVector<value, 2> values;
		SmallVector<std::string, 1> comments;
	};
	SmallVector<key, 4> mKeys;
	SmallVector<std::string, 2> mComments;
};

}} // namespace EE::System

#endif
