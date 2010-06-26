// IniFile.cpp:  Implementation of the cIniFile class.
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

#include "base.hpp"

#define MAX_KEYNAME    128
#define MAX_VALUENAME  128
#define MAX_VALUEDATA 2048

namespace EE { namespace System {

class EE_API cIniFile {
	private:
		bool   mCaseInsensitive;
		std::string mPath;
		struct key {
			vector<std::string> names;
			vector<std::string> values;
			vector<std::string> comments;
		};
		vector<key>    mKeys;
		vector<std::string> mNames;
		vector<std::string> mComments;
		std::string CheckCase ( std::string s ) const;
		
		std::vector <std::string> mLines;
	public:
		enum errors { noID = -1};
		cIniFile ( std::string const iniPath = "" );
		cIniFile ( const Uint8* RAWData, const Uint32& size );
		
		bool LoadFromFile( const std::string& iniPath );
		bool LoadFromMemory( const Uint8* RAWData, const Uint32& size );
		
		virtual ~cIniFile()                            {}

		// Sets whether or not keynames and valuenames should be case sensitive.
		// The default is case insensitive.
		void CaseSensitive()                           {mCaseInsensitive = false;}
		void CaseInsensitive()                         {mCaseInsensitive = true;}

		// Sets mPath of ini file to read and write from.
		void Path ( std::string const newPath )                {mPath = newPath;}
		std::string Path() const                            {return mPath;}
		void SetPath ( std::string const newPath )             {Path ( newPath );}

		// Reads ini file specified using mPath.
		// Returns true if successful, false otherwise.
		bool ReadFile();

		// Writes data stored in class to ini file.
		bool WriteFile();

		// Deletes all stored ini data.
		void Erase();
		void Clear()                                   {Erase();}
		void Reset()                                   {Erase();}

		// Returns index of specified key, or noID if not found.
		long FindKey ( std::string const keyname ) const;

		// Returns index of specified value, in the specified key, or noID if not found.
		long FindValue ( unsigned const keyID, std::string const valuename ) const;

		// Returns number of mKeys currently in the ini.
		unsigned NumKeys() const                       {return mNames.size();}
		unsigned GetNumKeys() const                    {return NumKeys();}

		// Add a key name.
		unsigned AddKeyName ( std::string const keyname );

		// Returns key Names by index.
		std::string KeyName ( unsigned const keyID ) const;
		std::string GetKeyName ( unsigned const keyID ) const {return KeyName ( keyID );}

		// Returns number of values stored for specified key.
		unsigned NumValues ( unsigned const keyID );
		unsigned GetNumValues ( unsigned const keyID )   {return NumValues ( keyID );}
		unsigned NumValues ( std::string const keyname );
		unsigned GetNumValues ( std::string const keyname )   {return NumValues ( keyname );}

		// Returns value name by index for a given keyname or keyID.
		std::string ValueName ( unsigned const keyID, unsigned const valueID ) const;
		std::string GetValueName ( unsigned const keyID, unsigned const valueID ) const {
			return ValueName ( keyID, valueID );
		}
		std::string ValueName ( std::string const keyname, unsigned const valueID ) const;
		std::string GetValueName ( std::string const keyname, unsigned const valueID ) const {
			return ValueName ( keyname, valueID );
		}

		// Gets value of [keyname] valuename =.
		// Overloaded to return std::string, int, and double.
		// Returns defValue if key/value not found.
		std::string GetValue ( unsigned const keyID, unsigned const valueID, std::string const defValue = "" ) const;
		std::string GetValue ( std::string const keyname, std::string const valuename, std::string const defValue = "" ) const;
		int    GetValueI ( std::string const keyname, std::string const valuename, int const defValue = 0 ) const;
		bool   GetValueB ( std::string const keyname, std::string const valuename, bool const defValue = false ) const {
			return 0 != ( GetValueI ( keyname, valuename, int ( defValue ) ) );
		}
		double   GetValueF ( std::string const keyname, std::string const valuename, double const defValue = 0.0 ) const;
		// This is a variable length formatted GetValue routine. All these voids
		// are required because there is no vsscanf() like there is a vsprintf().
		// Only a maximum of 8 variable can be read.
		unsigned GetValueV ( std::string const keyname, std::string const valuename, char *format,
							 void *v1 = 0, void *v2 = 0, void *v3 = 0, void *v4 = 0,
							 void *v5 = 0, void *v6 = 0, void *v7 = 0, void *v8 = 0,
							 void *v9 = 0, void *v10 = 0, void *v11 = 0, void *v12 = 0,
							 void *v13 = 0, void *v14 = 0, void *v15 = 0, void *v16 = 0 );

		// Sets value of [keyname] valuename =.
		// Specify the optional paramter as false (0) if you do not want it to create
		// the key if it doesn't exist. Returns true if data entered, false otherwise.
		// Overloaded to accept std::string, int, and double.
		bool SetValue ( unsigned const keyID, unsigned const valueID, std::string const value );
		bool SetValue ( std::string const keyname, std::string const valuename, std::string const value, bool const create = true );
		bool SetValueI ( std::string const keyname, std::string const valuename, int const value, bool const create = true );
		bool SetValueB ( std::string const keyname, std::string const valuename, bool const value, bool const create = true ) {
			return SetValueI ( keyname, valuename, int ( value ), create );
		}
		bool SetValueF ( std::string const keyname, std::string const valuename, double const value, bool const create = true );
		bool SetValueV ( std::string const keyname, std::string const valuename, char *format, ... );

		// Deletes specified value.
		// Returns true if value existed and deleted, false otherwise.
		bool DeleteValue ( std::string const keyname, std::string const valuename );

		// Deletes specified key and all values contained within.
		// Returns true if key existed and deleted, false otherwise.
		bool DeleteKey ( std::string keyname );

		// Header comment functions.
		// Header comments are those comments before the first key.
		//
		// Number of header comments.
		unsigned NumHeaderComments()                  {return mComments.size();}
		// Add a header comment.
		void     HeaderComment ( std::string const comment );
		// Return a header comment.
		std::string   HeaderComment ( unsigned const commentID ) const;
		// Delete a header comment.
		bool     DeleteHeaderComment ( unsigned commentID );
		// Delete all header comments.
		void     DeleteHeaderComments()               { mComments.clear();}

		// Key comment functions.
		// Key comments are those comments within a key. Any comments
		// defined within value Names will be added to this list. Therefore,
		// these comments will be moved to the top of the key definition when
		// the cIniFile::WriteFile() is called.
		//
		// Number of key comments.
		unsigned NumKeyComments ( unsigned const keyID ) const;
		unsigned NumKeyComments ( std::string const keyname ) const;
		// Add a key comment.
		bool     KeyComment ( unsigned const keyID, std::string const comment );
		bool     KeyComment ( std::string const keyname, std::string const comment );
		// Return a key comment.
		std::string   KeyComment ( unsigned const keyID, unsigned const commentID ) const;
		std::string   KeyComment ( std::string const keyname, unsigned const commentID ) const;
		// Delete a key comment.
		bool     DeleteKeyComment ( unsigned const keyID, unsigned const commentID );
		bool     DeleteKeyComment ( std::string const keyname, unsigned const commentID );
		// Delete all comments for a key.
		bool     DeleteKeyComments ( unsigned const keyID );
		bool     DeleteKeyComments ( std::string const keyname );
};

}}

#endif
