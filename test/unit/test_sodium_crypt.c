//
// neuropil is copyright 2016-2018 by pi-lar GmbH
// Licensed under the Open Software License (OSL 3.0), please see LICENSE file for details
//
#include <stdlib.h>
#include <inttypes.h>

#include <criterion/criterion.h>
#include "sodium.h"


#include "../test_macros.c"

#define MESSAGE ((const unsigned char *) "test")
#define MESSAGE_LEN 4
#define CIPHERTEXT_LEN (crypto_secretbox_MACBYTES + MESSAGE_LEN)
 
TestSuite(sodium_crypt );


Test(sodium_crypt, _sodium_crypto_routines, .description="test cryptobox easy usage and creation of ed25519 key/signpairs")
{
	unsigned char node_1_pk[crypto_sign_PUBLICKEYBYTES];
	unsigned char node_1_sk[crypto_sign_SECRETKEYBYTES];
	unsigned char node_2_pk[crypto_sign_PUBLICKEYBYTES];
	unsigned char node_2_sk[crypto_sign_SECRETKEYBYTES];

	unsigned char nonce[crypto_secretbox_NONCEBYTES];

	cr_expect(0 == crypto_sign_keypair(node_1_pk, node_1_sk)); // ed25519
	cr_expect(0 == crypto_sign_keypair(node_2_pk, node_2_sk)); // ed25519

	// convert to curve key
	unsigned char node_1_curve_sk[crypto_scalarmult_curve25519_BYTES];
	unsigned char node_2_curve_sk[crypto_scalarmult_curve25519_BYTES];
	cr_expect(0 == crypto_sign_ed25519_sk_to_curve25519(node_1_curve_sk, node_1_sk));
	cr_expect(0 == crypto_sign_ed25519_pk_to_curve25519(node_2_curve_sk, node_2_sk));
	//
	unsigned char node_1_dh_pk[crypto_scalarmult_BYTES];
	cr_expect(0 == crypto_scalarmult_base(node_1_dh_pk, node_1_curve_sk));

	unsigned char node_2_dh_pk[crypto_scalarmult_BYTES];
	cr_expect(0 == crypto_scalarmult_base(node_2_dh_pk, node_2_curve_sk));


	unsigned char node_1_shared[crypto_scalarmult_BYTES];
	cr_expect(0 == crypto_scalarmult(node_1_shared, node_1_curve_sk, node_2_dh_pk));
	unsigned char node_2_shared[crypto_scalarmult_BYTES];
	cr_expect(0 == crypto_scalarmult(node_2_shared, node_2_curve_sk, node_1_dh_pk));

	// crypt it
	// unsigned char nonce[crypto_secretbox_NONCEBYTES];
	// unsigned char key[crypto_secretbox_KEYBYTES];
	unsigned char ciphertext[CIPHERTEXT_LEN];

	randombytes_buf(nonce, sizeof nonce);

	cr_expect(0 == crypto_secretbox_easy(ciphertext, MESSAGE, MESSAGE_LEN, nonce, node_1_shared));

	unsigned char decrypted[MESSAGE_LEN];
	cr_expect(0 == crypto_secretbox_open_easy(decrypted, ciphertext, CIPHERTEXT_LEN, nonce, node_2_shared), "could not decrypt");
	    
}


#define MESSAGE_PART1 ((const unsigned char *) "Arbitrary data to hash")
#define MESSAGE_PART1_LEN 22
#define MESSAGE_PART2 ((const unsigned char *) "is longer than expected")
#define MESSAGE_PART2_LEN 23
#define MESSAGE_PART3 ((const unsigned char *) "and may get even longer!")
#define MESSAGE_PART3_LEN 24


Test(sodium_crypt, _concat_hash_values, .description="test whether hashing can be concatenated")
{
	unsigned char hash_1[crypto_generichash_BYTES];
	unsigned char hash_2[crypto_generichash_BYTES];
	unsigned char hash_3[crypto_generichash_BYTES];
	unsigned char key[crypto_generichash_KEYBYTES];

	crypto_generichash_state state_1, state_2, state_3;

	randombytes_buf(key, sizeof key);

	// calculate combined hash
	crypto_generichash_init(&state_1, NULL, 0, sizeof hash_1);
	crypto_generichash_update(&state_1, MESSAGE_PART1, MESSAGE_PART1_LEN);
	crypto_generichash_update(&state_1, MESSAGE_PART2, MESSAGE_PART2_LEN);
	crypto_generichash_update(&state_1, MESSAGE_PART3, MESSAGE_PART3_LEN);
	crypto_generichash_final(&state_1, hash_1, sizeof hash_1);


	// first calculate only part of the hash
	crypto_generichash_init(&state_2, NULL, 0, sizeof hash_2);
	crypto_generichash_update(&state_2, MESSAGE_PART1, MESSAGE_PART1_LEN);
	crypto_generichash_update(&state_2, MESSAGE_PART2, MESSAGE_PART2_LEN);
	crypto_generichash_final(&state_2, hash_2, sizeof hash_2);


	// next add a value to this hash part
	crypto_generichash_init(&state_3, NULL, 0, sizeof hash_3);
	crypto_generichash_update(&state_3, MESSAGE_PART3, MESSAGE_PART3_LEN);
	crypto_generichash_update(&state_3, hash_2, crypto_generichash_BYTES);
	crypto_generichash_final(&state_3, hash_3, sizeof hash_3);

	cr_expect(0 != sodium_memcmp(hash_1, hash_3, crypto_generichash_BYTES), "test whether we can concat hash values");

}

 
Test(sodium_crypt, check_dhke, .description = "test the reowked crypto system")
{
	/*
	Our encyrption has 3 stages
	1. Handshake - no encryption available, dhke
	2. Transport encryption 
	3. E2E encryption & data signatures
	*/
	cr_assert(sodium_init() != -1,"Could not init sodium");

	// generate keypairs
	unsigned char ed25519_nodeA_public_key[crypto_sign_ed25519_PUBLICKEYBYTES];		   // actual token data
	unsigned char ed25519_nodeA_secret_key[crypto_sign_ed25519_SECRETKEYBYTES];		   // actual token data
	unsigned char ed25519_identA_public_key[crypto_sign_ed25519_PUBLICKEYBYTES];	   // actual token data
	unsigned char ed25519_identA_secret_key[crypto_sign_ed25519_SECRETKEYBYTES];	   // actual token data
	crypto_sign_ed25519_keypair(ed25519_nodeA_public_key, ed25519_nodeA_secret_key);   // actual token data
	crypto_sign_ed25519_keypair(ed25519_identA_public_key, ed25519_identA_secret_key); // actual token data
																					   // actual token data
	unsigned char ed25519_nodeB_public_key[crypto_sign_ed25519_PUBLICKEYBYTES];		   // actual token data
	unsigned char ed25519_nodeB_secret_key[crypto_sign_ed25519_SECRETKEYBYTES];		   // actual token data
	unsigned char ed25519_identB_public_key[crypto_sign_ed25519_PUBLICKEYBYTES];	   // actual token data
	unsigned char ed25519_identB_secret_key[crypto_sign_ed25519_SECRETKEYBYTES];	   // actual token data
	crypto_sign_ed25519_keypair(ed25519_nodeB_public_key, ed25519_nodeB_secret_key);   // actual token data
	crypto_sign_ed25519_keypair(ed25519_identB_public_key, ed25519_identB_secret_key); // actual token data

	unsigned char nodeA_public_key[crypto_kx_PUBLICKEYBYTES], nodeA_secret_key[crypto_kx_SECRETKEYBYTES];	// calculated token data
	unsigned char nodeB_public_key[crypto_kx_PUBLICKEYBYTES], nodeB_secret_key[crypto_kx_SECRETKEYBYTES];	// calculated token data
	crypto_sign_ed25519_pk_to_curve25519(nodeA_public_key, ed25519_nodeA_public_key);						// calculated token data
	crypto_sign_ed25519_sk_to_curve25519(nodeA_secret_key, ed25519_nodeA_secret_key);						// calculated token data
	crypto_sign_ed25519_pk_to_curve25519(nodeB_public_key, ed25519_nodeB_public_key);						// calculated token data
	crypto_sign_ed25519_sk_to_curve25519(nodeB_secret_key, ed25519_nodeB_secret_key);						// calculated token data
																											// calculated token data
	unsigned char identA_public_key[crypto_kx_PUBLICKEYBYTES], identA_secret_key[crypto_kx_SECRETKEYBYTES];	// calculated token data
	unsigned char identB_public_key[crypto_kx_PUBLICKEYBYTES], identB_secret_key[crypto_kx_SECRETKEYBYTES];	// calculated token data
	crypto_sign_ed25519_pk_to_curve25519(identA_public_key, ed25519_identA_public_key);						// calculated token data
	crypto_sign_ed25519_sk_to_curve25519(identA_secret_key, ed25519_identA_secret_key);						// calculated token data
	crypto_sign_ed25519_pk_to_curve25519(identB_public_key, ed25519_identB_public_key);						// calculated token data
	crypto_sign_ed25519_sk_to_curve25519(identB_secret_key, ed25519_identB_secret_key);						// calculated token data
	
	// all 8 keypairs are now available 2x(sign_node/crypto_node, sign_ident/crypto_ident)

	// Node-A initiates Handshake and sends nodeA_public_key to Node-B

	// On Node-B
	unsigned char nodeB_session_key_to_read[crypto_kx_SESSIONKEYBYTES], nodeB_session_key_to_write[crypto_kx_SESSIONKEYBYTES];
	// Node-B receives Handshake and so has to apply the role of the server
	cr_assert(crypto_kx_server_session_keys(nodeB_session_key_to_read, nodeB_session_key_to_write,
		nodeB_public_key, nodeB_secret_key, nodeA_public_key) == 0, "Suspicious public key, bail out");
	// Node-B can now receive encyrpted data from Node-A
	// Node-B sends his own public key to Node-A to enable a 2way communication

	// On Node-A
	unsigned char nodeA_session_key_to_read[crypto_kx_SESSIONKEYBYTES], nodeA_session_key_to_write[crypto_kx_SESSIONKEYBYTES];
	// Node-A receives Handshake and so has to apply the role of the client
	cr_assert(crypto_kx_client_session_keys(nodeA_session_key_to_read, nodeA_session_key_to_write,
		nodeA_public_key, nodeA_secret_key, nodeB_public_key) == 0, "Suspicious public key, bail out");



	// The handshake is now complete, lets talk about transport encyption (aka. general DataExchange):



	// Node-A now wants to send Data to Node-B
	// On Node-A
	const int message_len = 1024 - crypto_secretbox_MACBYTES;

	unsigned char data_from_node_a[message_len];	
	randombytes_buf(data_from_node_a, sizeof data_from_node_a); // generate data

	unsigned char nonce_from_node_a[crypto_secretbox_NONCEBYTES];
	randombytes_buf(nonce_from_node_a, sizeof nonce_from_node_a);
	unsigned char encyrypted_data[1024] = { 0 };
	cr_assert(0 == crypto_secretbox_easy(
		encyrypted_data,
		data_from_node_a, 
		message_len, 
		nonce_from_node_a, 
		nodeA_session_key_to_write),		
		"Could not encypt data"
	);
	// Node-A now sends the encyrypted_data as well as the nonce to Node-B
	// On Node-B
	unsigned char decyrypted_data[1024] = { 0 };
	cr_assert(0 == crypto_secretbox_open_easy(
		decyrypted_data,
		encyrypted_data,
		1024, 
		nonce_from_node_a, 
		nodeB_session_key_to_read), 
		"Could not decypt data"
	);
	cr_assert(memcmp(decyrypted_data, data_from_node_a, message_len) == 0, "Decrypted data does not match.");
	//TODO: invert roles of A and B


	// The DataExchange is now complete, lets talk about E2E encryption :

	// E2E is always over the identity of the nodes (for simplicity we use the identA on nodeA...)
	
	// If Node-B wants to send Node-A a message both need to exchange an AAAToken containing their own ident public_key
	// As well as the actual E2E encrypted message + nonce
	// To send the same message to multiple receipients while it is still e2e encrypted we use a intermediate key
	// The intermediate key encrypts the actual message. the intermediate secret key is then 
	// encrypted with every reciepents public key and prependet to the actual message.
	// as a downside to this the message may bloat for every recipient

	unsigned char message_from_node_b[235];
	randombytes_buf(message_from_node_b, sizeof message_from_node_b); // generate message
	unsigned char message_from_node_b_nonce[crypto_secretbox_NONCEBYTES];
	randombytes_buf(message_from_node_b_nonce, sizeof message_from_node_b_nonce);

	unsigned char intermediate_key[crypto_secretbox_KEYBYTES];
	// Generate Keypair on Node-B
	crypto_secretbox_keygen(intermediate_key);
	unsigned char intermediate_nonce[crypto_secretbox_NONCEBYTES];
	randombytes_buf(intermediate_nonce, sizeof intermediate_nonce);

	// Node-B now uses the public key of Node-A to encrypt the intermediate key
	unsigned char encyrypted_intermediate_key[crypto_secretbox_KEYBYTES + crypto_secretbox_MACBYTES] = { 0 };
	cr_assert(0 == crypto_box_easy(
		encyrypted_intermediate_key,
		intermediate_key,
		sizeof intermediate_key,
		intermediate_nonce,
		identA_public_key, 
		identB_secret_key),
		"Could not encypt intermediate key"
	);
	unsigned char encyrypted_message[sizeof(message_from_node_b) + crypto_secretbox_MACBYTES] = { 0 };
	// And encrypts the message with the intermediate key
	cr_assert(0 == crypto_secretbox_easy(
		encyrypted_message,
		message_from_node_b,
		sizeof message_from_node_b,
		message_from_node_b_nonce,
		intermediate_key),
		"Could not encypt message with intermediate key"
	);
	// Node-B now sends
	//  - encyrypted_intermediate_key
	//  - intermediate_nonce
	//  - encyrypted_message
	//  - message_from_node_b_nonce
	// to Node-A via DataExchange

	// Now Node-A needs to decrypt the message. We assume the DataExchange was already done
	

	unsigned char decyrypted_intermediate_key[crypto_secretbox_KEYBYTES] = { 0 };
	cr_assert(0 == crypto_box_open_easy(
		decyrypted_intermediate_key,
		encyrypted_intermediate_key,
		sizeof encyrypted_intermediate_key,
		intermediate_nonce,
		identB_public_key,
		identA_secret_key),
		"Could not decypt intermediate key data"
	);
	//TODO: assumption: Node-B knows message size ...
	unsigned char decrypted_message_from_node_b[235];
	cr_assert(0 == crypto_secretbox_open_easy(
		decrypted_message_from_node_b,
		encyrypted_message,
		sizeof encyrypted_message,
		message_from_node_b_nonce,
		decyrypted_intermediate_key),
		"Could not decypt message"
	);

	cr_assert(0 == memcmp(decrypted_message_from_node_b, message_from_node_b, sizeof message_from_node_b),
		"Message is not the same");

	// Now lets create a signature for data_from_node_a (or any data for this matter) to verify the system
	unsigned char sig[crypto_sign_BYTES];
	cr_assert(0 ==
		crypto_sign_detached(sig, NULL, data_from_node_a, sizeof data_from_node_a, ed25519_identA_secret_key),
		"Signature could not be created");

	cr_assert(0 == 
		crypto_sign_verify_detached(sig, data_from_node_a, sizeof data_from_node_a, ed25519_identA_public_key),
		"Signature could not be verified"
	);
}