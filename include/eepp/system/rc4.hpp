#ifndef EE_SYSTEMCRC4_H
#define EE_SYSTEMCRC4_H

#include <eepp/config.hpp>
#include <string>
#include <vector>

namespace EE { namespace System {

/** @brief RC4 Encryption Class. For more information check Wikipedia:
 * http://en.wikipedia.org/wiki/RC4. */
class EE_API RC4 {
  public:
	RC4();

	~RC4();

	/** @brief Set the encryption Key.
	**	@param key the key data
	**	@param size the key size
	*/
	void setKey( const Uint8* key, Uint32 size );

	/** @brief Set the encryption Key. */
	void setKey( const std::vector<Uint8>& Key );

	/** @brief Set the encryption Key. */
	void setKey( const std::string& Key );

	/** @brief Encrypt the buffer ( you must set the key first ).
	**	@param data The buffer to encrypt
	**	@param size The buffer size
	*/
	void encryptByte( Uint8* data, Uint32 size );

	/** @brief Encrypt a vector of bytes ( you must set the key first ). */
	void encryptByte( std::vector<Uint8>& buffer );

	/** @brief Decrypt a vector of bytes ( you must set the key first ). */
	void decryptByte( std::vector<Uint8>& buffer );

	/** @brief Encrypt a string ( you must set the key first ). */
	void encryptString( std::string& buffer );

	/** @brief Decrypt a string ( you must set the key first ). */
	void decryptString( std::string& buffer );

	/** @brief Encrypt a file ( you must set the key first ). */
	bool encryptFile( const std::string& SourceFile, const std::string& DestFile );

	/** @brief Decrypt a file ( you must set the key first ). */
	bool decryptFile( const std::string& SourceFile, const std::string& DestFile );

  private:
	struct RC4Key {
		Uint8 state[256];
	};
	RC4Key mKey;

	void swap( Uint8& a, Uint8& b );
};

}} // namespace EE::System

#endif
