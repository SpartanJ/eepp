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

#include <eepp/system/base.hpp>

#define MAX_KEYNAME    128
#define MAX_VALUENAME  128
#define MAX_VALUEDATA 2048

namespace EE { namespace System {

class Pack;

class EE_API IniFile {
	public:
		enum errors { noID = -1 };

		/** Initialize and load the ini file from path  */
		IniFile ( std::string const iniPath = "", const bool& readFile = true );

		/** Initialize and load the ini file from memory  */
		IniFile ( const Uint8* RAWData, const Uint32& size, const bool& readFile = true );

		/** Initialize and load the ini file from a pack file  */
		IniFile ( Pack * Pack, std::string iniPackPath, const bool& readFile = true );

		virtual ~IniFile() {}

		/** Loads an ini file from path */
		bool LoadFromFile( const std::string& iniPath );

		/** Loads an ini file from memory */
		bool LoadFromMemory( const Uint8* RAWData, const Uint32& size );

		/** Loads an ini file from a pack file */
		bool LoadFromPack( Pack * Pack, std::string iniPackPath );

		/** Sets whether or not keynames and valuenames should be case sensitive.
		** The default is case insensitive. */
		void CaseSensitive()                           {mCaseInsensitive = false;}

		/** @see CaseSensitive() */
		void CaseInsensitive()                         {mCaseInsensitive = true;}

		/** Sets mPath of ini file to read and write from. */
		void Path ( std::string const newPath )                {mPath = newPath;}

		/** @return The ini file path */
		std::string Path() const                            {return mPath;}

		/** Reads ini file specified using mPath.
		*	@return true if successful, false otherwise. */
		bool ReadFile();

		/** Writes data stored in class to ini file. */
		bool WriteFile();

		/** Deletes all stored ini data. */
		void Clear();

		/** @return index of specified key, or noID if not found. */
		long FindKey ( std::string const keyname ) const;

		/** @return index of specified value, in the specified key, or noID if not found. */
		long FindValue ( unsigned const keyID, std::string const valuename ) const;

		/** @return number of Keys currently in the ini. */
		unsigned GetNumKeys() const                       {return (unsigned int)mNames.size();}

		/** Add a key name. */
		unsigned AddKeyName ( std::string const keyname );

		/** @return key Names by index. */
		std::string GetKeyName ( unsigned const keyID ) const;

		/** @return number of values stored for specified key. */
		unsigned GetNumValues ( unsigned const keyID );

		/** @return number of values stored for specified key from its name. */
		unsigned GetNumValues ( std::string const keyname );

		/** @return value name by index for a given keyname or keyID. */
		std::string GetValueName ( unsigned const keyID, unsigned const valueID ) const;

		/** @return A value name from keyname and valueId */
		std::string GetValueName ( std::string const keyname, unsigned const valueID ) const;

		/** Gets value of [keyname] valuename =.
		** Overloaded to return std::string, int, and double.
		** @return defValue if key/value not found otherwise the value obtained. */
		std::string GetValue ( unsigned const keyID, unsigned const valueID, std::string const defValue = "" ) const;

		/** Gets a value from a keyname and valuename */
		std::string GetValue ( std::string const keyname, std::string const valuename, std::string const defValue = "" ) const;

		/** Gets the value as integer */
		int    GetValueI ( std::string const keyname, std::string const valuename, int const defValue = 0 ) const;

		/** Gets the value as boolean */
		bool   GetValueB ( std::string const keyname, std::string const valuename, bool const defValue = false ) const {
			return 0 != ( GetValueI ( keyname, valuename, int ( defValue ) ) );
		}

		/** Gets the value as double */
		double   GetValueF ( std::string const keyname, std::string const valuename, double const defValue = 0.0 ) const;

		/** This is a variable length formatted GetValue routine. All these voids
		** are required because there is no vsscanf() like there is a vsprintf().
		** Only a maximum of 8 variable can be read. */
		unsigned GetValueV ( std::string const keyname, std::string const valuename, char *format,
							 void *v1 = 0, void *v2 = 0, void *v3 = 0, void *v4 = 0,
							 void *v5 = 0, void *v6 = 0, void *v7 = 0, void *v8 = 0,
							 void *v9 = 0, void *v10 = 0, void *v11 = 0, void *v12 = 0,
							 void *v13 = 0, void *v14 = 0, void *v15 = 0, void *v16 = 0 );

		/** Sets value of [keyname] valuename =.
		** Specify the optional paramter as false (0) if you do not want it to create
		** the key if it doesn't exist. @return true if data entered, false otherwise.
		** Overloaded to accept std::string, int, and double. */
		bool SetValue ( unsigned const keyID, unsigned const valueID, std::string const value );

		/** Sets the value from a keyname and a valuename
		*	@param keyname The key name
		*	@param valuename The value name
		*	@param value The value to assign
		*	@param create If true it will create the keyname if doesn't exists
		*/
		bool SetValue ( std::string const keyname, std::string const valuename, std::string const value, bool create = true );

		/** Sets a integer value from a keyname and a valuename
		*	@param keyname The key name
		*	@param valuename The value name
		*	@param value The value to assign
		*	@param create If true it will create the keyname if doesn't exists
		*/
		bool SetValueI ( std::string const keyname, std::string const valuename, int const value, bool create = true );

		/** Sets a boolean value from a keyname and a valuename
		*	@param keyname The key name
		*	@param valuename The value name
		*	@param value The value to assign
		*	@param create If true it will create the keyname if doesn't exists
		*/
		bool SetValueB ( std::string const keyname, std::string const valuename, bool const value, bool create = true ) {
			return SetValueI ( keyname, valuename, int ( value ), create );
		}

		/** Sets a double value from a keyname and a valuename
		*	@param keyname The key name
		*	@param valuename The value name
		*	@param value The value to assign
		*	@param create If true it will create the keyname if doesn't exists
		*/
		bool SetValueF ( std::string const keyname, std::string const valuename, double const value, bool create = true );

		/** Sets a formated value from a keyname and a valuename */
		bool SetValueV ( std::string const keyname, std::string const valuename, char *format, ... );

		/** Deletes specified value.
		** @return true if value existed and deleted, false otherwise. */
		bool DeleteValue ( std::string const keyname, std::string const valuename );

		/** Deletes specified key and all values contained within.
		** @return true if key existed and deleted, false otherwise. */
		bool DeleteKey ( std::string keyname );

		/** Header comment functions.
		** Header comments are those comments before the first key.
		** Number of header comments.*/
		unsigned NumHeaderComments()                  {return (unsigned int)mComments.size();}

		/** Add a header comment. */
		void AddHeaderComment ( std::string const comment );

		/** Return a header comment. */
		std::string GetHeaderComment ( unsigned const commentID ) const;

		/** Delete a header comment. */
		bool DeleteHeaderComment ( unsigned commentID );

		/** Delete all header comments. */
		void DeleteHeaderComments()               { mComments.clear();}

		/** Key comment functions.
		** Key comments are those comments within a key. Any comments
		** defined within value Names will be added to this list. Therefore,
		** these comments will be moved to the top of the key definition when
		** the IniFile::WriteFile() is called.
		** Number of key comments. */
		unsigned GetNumKeyComments ( unsigned const keyID ) const;
		unsigned GetNumKeyComments ( std::string const keyname ) const;

		/** Add a key comment.*/
		bool AddKeyComment ( unsigned const keyID, std::string const comment );
		bool AddKeyComment ( std::string const keyname, std::string const comment );

		/** Return a key comment. */
		std::string GetKeyComment ( unsigned const keyID, unsigned const commentID ) const;
		std::string GetKeyComment ( std::string const keyname, unsigned const commentID ) const;

		/** Delete a key comment. */
		bool     DeleteKeyComment ( unsigned const keyID, unsigned const commentID );
		bool     DeleteKeyComment ( std::string const keyname, unsigned const commentID );

		/** Delete all comments for a key. */
		bool     DeleteKeyComments ( unsigned const keyID );
		bool     DeleteKeyComments ( std::string const keyname );
	private:
		bool	mCaseInsensitive;
		bool	mIniReaded;
		std::string mPath;
		struct key {
			std::vector<std::string> names;
			std::vector<std::string> values;
			std::vector<std::string> comments;
		};
		std::vector<key>    mKeys;
		std::vector<std::string> mNames;
		std::vector<std::string> mComments;
		std::string CheckCase ( std::string s ) const;

		std::vector <std::string> mLines;
};

}}

#endif
