//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <crypto/keypair.h>

#include <iostream>  // TODO: remove only for debug

#include <eccrypto.h>  // for cryptopp ECC encryption
#include <oids.h>      // for cryptopp ECC curve function
#include <osrng.h>     // for cryptopp random number generation

namespace blocxxi {
namespace crypto {

namespace cr = CryptoPP;

namespace {
using PrivateKeyType = cr::ECDSA<cr::ECP, cr::SHA256>::PrivateKey;
using PublicKeyType = cr::ECDSA<cr::ECP, cr::SHA256>::PublicKey;

void MakeRandomPrivateKey(PrivateKeyType &private_key) {
  // Generate a new random private key
  cr::AutoSeededRandomPool prng;

  do {
    private_key.Initialize(prng, cr::ASN1::secp256k1());
    // Level denotes the level of thoroughness: 0 - using this object won't
    // cause a crash or exception (rng is ignored); 1 - this object will
    // probably function (encrypt, sign, etc.) correctly (but may not check for
    // weak keys and such); 2 - make sure this object will function correctly,
    // and do reasonable security checks; 3 - do checks that may take a long
    // time.
  } while (!private_key.Validate(prng, 3));
}

void MakePrivateKeyFromSecret(PrivateKeyType &private_key,
                              const KeyPair::PrivateKey &secret) {
  // Initialize the PrivateKey from the given private exponent
  cr::Integer privateExponent(secret.Data(), secret.Size(),
                              cr::Integer::UNSIGNED,
                              cr::ByteOrder::BIG_ENDIAN_ORDER);
  private_key.Initialize(cr::ASN1::secp256k1(), privateExponent);
}

void DerivePublicKey(PublicKeyType &public_key,
                     const PrivateKeyType &private_key) {
  // Derive the public key
  private_key.MakePublicKey(public_key);
}

void UpdateSecret(KeyPair::PrivateKey &hash,
                  const PrivateKeyType &private_key) {
  // Save the private exponent part of the key in our secret part
  auto const &pex = private_key.GetPrivateExponent();
  pex.Encode(hash.Data(), 32);
}

void UpdatePublic(KeyPair::PublicKey &hash, const PublicKeyType &public_key) {
  // Save the public key in our public part (which must be 64 bytes long)
  auto q = public_key.GetPublicElement();
  auto qx = q.x;
  auto qy = q.y;
  qx.Encode(hash.Data(), 32);
  qy.Encode(hash.Data() + 32, 32);
}

}  // namespace

KeyPair::KeyPair() {
  PrivateKeyType private_key;
  PublicKeyType public_key;
  try {
    MakeRandomPrivateKey(private_key);
    DerivePublicKey(public_key, private_key);
  } catch (std::exception &ex) {
    // Change to log
    std::cerr << "Failed to initialize KeyPair: " << ex.what() << std::endl;
    throw;
  }

  // Non exception throwing code
  UpdateSecret(secret_, private_key);
  UpdatePublic(public_, public_key);
}

KeyPair::KeyPair(const KeyPair::PrivateKey &secret) {
  PrivateKeyType private_key;
  PublicKeyType public_key;
  try {
    MakePrivateKeyFromSecret(private_key, secret);
    DerivePublicKey(public_key, private_key);
  } catch (std::exception &ex) {
    // Change to log
    std::cerr << "Failed to initialize KeyPair: " << ex.what() << std::endl;
    throw;
  }

  // Non exception throwing code
  secret_ = secret;
  UpdatePublic(public_, public_key);
}

KeyPair::KeyPair(const std::string &secret_hex) {
  PrivateKeyType private_key;
  PublicKeyType public_key;
  try {
    // Don't reverse so that the encoded data is big endian
    auto secret = KeyPair::PrivateKey::FromHex(secret_hex, false);
    MakePrivateKeyFromSecret(private_key, secret);
    DerivePublicKey(public_key, private_key);
  } catch (const std::exception &ex) {
    // Change to log
    std::cerr << "Failed to initialize KeyPair: " << ex.what() << std::endl;
    throw;
  }

  // Non exception throwing code
  // Use the clean secret from the private key
  UpdateSecret(secret_, private_key);
  UpdatePublic(public_, public_key);
}

}  // namespace crypto
}  // namespace blocxxi