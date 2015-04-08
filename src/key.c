#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
// #include <openssl/evp.h>

#include "sodium.h"
#include "key.h"

#include "base.h"
#include "log.h"


void key_print (Key* k)
{
    int i;
    char hexstr[KEY_SIZE];	// this is big just to be safe
    char base4str[KEY_SIZE];	// 

    for (i = 4; i >= 0; i--) sprintf (hexstr, "%08x", (unsigned int) k->t[i]);

    if (IS_BASE_16)
	{
	    for (i = 0; i < strlen (hexstr); i++)
		{
		    if (i % 8 == 0) printf (" ");
		    printf ("%c", hexstr[i]);
		}
	}
    else if (IS_BASE_4)
	{
	    hex_to_base4 (hexstr, base4str);

	    for (i = 0; i < strlen (base4str); i++)
		{
		    if (i % 16 == 0) printf (" ");
		    printf ("%c", base4str[i]);
		}
	}
    else
	{
	    log_msg (LOG_WARN, "Unknown base");
	}
    printf ("\n");
}

void key_to_str (Key* k)
{
    k->valid = 0;
    // log_msg(LOG_WARN, "key %0lu %0lu %0lu %0lu", k->t[0], k->t[1], k->t[2], k->t[3]);
	// log_msg(LOG_WARN, "key %016lx %016lx %016lx %016lx", k->t[0], k->t[1], k->t[2], k->t[3]);
	memset  (k->keystr, 0, 64);
	sprintf ((char*) k->keystr, "%016lx%016lx%016lx%016lx", k->t[0], k->t[1], k->t[2], k->t[3]);
	k->keystr[64] = '\0';
	k->valid = 1;
    log_msg (LOG_DEBUG, "key string now: %s", k->keystr);
}

void str_to_key (Key *k, const char* key_string)
{
	// TODO: this is dangerous, encoding could be different between systems,
	// encoding has to be send over teh wire to be sure ...
	// for now: all tests on the same system
    k->valid = 0;

    memset (k->keystr, 0, 64);
    memcpy (k->keystr, key_string, 64);
    k->keystr[64] = '\0';

    for (int i = 0; i < 4; i++) {
    	unsigned char substring[17];
    	memcpy(substring, k->keystr + i*16, 16);
    	substring[16] = '\0';
    	k->t[i] = strtoul((const char*)substring, NULL, 16);
        // log_msg(LOG_KEYDEBUG, "keystr substring to ul: %s -> %ul ", substring, kResult->t[i]);
    }

    k->valid = 1;
}

unsigned char* key_generate_hash (const unsigned char* key_in, size_t digest_size, unsigned char* digest_out)
{
    unsigned char md_value[32]; //  = (unsigned char *) malloc (32);
    unsigned int md_len, i;
    char digit[10];
    unsigned char *tmp;

    crypto_hash_sha256(md_value, key_in, digest_size);
    log_msg (LOG_KEYDEBUG, "md value (%s) now: [%s]", key_in, md_value);

    // TODO: move it to KECCAK because of possible length extension attack ???
    // TODO: move to SHA-2 at least ?
//    crypto_hash_sha256_state state;
//    crypto_hash_sha256_init(&state);
//    crypto_hash_sha256_update(&state, key_in, sizeof(key_in));
//    crypto_hash_sha256_final(&state, tmp);
//    log_msg (LOG_KEYDEBUG, "md value (%s) now: [%s]", key_in, tmp);

    digest_out = (unsigned char *) malloc (65);
    // digest = strndup(md_value, 64);
    // printf("key.c:sha1_keygen digest %s\n", digest);

    tmp = digest_out;
    *tmp = '\0';
    for (i = 0; i < 32; i++)
	{
    	convert_base16 (md_value[i], digit);
    	// memcpy(tmp, digit, sizeof(digit));
	    strcat ((char*)tmp, digit);
	    tmp = tmp + strlen (digit);
	}
    digest_out[64] = '\0';
    return digest_out;
}


Key* key_create_from_hostport(const char* strOrig, int port) {

	unsigned char name[256];
	snprintf ((char*) name, 255, "%s:%d", strOrig, port);
	unsigned char* digest = NULL;

	digest = key_generate_hash (name, strlen ((char*) name) * sizeof (char), digest);
	log_msg (LOG_KEYDEBUG, "digest calculation returned HASH: %s", digest);

	Key* tmp = key_create_from_hash(digest);
	log_msg (LOG_KEYDEBUG, "HASH(%s) = [%s]", name, key_get_as_string(tmp));

	return tmp;
}

Key* key_create_from_hash(const unsigned char* strOrig)
// void str_to_key (const char *strOrig, Key * k)
{
	int i, len;
	Key* kResult = (Key*) malloc(sizeof(Key));

	kResult->valid = 0;

	memset (kResult->keystr, 0, 64);
    memcpy (kResult->keystr, strOrig, 64);
    kResult->keystr[64] = '\0';

    for (i = 0; i < 4; i++) {
    	unsigned char substring[17];
    	memcpy(substring, kResult->keystr + i*16, 16);
    	substring[16] = '\0';
    	kResult->t[i] = strtoul((const char*)substring, NULL, 16);
        // log_msg(LOG_KEYDEBUG, "keystr substring to ul: %s -> %ul ", substring, kResult->t[i]);
    }

    kResult->valid = 1;

    return kResult;
}


void key_assign (Key* k1, const Key* const k2)
{
    int i;
    for (i = 0; i < 4; i++)
    	k1->t[i] = k2->t[i];
    key_to_str (k1);
}

void key_assign_ui (Key* k, unsigned long ul)
{
    int i;
    for (i = 1; i < 3; i++)
    	k->t[i] = 0;
    k->t[3] = ul;
    key_to_str (k);
}

int key_equal (Key* k1, Key* k2)
{
    int i;
    for (i = 0; i < 4; i++)
    	if (k1->t[i] != k2->t[i])
    		return 0;
    return 1;
}

int key_equal_ui (Key* k, unsigned long ul)
{
	log_msg (LOG_KEYDEBUG, "deprecated");
    int i;
    if (k->t[3] != ul) return (0);
    for (i = 2; i >= 0; i--)
    	if (k->t[i] != 0)
    		return 0;
    return 1;
}

int key_comp (const Key* const k1, const Key* const k2)
{

    int i;

    for (i = 0; i < 4; i++)
	{
	    if 		(k1->t[i] > k2->t[i]) return (1);
	    else if (k1->t[i] < k2->t[i]) return (-1);
	}
    return (0);
}

void key_add (Key* result, const Key* const op1, const Key* const op2)
{

    double tmp, a, b;
    int i;
    a = b = tmp = 0;

    for (i = 3; i >= 0; i--)
	{
	    a = op1->t[i];
	    b = op2->t[i];
	    tmp += a + b;

	    if (tmp > ULONG_MAX) tmp = 1; // tmp - ULONG_MAX;
	    else                 tmp = 0;

	    result->t[i] = (unsigned long) tmp;
	}

    result->valid = 0;
}

void key_sub (Key* result, const Key* const op1, const Key* const op2)
{
    int i;
    double tmp, a, b, carry;
    Key key_tmp, key_a, key_b;
    int swapped = 0;

    carry = 0;

    key_assign(&key_a, op1);
	key_assign(&key_b, op2);

    if (key_comp (&key_a, &key_b) < 0)
	{
    	// swap keys and do the calculation
    	key_assign(&key_tmp, &key_a);
    	key_assign(&key_a, &key_b);
    	key_assign(&key_b, &key_tmp);

    	swapped = 1;
	}

    for (i = 3; i >= 0; i--)
	{
	    a = key_a.t[i] - carry;
	    b = key_b.t[i];

	    if (b <= a)
		{
		    tmp = a - b;
		    carry = 0;
		}
	    else
		{
		    a = a + ULONG_MAX + 1;
		    tmp = a - b;
		    carry = 1;
		}
	    result->t[i] = (unsigned long) tmp;
	}

    if (swapped) {
    	key_assign(&key_tmp, result);
    	key_sub(result, &Key_Max, &key_tmp);
    }

    result->valid = 0;
}


// void key_makehash (Key* hashed, char *s)
// {
// key_make_hash (hashed, s, strlen (s) * sizeof (char));
// log_msg (LOG_KEYDEBUG, "key_makehash: HASH( %s ) = [%s]", s, get_key_string (hashed));
// }


// void key_make_hash (Key* hashed, char *s, size_t size)
// {
//    char *digest;
//    int i;
//
//    digest = sha1_keygen (s, size, NULL);
//    // printf("key.c:key_make_hash:digest %s\n", digest);
//    str_to_key (digest, hashed);
//    log_msg(LOG_DEBUG, "hashed %s", hashed->keystr);
//
//    //for(i=0; i < 4; i++) sscanf(digest+(i*8*sizeof(char)),"%08x",&hashed->t[(4-i)]);
//    //key_to_str(hashed->keystr,*hashed);
//
//    free (digest);
// }

void key_init ()
{
    int i;
    for (i = 0; i < 4; i++)
	{
	    Key_Max.t[i] = ULONG_MAX;
	    Key_Half.t[i] = ULONG_MAX;
	}

    Key_Half.t[0] = Key_Half.t[0] / 2;

    key_to_str (&Key_Max);
    key_to_str (&Key_Half);

}

void key_distance (Key* diff, const Key* const k1, const Key* const k2)
{
    int comp;

    // comp = key_comp (k1, k2);
    /* k1 > k2 */
    // if (comp > 0) key_sub (diff, k1, k2);
    // else
    // Key key_tmp_1, key_tmp_2;
    // key_sub (&key_tmp_1, k1, k2);
    // key_sub (&key_tmp_2, k2, k1);
    key_sub (diff, k1, k2);

    // if (key_comp(&key_tmp_1, &key_tmp_2) >= 0) key_assign(diff, &key_tmp_2);
    // else key_assign(diff, &key_tmp_1);

    // comp = key_comp (diff, &Key_Half);
    /* diff > Key_Half */
    // if (comp > 0) key_sub (diff, &Key_Max, diff);

    diff->valid = 0;
    // log_msg(LOG_KEYDEBUG, "key_distance %s to %s = %s", key_get_as_string((Key*) k1), key_get_as_string((Key*)k2), key_get_as_string(diff));
}


int key_between (const Key* const test, const Key* const left, const Key* const right)
{

    int complr = key_comp (left, right);
    int complt = key_comp (left, test);
    int comptr = key_comp (test, right);

    /* it's on one of the edges */
    if (complt == 0 || comptr == 0) return (1);

    if (complr < 0)
	{
	    if (complt < 0 && comptr < 0) return (1);
	    return (0);
	}
    else if (complr == 0)
	{
	    return (0);
	}
    else
	{
	    if (complt < 0 || comptr < 0)
		return (1);
	    return (0);

	}
}

// Return the string representation of key
// This function should be used instead of directly accessing the keystr field
unsigned char* key_get_as_string (Key* key)
{
    if (!key->valid)
	{
	    key_to_str (key);
	    key->valid = 1;
	}
    return key->keystr;
}

void key_midpoint (Key* mid, Key* key)
{

    if (key_comp (key, &Key_Half) < 0) key_add (mid, key, &Key_Half);
    else 							   key_sub (mid, key, &Key_Half);

    mid->valid = 0;
}


int key_index (Key* mykey, Key* k)
{
    int max_len, i;
    unsigned char mystr[65];
    unsigned char kstr[65];
    max_len = KEY_SIZE / BASE_B;
    memcpy (mystr, key_get_as_string (mykey), 64);
    memcpy (kstr, key_get_as_string (k), 64);

    for (i = 0; (mystr[i] == kstr[i]) && (i < max_len); i++);

    if (i == max_len) i = max_len - 1;

    // log_msg (LOG_KEYDEBUG, "key_index:%d me:%s lookup_key:%s", i, mykey->keystr, k->keystr);
    return (i);
}

int power (int base, int n)
{
    int i, p = 1;

    for (i = 1; i <= n; i++)
    	p = p * base;
    return p;
}


