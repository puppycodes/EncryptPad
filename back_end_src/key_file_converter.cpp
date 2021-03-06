#include "key_file_converter.h"
#include "openpgp.h"
#include "botan/base64.h"
#include "botan/exceptn.h"
#include "file_encryption.h"

using namespace Botan;
using namespace LibEncryptMsg;

namespace EncryptPad
{
    bool DecryptKeyFileContent(const std::string &key, EncryptParams *key_file_encrypt_params, std::string &out)
    {
        if(!IsKeyFileEncrypted(key))
        {
            out = key;
            return true;
        }

        // key is encrypted. Key file passphrase is required.
        if(!key_file_encrypt_params)
        {
            return false;
        }

        std::string label;
        DataSource_Memory data_source(key);
        SecureVector<byte> buffer = PGP_decode(data_source, label);
        SecureVector<byte> out_buffer;

        PacketMetadata metadata;
        EpadResult result = DecryptBuffer(buffer, *key_file_encrypt_params, out_buffer, metadata);
        if(result != EpadResult::Success)
            return false;

        out = std::string(reinterpret_cast<const char*>(out_buffer.data()), out_buffer.size());
        return true;
    }

    bool EncryptKeyFileContent(SecureVector<byte> &in_buffer, EncryptParams *key_file_encrypt_params, std::string &out,
            PacketMetadata &metadata)
    {
        assert(key_file_encrypt_params);

        SecureVector<byte> out_buffer;

        EpadResult result = EncryptBuffer(in_buffer, *key_file_encrypt_params, out_buffer, metadata);
        if(result != EpadResult::Success)
            return false;

        out = PGP_encode(out_buffer.data(), out_buffer.size(), "MESSAGE");
        return true;
    }

    bool IsKeyFileEncrypted(const std::string &key)
    {
        static std::string ascii_prefix("-----BEGIN PGP MESSAGE-----");
        return key.size() > ascii_prefix.size()
            && std::equal(ascii_prefix.begin(), ascii_prefix.end(), key.begin());
    }

    bool ValidateDecryptedKeyFile(const std::string &key)
    {
        SecureVector<byte> bytes;
        try
        {
            bytes = base64_decode(key);
        }
        catch(const Botan::Invalid_Argument&)
        {
            return false;
        }
        return !bytes.empty();
    }
}
