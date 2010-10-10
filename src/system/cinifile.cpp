#include "cinifile.hpp"

namespace EE { namespace System {

#if EE_PLATFORM == EE_PLATFORM_WIN
#define iniEOL std::endl
#else
#define iniEOL '\r' << std::endl
#endif

cIniFile::cIniFile ( std::string const iniPath ) {
	mCaseInsensitive = true;
	LoadFromFile( iniPath );
}

cIniFile::cIniFile ( const Uint8* RAWData, const Uint32& size ) {
	mCaseInsensitive = true;
	LoadFromMemory( RAWData, size );
}

bool cIniFile::LoadFromMemory( const Uint8* RAWData, const Uint32& size ) {
	std::string myfile;
	myfile.assign( reinterpret_cast<const char*> (RAWData), size );
	mLines.clear();
	mLines = SplitString( myfile );

	return true;
}

bool cIniFile::LoadFromFile( const std::string& iniPath ) {
	// Normally you would use ifstream, but the SGI CC compiler has
	// a few bugs with ifstream. So ... fstream used.
	std::fstream f;
	std::string   line;

	Path ( iniPath );

	f.open ( mPath.c_str(), std::ios::in );

	if ( f.fail() )
		return false;

	mLines.clear();

	while ( getline ( f, line ) )
		mLines.push_back( line );

	f.close();

	return true;
}

bool cIniFile::ReadFile() {
	std::string   line;
	std::string   keyname, valuename, value;
	std::string::size_type pLeft, pRight;

	if ( mLines.size() <= 0 )
		return false;

	for ( Uint32 i = 0; i < mLines.size(); i++ ) {
		line = LTrim ( mLines[i] );

		// To be compatible with Win32, check for existence of '\r'.
		// Win32 files have the '\r' and Unix files don't at the end of a line.
		// Note that the '\r' will be written to INI files from
		// Unix so that the created INI file can be read under Win32
		// without change.
		if ( line.length() && line[line.length() - 1] == '\r' )
			line = line.substr ( 0, line.length() - 1 );

		if ( line.length() ) {
			// Check that the user hasn't openned a binary file by checking the first
			// character of each line!
			if ( !isprint ( line[0] ) ) {
				eePRINT ( "Failing on char %d\n", line[0] );
				return false;
			}

			if ( ( pLeft = line.find_first_of ( ";#[=" ) ) != std::string::npos ) {
				switch ( line[pLeft] ) {
					case '[':
						if ( ( pRight = line.find_last_of ( "]" ) ) != std::string::npos &&
								pRight > pLeft ) {
							keyname = line.substr ( pLeft + 1, pRight - pLeft - 1 );
							AddKeyName ( keyname );
						}
						break;
					case '=':
						valuename = Trim( line.substr ( 0, pLeft ) ); // Remove the extra space between valuename and = . No spaced valuename permited.
						value = line.substr ( pLeft + 1 );
						SetValue ( keyname, valuename, value );
						break;
					case ';':
					case '#':
						if ( !mNames.size() )
							HeaderComment ( line.substr ( pLeft + 1 ) );
						else
							KeyComment ( keyname, line.substr ( pLeft + 1 ) );
						break;
				}
			}
		}
	}

	if ( mNames.size() )
		return true;
	return false;
}

bool cIniFile::WriteFile() {
	unsigned commentID, keyID, valueID;
	// Normally you would use ofstream, but the SGI CC compiler has
	// a few bugs with ofstream. So ... fstream used.
	std::fstream f;

	f.open ( mPath.c_str(), std::ios::out );
	if ( f.fail() )
		return false;

	// Write header mComments.
	for ( commentID = 0; commentID < mComments.size(); ++commentID )
		f << ';' << mComments[commentID] << iniEOL;
	if ( mComments.size() )
		f << iniEOL;

	// Write Keys and values.
	for ( keyID = 0; keyID < mKeys.size(); ++keyID ) {
		f << '[' << mNames[keyID] << ']' << iniEOL;
		// Comments.
		for ( commentID = 0; commentID < mKeys[keyID].comments.size(); ++commentID )
			f << ';' << mKeys[keyID].comments[commentID] << iniEOL;
		// Values.
		for ( valueID = 0; valueID < mKeys[keyID].names.size(); ++valueID )
			f << mKeys[keyID].names[valueID] << '=' << mKeys[keyID].values[valueID] << iniEOL;
		f << iniEOL;
	}
	f.close();

	return true;
}

long cIniFile::FindKey ( std::string const keyname ) const {
	for ( unsigned keyID = 0; keyID < mNames.size(); ++keyID )
		if ( CheckCase ( mNames[keyID] ) == CheckCase ( keyname ) )
			return long ( keyID );
	return noID;
}

long cIniFile::FindValue ( unsigned const keyID, std::string const valuename ) const {
	if ( !mKeys.size() || keyID >= mKeys.size() )
		return noID;

	for ( unsigned valueID = 0; valueID < mKeys[keyID].names.size(); ++valueID )
		if ( CheckCase ( mKeys[keyID].names[valueID] ) == CheckCase ( valuename ) )
			return long ( valueID );
	return noID;
}

unsigned cIniFile::AddKeyName ( std::string const keyname ) {
	mNames.resize ( mNames.size() + 1, keyname );
	mKeys.resize ( mKeys.size() + 1 );
	return (unsigned int)(mNames.size() - 1);
}

std::string cIniFile::KeyName ( unsigned const keyID ) const {
	if ( keyID < mNames.size() )
		return mNames[keyID];
	else
		return "";
}

unsigned cIniFile::NumValues ( unsigned const keyID ) {
	if ( keyID < mKeys.size() )
		return (unsigned int)mKeys[keyID].names.size();
	return 0;
}

unsigned cIniFile::NumValues ( std::string const keyname ) {
	long keyID = FindKey ( keyname );
	if ( keyID == noID )
		return 0;
	return (unsigned int)mKeys[keyID].names.size();
}

std::string cIniFile::ValueName ( unsigned const keyID, unsigned const valueID ) const {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].names.size() )
		return mKeys[keyID].names[valueID];
	return "";
}

std::string cIniFile::ValueName ( std::string const keyname, unsigned const valueID ) const {
	long keyID = FindKey ( keyname );
	if ( keyID == noID )
		return "";
	return ValueName ( keyID, valueID );
}

bool cIniFile::SetValue ( unsigned const keyID, unsigned const valueID, std::string const value ) {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].names.size() )
		mKeys[keyID].values[valueID] = value;

	return false;
}

bool cIniFile::SetValue ( std::string const keyname, std::string const valuename, std::string const value, bool create ) {
	long keyID = FindKey ( keyname );
	if ( keyID == noID ) {
		if ( create )
			keyID = long ( AddKeyName ( keyname ) );
		else
			return false;
	}

	long valueID = FindValue ( unsigned ( keyID ), valuename );
	if ( valueID == noID ) {
		if ( !create )
			return false;
		mKeys[keyID].names.resize ( mKeys[keyID].names.size() + 1, valuename );
		mKeys[keyID].values.resize ( mKeys[keyID].values.size() + 1, value );
	} else
		mKeys[keyID].values[valueID] = value;

	return true;
}

bool cIniFile::SetValueI ( std::string const keyname, std::string const valuename, int const value, bool create ) {
	char svalue[MAX_VALUEDATA];

	StrFormat( svalue, MAX_VALUEDATA, "%d", value );
	return SetValue ( keyname, valuename, svalue, create );
}

bool cIniFile::SetValueF ( std::string const keyname, std::string const valuename, double const value, bool create ) {
	char svalue[MAX_VALUEDATA];

	StrFormat ( svalue, MAX_VALUEDATA, "%f", value );
	return SetValue ( keyname, valuename, svalue, create );
}

bool cIniFile::SetValueV ( std::string const keyname, std::string const valuename, char *format, ... ) {
	va_list args;
	char value[MAX_VALUEDATA];

	va_start ( args, format );
#ifdef EE_COMPILER_MSVC
	vsprintf_s( value, MAX_VALUEDATA, format, args );
#else
	vsprintf ( value, format, args );
#endif
	va_end ( args );
	return SetValue ( keyname, valuename, value );
}

std::string cIniFile::GetValue ( unsigned const keyID, unsigned const valueID, std::string const defValue ) const {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].names.size() )
		return mKeys[keyID].values[valueID];
	return defValue;
}

std::string cIniFile::GetValue ( std::string const keyname, std::string const valuename, std::string const defValue ) const {
	long keyID = FindKey ( keyname );
	if ( keyID == noID )
		return defValue;

	long valueID = FindValue ( unsigned ( keyID ), valuename );
	if ( valueID == noID )
		return defValue;

	return mKeys[keyID].values[valueID];
}

int cIniFile::GetValueI ( std::string const keyname, std::string const valuename, int const defValue ) const {
	char svalue[MAX_VALUEDATA];

	StrFormat ( svalue, MAX_VALUEDATA, "%d", defValue );
	return atoi ( GetValue ( keyname, valuename, svalue ).c_str() );
}

double cIniFile::GetValueF ( std::string const keyname, std::string const valuename, double const defValue ) const {
	char svalue[MAX_VALUEDATA];

	StrFormat ( svalue, MAX_VALUEDATA, "%f", defValue );
	return atof ( GetValue ( keyname, valuename, svalue ).c_str() );
}

// 16 variables may be a bit of over kill, but hey, it's only code.
unsigned cIniFile::GetValueV ( std::string const keyname, std::string const valuename, char *format,
							   void *v1, void *v2, void *v3, void *v4,
							   void *v5, void *v6, void *v7, void *v8,
							   void *v9, void *v10, void *v11, void *v12,
							   void *v13, void *v14, void *v15, void *v16 ) {
	std::string   value;
	// va_list  args;
	unsigned nVals;


	value = GetValue ( keyname, valuename );
	if ( !value.length() )
		return false;
	// Why is there not vsscanf() function. Linux man pages say that there is
	// but no compiler I've seen has it defined. Bummer!
	//
	// va_start( args, format);
	// nVals = vsscanf( value.c_str(), format, args);
	// va_end( args);
#ifdef EE_COMPILER_MSVC
	nVals = (eeUint)sscanf_s( value.c_str(), format,
					 v1, v2, v3, v4, v5, v6, v7, v8,
					 v9, v10, v11, v12, v13, v14, v15, v16 );
#else
	nVals = (eeUint)sscanf ( value.c_str(), format,
					 v1, v2, v3, v4, v5, v6, v7, v8,
					 v9, v10, v11, v12, v13, v14, v15, v16 );
#endif
	return nVals;
}

bool cIniFile::DeleteValue ( std::string const keyname, std::string const valuename ) {
	long keyID = FindKey ( keyname );
	if ( keyID == noID )
		return false;

	long valueID = FindValue ( unsigned ( keyID ), valuename );
	if ( valueID == noID )
		return false;

	// This looks strange, but is neccessary.
	std::vector<std::string>::iterator npos = mKeys[keyID].names.begin() + valueID;
	std::vector<std::string>::iterator vpos = mKeys[keyID].values.begin() + valueID;
	mKeys[keyID].names.erase ( npos, npos + 1 );
	mKeys[keyID].values.erase ( vpos, vpos + 1 );

	return true;
}

bool cIniFile::DeleteKey ( std::string const keyname ) {
	long keyID = FindKey ( keyname );
	if ( keyID == noID )
		return false;

	// Now hopefully this destroys the vector lists within mKeys.
	// Looking at <vector> source, this should be the case using the destructor.
	// If not, I may have to do it explicitly. Memory leak check should tell.
	// memleak_test.cpp shows that the following not required.
	//mKeys[keyID].names.clear();
	//mKeys[keyID].values.clear();

	std::vector<std::string>::iterator npos = mNames.begin() + keyID;
	std::vector<key>::iterator    kpos = mKeys.begin() + keyID;
	mNames.erase ( npos, npos + 1 );
	mKeys.erase ( kpos, kpos + 1 );

	return true;
}

void cIniFile::Erase() {
	// This loop not needed. The vector<> destructor seems to do
	// all the work itself. memleak_test.cpp shows this.
	//for ( unsigned i = 0; i < mKeys.size(); ++i) {
	//  mKeys[i].names.clear();
	//  mKeys[i].values.clear();
	//}
	mNames.clear();
	mKeys.clear();
	mComments.clear();
}

void cIniFile::HeaderComment ( std::string const comment ) {
	mComments.resize ( mComments.size() + 1, comment );
}

std::string cIniFile::HeaderComment ( unsigned const commentID ) const {
	if ( commentID < mComments.size() )
		return mComments[commentID];
	return "";
}

bool cIniFile::DeleteHeaderComment ( unsigned commentID ) {
	if ( commentID < mComments.size() ) {
		std::vector<std::string>::iterator cpos = mComments.begin() + commentID;
		mComments.erase ( cpos, cpos + 1 );
		return true;
	}
	return false;
}

unsigned cIniFile::NumKeyComments ( unsigned const keyID ) const {
	if ( keyID < mKeys.size() )
		return (unsigned int)mKeys[keyID].comments.size();
	return 0;
}

unsigned cIniFile::NumKeyComments ( std::string const keyname ) const {
	long keyID = FindKey ( keyname );
	if ( keyID == noID )
		return 0;
	return (unsigned int)mKeys[keyID].comments.size();
}

bool cIniFile::KeyComment ( unsigned const keyID, std::string const comment ) {
	if ( keyID < mKeys.size() ) {
		mKeys[keyID].comments.resize ( mKeys[keyID].comments.size() + 1, comment );
		return true;
	}
	return false;
}

bool cIniFile::KeyComment ( std::string const keyname, std::string const comment ) {
	long keyID = FindKey ( keyname );
	if ( keyID == noID )
		return false;
	return KeyComment ( unsigned ( keyID ), comment );
}

std::string cIniFile::KeyComment ( unsigned const keyID, unsigned const commentID ) const {
	if ( keyID < mKeys.size() && commentID < mKeys[keyID].comments.size() )
		return mKeys[keyID].comments[commentID];
	return "";
}

std::string cIniFile::KeyComment ( std::string const keyname, unsigned const commentID ) const {
	long keyID = FindKey ( keyname );
	if ( keyID == noID )
		return "";
	return KeyComment ( unsigned ( keyID ), commentID );
}

bool cIniFile::DeleteKeyComment ( unsigned const keyID, unsigned const commentID ) {
	if ( keyID < mKeys.size() && commentID < mKeys[keyID].comments.size() ) {
		std::vector<std::string>::iterator cpos = mKeys[keyID].comments.begin() + commentID;
		mKeys[keyID].comments.erase ( cpos, cpos + 1 );
		return true;
	}
	return false;
}

bool cIniFile::DeleteKeyComment ( std::string const keyname, unsigned const commentID ) {
	long keyID = FindKey ( keyname );
	if ( keyID == noID )
		return false;
	return DeleteKeyComment ( unsigned ( keyID ), commentID );
}

bool cIniFile::DeleteKeyComments ( unsigned const keyID ) {
	if ( keyID < mKeys.size() ) {
		mKeys[keyID].comments.clear();
		return true;
	}
	return false;
}

bool cIniFile::DeleteKeyComments ( std::string const keyname ) {
	long keyID = FindKey ( keyname );
	if ( keyID == noID )
		return false;
	return DeleteKeyComments ( unsigned ( keyID ) );
}

std::string cIniFile::CheckCase ( std::string s ) const {
	if ( mCaseInsensitive )
		for ( std::string::size_type i = 0; i < s.length(); ++i )
			s[i] = std::tolower ( s[i] );
	return s;
}

}}
