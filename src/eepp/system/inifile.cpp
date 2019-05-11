#include <cctype>
#include <eepp/system/inifile.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/filesystem.hpp>
#include <cstdarg>

namespace EE { namespace System {

#if EE_PLATFORM == EE_PLATFORM_WIN
#define iniEOL std::endl
#else
#define iniEOL '\r' << std::endl
#endif

IniFile::IniFile ( std::string const iniPath, const bool& shouldReadFile ) :
	mCaseInsensitive( true ),
	mIniReaded( false )
{
	loadFromFile( iniPath );

	if ( shouldReadFile )
		readFile();
}

IniFile::IniFile (const Uint8* RAWData, const Uint32& size, const bool& shouldReadFile ) :
	mCaseInsensitive( true ),
	mIniReaded( false )
{
	loadFromMemory( RAWData, size );

	if ( shouldReadFile )
		readFile();
}

IniFile::IniFile( Pack * Pack, std::string iniPackPath, const bool& shouldReadFile ) :
	mCaseInsensitive( true ),
	mIniReaded( false )
{
	loadFromPack( Pack, iniPackPath );

	if ( shouldReadFile )
		readFile();
}

IniFile::IniFile( IOStream& stream, const bool& shouldReadFile ) :
	mCaseInsensitive( true ),
	mIniReaded( false )
{
	loadFromStream( stream );

	if ( shouldReadFile )
		readFile();
}

bool IniFile::loadFromPack( Pack * Pack, std::string iniPackPath ) {
	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( iniPackPath ) ) {
		ScopedBuffer buffer;

		Pack->extractFileToMemory( iniPackPath, buffer );

		return loadFromMemory( buffer.get(), buffer.length() );
	}

	return false;
}

bool IniFile::loadFromStream( IOStream& stream ) {
	if ( !stream.isOpen() )
		return false;

	std::string myfile( (size_t)stream.getSize(), '\0' );

	stream.read( (char*)&myfile[0], stream.getSize() );

	clear();
	mLines.clear();
	mLines = String::split( myfile );

	return true;
}

bool IniFile::loadFromMemory( const Uint8* RAWData, const Uint32& size ) {
	IOStreamMemory f( reinterpret_cast<const char*>( RAWData ), size );
	return loadFromStream( f );
}

bool IniFile::loadFromFile( const std::string& iniPath ) {
	path ( iniPath );

	if ( FileSystem::fileExists( iniPath ) ) {
		IOStreamFile f( mPath );
		return loadFromStream( f );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string tPath( iniPath );

		Pack * tPack = PackManager::instance()->exists( tPath );

		if ( NULL != tPack ) {
			return loadFromPack( tPack, tPath );
		}
	}

	return false;
}

bool IniFile::readFile() {
	std::string   line;
	std::string   keyname, valuename, value;
	std::string::size_type pLeft, pRight;

	if ( mIniReaded )
		return true;

	if ( mLines.size() <= 0 )
		return false;

	for ( Uint32 i = 0; i < mLines.size(); i++ ) {
		line = String::lTrim ( mLines[i] );

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
				eePRINT ( "IniFile::ReadFile(): Failing on char %d\n", line[0] );
				return false;
			}

			if ( ( pLeft = line.find_first_of ( ";#[=" ) ) != std::string::npos ) {
				switch ( line[pLeft] ) {
					case '[':
						if ( ( pRight = line.find_last_of ( "]" ) ) != std::string::npos &&
								pRight > pLeft ) {
							keyname = line.substr ( pLeft + 1, pRight - pLeft - 1 );
							addKeyName ( keyname );
						}
						break;
					case '=':
						valuename = String::trim( line.substr ( 0, pLeft ) ); // Remove the extra space between valuename and = . No spaced valuename permited.
						value = String::lTrim( line.substr ( pLeft + 1 ) );
						setValue ( keyname, valuename, value );
						break;
					case ';':
					case '#':
						if ( !mNames.size() )
							addHeaderComment ( line.substr ( pLeft + 1 ) );
						else
							addKeyComment ( keyname, line.substr ( pLeft + 1 ) );
						break;
				}
			}
		}
	}

	if ( mNames.size() ) {
		mIniReaded = true;

		return true;
	}

	return false;
}

bool IniFile::writeFile() {
	unsigned commentID, keyID, valueID;

	IOStreamFile f( mPath, "w" );

	if ( !f.isOpen() )
		return false;

	std::string str;

	// Write header mComments.
	for ( commentID = 0; commentID < mComments.size(); ++commentID ) {
		str = ';' + mComments[commentID] + '\n';

		f.write( str.c_str(), str.size() );
	}

	if ( mComments.size() ) {
		str = "\n";

		f.write( str.c_str(), str.size() );
	}

	// Write Keys and values.
	for ( keyID = 0; keyID < mKeys.size(); ++keyID ) {
		str = '[' + mNames[keyID] + ']' + '\n';

		f.write( str.c_str(), str.size() );

		// Comments.
		for ( commentID = 0; commentID < mKeys[keyID].comments.size(); ++commentID ) {
			str = ';' + mKeys[keyID].comments[commentID] + '\n';

			f.write( str.c_str(), str.size() );
		}

		// Values.
		for ( valueID = 0; valueID < mKeys[keyID].names.size(); ++valueID ) {
			str = mKeys[keyID].names[valueID] + '=' + mKeys[keyID].values[valueID] + '\n';

			f.write( str.c_str(), str.size() );
		}

		str = "\n";

		f.write( str.c_str(), str.size() );
	}

	return true;
}

long IniFile::findKey ( std::string const keyname ) const {
	for ( unsigned keyID = 0; keyID < mNames.size(); ++keyID )
		if ( CheckCase ( mNames[keyID] ) == CheckCase ( keyname ) )
			return long ( keyID );
	return noID;
}

long IniFile::findValue ( unsigned const keyID, std::string const valuename ) const {
	if ( !mKeys.size() || keyID >= mKeys.size() )
		return noID;

	for ( unsigned valueID = 0; valueID < mKeys[keyID].names.size(); ++valueID )
		if ( CheckCase ( mKeys[keyID].names[valueID] ) == CheckCase ( valuename ) )
			return long ( valueID );
	return noID;
}

unsigned IniFile::addKeyName ( std::string const keyname ) {
	mNames.resize ( mNames.size() + 1, keyname );
	mKeys.resize ( mKeys.size() + 1 );
	return (unsigned int)(mNames.size() - 1);
}

std::string IniFile::getKeyName ( unsigned const keyID ) const {
	if ( keyID < mNames.size() )
		return mNames[keyID];
	else
		return "";
}

unsigned IniFile::getNumValues ( unsigned const keyID ) {
	if ( keyID < mKeys.size() )
		return (unsigned int)mKeys[keyID].names.size();
	return 0;
}

unsigned IniFile::getNumValues ( std::string const keyname ) {
	long keyID = findKey ( keyname );
	if ( keyID == noID )
		return 0;
	return (unsigned int)mKeys[keyID].names.size();
}

std::string IniFile::getValueName ( unsigned const keyID, unsigned const valueID ) const {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].names.size() )
		return mKeys[keyID].names[valueID];
	return "";
}

std::string IniFile::getValueName ( std::string const keyname, unsigned const valueID ) const {
	long keyID = findKey ( keyname );
	if ( keyID == noID )
		return "";
	return getValueName ( keyID, valueID );
}

bool IniFile::setValue ( unsigned const keyID, unsigned const valueID, std::string const value ) {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].names.size() )
		mKeys[keyID].values[valueID] = value;

	return false;
}

bool IniFile::setValue ( std::string const keyname, std::string const valuename, std::string const value, bool create ) {
	long keyID = findKey ( keyname );
	if ( keyID == noID ) {
		if ( create )
			keyID = long ( addKeyName ( keyname ) );
		else
			return false;
	}

	long valueID = findValue ( unsigned ( keyID ), valuename );
	if ( valueID == noID ) {
		if ( !create )
			return false;
		mKeys[keyID].names.resize ( mKeys[keyID].names.size() + 1, valuename );
		mKeys[keyID].values.resize ( mKeys[keyID].values.size() + 1, value );
	} else
		mKeys[keyID].values[valueID] = value;

	return true;
}

bool IniFile::setValueI ( std::string const keyname, std::string const valuename, int const value, bool create ) {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer( svalue, MAX_VALUEDATA, "%d", value );
	return setValue ( keyname, valuename, svalue, create );
}

bool IniFile::setValueF ( std::string const keyname, std::string const valuename, double const value, bool create ) {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer ( svalue, MAX_VALUEDATA, "%f", value );
	return setValue ( keyname, valuename, svalue, create );
}

bool IniFile::setValueV ( std::string const keyname, std::string const valuename, char *format, ... ) {
	va_list args;
	char value[MAX_VALUEDATA];

	va_start ( args, format );
#ifdef EE_COMPILER_MSVC
	vsprintf_s( value, MAX_VALUEDATA, format, args );
#else
	vsprintf ( value, format, args );
#endif
	va_end ( args );
	return setValue ( keyname, valuename, value );
}

std::string IniFile::getValue ( unsigned const keyID, unsigned const valueID, std::string const defValue ) const {
	if ( keyID < mKeys.size() && valueID < mKeys[keyID].names.size() )
		return mKeys[keyID].values[valueID];
	return defValue;
}

std::string IniFile::getValue ( std::string const keyname, std::string const valuename, std::string const defValue ) const {
	long keyID = findKey ( keyname );
	if ( keyID == noID )
		return defValue;

	long valueID = findValue ( unsigned ( keyID ), valuename );
	if ( valueID == noID )
		return defValue;

	return mKeys[keyID].values[valueID];
}

int IniFile::getValueI ( std::string const keyname, std::string const valuename, int const defValue ) const {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer ( svalue, MAX_VALUEDATA, "%d", defValue );
	return atoi ( getValue ( keyname, valuename, svalue ).c_str() );
}

bool IniFile::getValueB(const std::string keyname, const std::string valuename, const bool defValue) const {
   std::string val = getValue ( keyname, valuename, defValue ? "1" : "0" );
   char fist = !val.empty() ? val[0] : '0';
   return fist == '1' || fist == 't' || fist == 'y' || fist == 'T' || fist == 'Y';
}

double IniFile::getValueF ( std::string const keyname, std::string const valuename, double const defValue ) const {
	char svalue[MAX_VALUEDATA];

	String::formatBuffer ( svalue, MAX_VALUEDATA, "%f", defValue );
	return atof ( getValue ( keyname, valuename, svalue ).c_str() );
}

// 16 variables may be a bit of over kill, but hey, it's only code.
unsigned IniFile::getValueV ( std::string const keyname, std::string const valuename, char *format,
							   void *v1, void *v2, void *v3, void *v4,
							   void *v5, void *v6, void *v7, void *v8,
							   void *v9, void *v10, void *v11, void *v12,
							   void *v13, void *v14, void *v15, void *v16 ) {
	std::string   value;
	// va_list  args;
	unsigned nVals;


	value = getValue ( keyname, valuename );
	if ( !value.length() )
		return false;
	// Why is there not vsscanf() function. Linux man pages say that there is
	// but no compiler I've seen has it defined. Bummer!
	//
	// va_start( args, format);
	// nVals = vsscanf( value.c_str(), format, args);
	// va_end( args);
#ifdef EE_COMPILER_MSVC
	nVals = (unsigned int)sscanf_s( value.c_str(), format,
					 v1, v2, v3, v4, v5, v6, v7, v8,
					 v9, v10, v11, v12, v13, v14, v15, v16 );
#else
	nVals = (unsigned int)sscanf ( value.c_str(), format,
					 v1, v2, v3, v4, v5, v6, v7, v8,
					 v9, v10, v11, v12, v13, v14, v15, v16 );
#endif
	return nVals;
}

bool IniFile::deleteValue ( std::string const keyname, std::string const valuename ) {
	long keyID = findKey ( keyname );
	if ( keyID == noID )
		return false;

	long valueID = findValue ( unsigned ( keyID ), valuename );
	if ( valueID == noID )
		return false;

	// This looks strange, but is neccessary.
	std::vector<std::string>::iterator npos = mKeys[keyID].names.begin() + valueID;
	std::vector<std::string>::iterator vpos = mKeys[keyID].values.begin() + valueID;
	mKeys[keyID].names.erase ( npos, npos + 1 );
	mKeys[keyID].values.erase ( vpos, vpos + 1 );

	return true;
}

bool IniFile::deleteKey ( std::string const keyname ) {
	long keyID = findKey ( keyname );
	if ( keyID == noID )
		return false;

	std::vector<std::string>::iterator npos = mNames.begin() + keyID;
	std::vector<key>::iterator kpos = mKeys.begin() + keyID;
	mNames.erase ( npos, npos + 1 );
	mKeys.erase ( kpos, kpos + 1 );

	return true;
}

void IniFile::clear() {
	mIniReaded = false;
	mNames.clear();
	mKeys.clear();
	mComments.clear();
}

void IniFile::addHeaderComment ( std::string const comment ) {
	mComments.resize ( mComments.size() + 1, comment );
}

std::string IniFile::getHeaderComment ( unsigned const commentID ) const {
	if ( commentID < mComments.size() )
		return mComments[commentID];
	return "";
}

bool IniFile::deleteHeaderComment ( unsigned commentID ) {
	if ( commentID < mComments.size() ) {
		std::vector<std::string>::iterator cpos = mComments.begin() + commentID;
		mComments.erase ( cpos, cpos + 1 );
		return true;
	}
	return false;
}

unsigned IniFile::getNumKeyComments ( unsigned const keyID ) const {
	if ( keyID < mKeys.size() )
		return (unsigned int)mKeys[keyID].comments.size();
	return 0;
}

unsigned IniFile::getNumKeyComments ( std::string const keyname ) const {
	long keyID = findKey ( keyname );
	if ( keyID == noID )
		return 0;
	return (unsigned int)mKeys[keyID].comments.size();
}

bool IniFile::addKeyComment ( unsigned const keyID, std::string const comment ) {
	if ( keyID < mKeys.size() ) {
		mKeys[keyID].comments.resize ( mKeys[keyID].comments.size() + 1, comment );
		return true;
	}
	return false;
}

bool IniFile::addKeyComment ( std::string const keyname, std::string const comment ) {
	long keyID = findKey ( keyname );
	if ( keyID == noID )
		return false;
	return addKeyComment ( unsigned ( keyID ), comment );
}

std::string IniFile::getKeyComment ( unsigned const keyID, unsigned const commentID ) const {
	if ( keyID < mKeys.size() && commentID < mKeys[keyID].comments.size() )
		return mKeys[keyID].comments[commentID];
	return "";
}

std::string IniFile::getKeyComment ( std::string const keyname, unsigned const commentID ) const {
	long keyID = findKey ( keyname );
	if ( keyID == noID )
		return "";
	return getKeyComment ( unsigned ( keyID ), commentID );
}

bool IniFile::deleteKeyComment ( unsigned const keyID, unsigned const commentID ) {
	if ( keyID < mKeys.size() && commentID < mKeys[keyID].comments.size() ) {
		std::vector<std::string>::iterator cpos = mKeys[keyID].comments.begin() + commentID;
		mKeys[keyID].comments.erase ( cpos, cpos + 1 );
		return true;
	}
	return false;
}

bool IniFile::deleteKeyComment ( std::string const keyname, unsigned const commentID ) {
	long keyID = findKey ( keyname );
	if ( keyID == noID )
		return false;
	return deleteKeyComment ( unsigned ( keyID ), commentID );
}

bool IniFile::deleteKeyComments ( unsigned const keyID ) {
	if ( keyID < mKeys.size() ) {
		mKeys[keyID].comments.clear();
		return true;
	}
	return false;
}

bool IniFile::deleteKeyComments ( std::string const keyname ) {
	long keyID = findKey ( keyname );
	if ( keyID == noID )
		return false;
	return deleteKeyComments ( unsigned ( keyID ) );
}

std::string IniFile::CheckCase ( std::string s ) const {
	if ( mCaseInsensitive )
		for ( std::string::size_type i = 0; i < s.length(); ++i )
			s[i] = std::tolower ( s[i] );
	return s;
}

}}
