#ifndef EE_SYSTEMCRC4_H
#define EE_SYSTEMCRC4_H

#include <eepp/system/base.hpp>

namespace EE { namespace System {

/** @brief RC4 Encryption Class. For more information check Wikipedia: http://en.wikipedia.org/wiki/RC4. */
class EE_API cRC4 {
	public:
		cRC4();

		~cRC4();

		/** @brief Set the encryption Key.
		**	@param key the key data
		**	@param size the key size
		*/
		void SetKey( const Uint8 * key, Uint32 size );
		
		/** @brief Set the encryption Key. */
		void SetKey( const std::vector<Uint8>& Key );
		
		/** @brief Set the encryption Key. */
		void SetKey( const std::string& Key );

		/** @brief Encrypt the buffer ( you must set the key first ).
		**	@param data The buffer to encrypt
		**	@param size The buffer size
		*/
		void EncryptByte( Uint8 * data, Uint32 size );

		/** @brief Encrypt a vector of bytes ( you must set the key first ). */
		void EncryptByte( std::vector<Uint8>& buffer );

		/** @brief Decrypt a vector of bytes ( you must set the key first ). */
		void DecryptByte( std::vector<Uint8>& buffer );
		
		/** @brief Encrypt a string ( you must set the key first ). */
		void EncryptString( std::string& buffer );

		/** @brief Decrypt a string ( you must set the key first ). */
		void DecryptString( std::string& buffer );

		/** @brief Encrypt a file ( you must set the key first ). */
		bool EncryptFile( const std::string& SourceFile, const std::string& DestFile );

		/** @brief Decrypt a file ( you must set the key first ). */
		bool DecryptFile( const std::string& SourceFile, const std::string& DestFile );
	private:
		typedef struct _RC4Key {
			Uint8 state[256];
		} RC4Key;
		RC4Key mKey;
		
		void Swap( Uint8& a, Uint8& b );
};

}}

#endif
