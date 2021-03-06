/* crypto/evp/evp.h */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#ifndef HEADER_ENVELOPE_H
#define HEADER_ENVELOPE_H

#include "openssl_aes.h"
#include "openssl_des.h"
#include "openssl_md5.h"
#include "openssl_sha.h"

/*
#define EVP_RC2_KEY_SIZE		16
#define EVP_RC4_KEY_SIZE		16
#define EVP_BLOWFISH_KEY_SIZE		16
#define EVP_CAST5_KEY_SIZE		16
#define EVP_RC5_32_12_16_KEY_SIZE	16
*/
#define EVP_MAX_MD_SIZE			64	/* longest known is SHA512 */
#define EVP_MAX_KEY_LENGTH		32
#define EVP_MAX_IV_LENGTH		16
#define EVP_MAX_BLOCK_LENGTH		32


#define EVP_PK_RSA	0x0001
#define EVP_PK_DSA	0x0002
#define EVP_PK_DH	0x0004
#define EVP_PK_EC	0x0008
#define EVP_PKT_SIGN	0x0010
#define EVP_PKT_ENC	0x0020
#define EVP_PKT_EXCH	0x0040
#define EVP_PKS_RSA	0x0100
#define EVP_PKS_DSA	0x0200
#define EVP_PKS_EC	0x0400
#define EVP_PKT_EXP	0x1000 /* <= 512 bit key */

#define EVP_PKEY_NONE	NID_undef
#define EVP_PKEY_RSA	NID_rsaEncryption
#define EVP_PKEY_RSA2	NID_rsa
#define EVP_PKEY_DSA	NID_dsa
#define EVP_PKEY_DSA1	NID_dsa_2
#define EVP_PKEY_DSA2	NID_dsaWithSHA
#define EVP_PKEY_DSA3	NID_dsaWithSHA1
#define EVP_PKEY_DSA4	NID_dsaWithSHA1_2
#define EVP_PKEY_DH	NID_dhKeyAgreement
#define EVP_PKEY_EC	NID_X9_62_id_ecPublicKey
#define EVP_PKEY_HMAC	NID_hmac

#ifdef	__cplusplus
extern "C" {
#endif

    typedef struct evp_cipher_st EVP_CIPHER;
    typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
    typedef struct env_md_st EVP_MD;
    typedef struct env_md_ctx_st EVP_MD_CTX;
    typedef struct evp_pkey_st EVP_PKEY;
    typedef struct engine_st ENGINE;

    struct env_md_st {
        int type;
        int pkey_type;
        int md_size;
        unsigned long flags;
        int (*init)(EVP_MD_CTX *ctx);
        int (*update)(EVP_MD_CTX *ctx,const void *data,size_t count);
        int (*final)(EVP_MD_CTX *ctx,unsigned char *md);
        int (*copy)(EVP_MD_CTX *to,const EVP_MD_CTX *from);
        int (*cleanup)(EVP_MD_CTX *ctx);

        /* FIXME: prototype these some day */
        int (*sign)(int type, const unsigned char *m, unsigned int m_length,
                    unsigned char *sigret, unsigned int *siglen, void *key);
        int (*verify)(int type, const unsigned char *m, unsigned int m_length,
                      const unsigned char *sigbuf, unsigned int siglen,
                      void *key);
        int required_pkey_type[5]; /*EVP_PKEY_xxx */
        int block_size;
        int ctx_size; /* how big does the ctx->md_data need to be */
    } /* EVP_MD */;

    typedef int evp_sign_method(int type,const unsigned char *m,
                                unsigned int m_length,unsigned char *sigret,
                                unsigned int *siglen, void *key);
    typedef int evp_verify_method(int type,const unsigned char *m,
                                  unsigned int m_length,const unsigned char *sigbuf,
                                  unsigned int siglen, void *key);

    typedef struct {
        EVP_MD_CTX *mctx;
        void *key;
    } EVP_MD_SVCTX;

#define EVP_MD_FLAG_ONESHOT	0x0001 /* digest can only handle a single
    * block */

#define EVP_MD_FLAG_FIPS	0x0400 /* Note if suitable for use in FIPS mode */

#define EVP_MD_FLAG_SVCTX	0x0800 /* pass EVP_MD_SVCTX to sign/verify */

#define EVP_PKEY_NULL_method	NULL,NULL,{0,0,0,0}


#define EVP_PKEY_DSA_method	EVP_PKEY_NULL_method


#define EVP_PKEY_ECDSA_method   EVP_PKEY_NULL_method

#define EVP_PKEY_RSA_method	EVP_PKEY_NULL_method
#define EVP_PKEY_RSA_ASN1_OCTET_STRING_method EVP_PKEY_NULL_method


    struct env_md_ctx_st {
        const EVP_MD *digest;
        ENGINE *engine; /* functional reference if 'digest' is ENGINE-provided */
        unsigned long flags;
        void *md_data;
    } /* EVP_MD_CTX */;

    /* values for EVP_MD_CTX flags */

#define EVP_MD_CTX_FLAG_ONESHOT		0x0001 /* digest update will be called
    * once only */
#define EVP_MD_CTX_FLAG_CLEANED		0x0002 /* context has already been
    * cleaned */
#define EVP_MD_CTX_FLAG_REUSE		0x0004 /* Don't free up ctx->md_data
    * in EVP_MD_CTX_cleanup */
#define EVP_MD_CTX_FLAG_NON_FIPS_ALLOW	0x0008	/* Allow use of non FIPS digest
    * in FIPS mode */

#define EVP_MD_CTX_FLAG_PAD_MASK	0xF0	/* RSA mode to use */
#define EVP_MD_CTX_FLAG_PAD_PKCS1	0x00	/* PKCS#1 v1.5 mode */
#define EVP_MD_CTX_FLAG_PAD_X931	0x10	/* X9.31 mode */
#define EVP_MD_CTX_FLAG_PAD_PSS		0x20	/* PSS mode */
#define M_EVP_MD_CTX_FLAG_PSS_SALT(ctx) \
		((ctx->flags>>16) &0xFFFF) /* seed length */
#define EVP_MD_CTX_FLAG_PSS_MDLEN	0xFFFF	/* salt len same as digest */
#define EVP_MD_CTX_FLAG_PSS_MREC	0xFFFE	/* salt max or auto recovered */



    /* Macros to reduce FIPS dependencies: do NOT use in applications */
#define M_EVP_MD_size(e)		((e)->md_size)
#define M_EVP_MD_block_size(e)		((e)->block_size)
#define M_EVP_MD_CTX_set_flags(ctx,flgs) ((ctx)->flags|=(flgs))
#define M_EVP_MD_CTX_clear_flags(ctx,flgs) ((ctx)->flags&=~(flgs))
#define M_EVP_MD_CTX_test_flags(ctx,flgs) ((ctx)->flags&(flgs))
#define M_EVP_MD_type(e)			((e)->type)
#define M_EVP_MD_CTX_type(e)		M_EVP_MD_type(M_EVP_MD_CTX_md(e))
#define M_EVP_MD_CTX_md(e)			((e)->digest)



    const EVP_MD *EVP_md5(void);
    const EVP_MD *EVP_sha1(void);

    void EVP_MD_CTX_init(EVP_MD_CTX *ctx);
    EVP_MD_CTX *EVP_MD_CTX_create(void);
    int	EVP_MD_CTX_cleanup(EVP_MD_CTX *ctx);


    const EVP_CIPHER *EVP_des_cbc(void);
    const EVP_CIPHER *EVP_aes_128_cfb128(void);

    int	EVP_DigestInit(EVP_MD_CTX *ctx, const EVP_MD *type);
    int	EVP_DigestUpdate(EVP_MD_CTX *ctx,const void *d,
                         size_t cnt);
    int	EVP_DigestFinal(EVP_MD_CTX *ctx,unsigned char *md,unsigned int *s);



    typedef struct asn1_type_st {
        int type;
        //union	{
        //	char *ptr;
        //	ASN1_BOOLEAN		boolean;
        //	ASN1_STRING *		asn1_string;
        //	ASN1_OBJECT *		object;
        //	ASN1_INTEGER *		integer;
        //	ASN1_ENUMERATED *	enumerated;
        //	ASN1_BIT_STRING *	bit_string;
        //	ASN1_OCTET_STRING *	octet_string;
        //	ASN1_PRINTABLESTRING *	printablestring;
        //	ASN1_T61STRING *	t61string;
        //	ASN1_IA5STRING *	ia5string;
        //	ASN1_GENERALSTRING *	generalstring;
        //	ASN1_BMPSTRING *	bmpstring;
        //	ASN1_UNIVERSALSTRING *	universalstring;
        //	ASN1_UTCTIME *		utctime;
        //	ASN1_GENERALIZEDTIME *	generalizedtime;
        //	ASN1_VISIBLESTRING *	visiblestring;
        //	ASN1_UTF8STRING *	utf8string;
        //	/* set and sequence are left complete and still
        //	 * contain the set or sequence bytes */
        //	ASN1_STRING *		set;
        //	ASN1_STRING *		sequence;
        //	ASN1_VALUE  *		asn1_value;
        //	} value;
    } ASN1_TYPE;

    struct evp_cipher_ctx_st {
        const EVP_CIPHER *cipher;
        ENGINE *engine;	/* functional reference if 'cipher' is ENGINE-provided */
        int encrypt;		/* encrypt or decrypt */
        int buf_len;		/* number we have left */

        unsigned char  oiv[EVP_MAX_IV_LENGTH];	/* original iv */
        unsigned char  iv[EVP_MAX_IV_LENGTH];	/* working iv */
        unsigned char buf[EVP_MAX_BLOCK_LENGTH];/* saved partial block */
        int num;				/* used by cfb/ofb mode */

        void *app_data;		/* application stuff */
        int key_len;		/* May change for variable length cipher */
        unsigned long flags;	/* Various flags */
        void *cipher_data; /* per EVP data */
        int final_used;
        int block_mask;
        unsigned char final[EVP_MAX_BLOCK_LENGTH];/* possible final block */
    } /* EVP_CIPHER_CTX */;

    struct evp_cipher_st {
        int nid;
        int block_size;
        int key_len;		/* Default value for variable length ciphers */
        int iv_len;
        unsigned long flags;	/* Various flags */
        int (*init)(EVP_CIPHER_CTX *ctx, const unsigned char *key,
                    const unsigned char *iv, int enc);	/* init key */
        int (*do_cipher)(EVP_CIPHER_CTX *ctx, unsigned char *out,
                         const unsigned char *in, unsigned int inl);/* encrypt/decrypt data */
        int (*cleanup)(EVP_CIPHER_CTX *); /* cleanup ctx */
        int ctx_size;		/* how big ctx->cipher_data needs to be */
        int (*set_asn1_parameters)(EVP_CIPHER_CTX *, ASN1_TYPE *); /* Populate a ASN1_TYPE with parameters */
        int (*get_asn1_parameters)(EVP_CIPHER_CTX *, ASN1_TYPE *); /* Get parameters from a ASN1_TYPE */
        int (*ctrl)(EVP_CIPHER_CTX *, int type, int arg, void *ptr); /* Miscellaneous operations */
        void *app_data;		/* Application data */
    } /* EVP_CIPHER */;


    /* Values for cipher flags */

    /* Modes for ciphers */

#define		EVP_CIPH_STREAM_CIPHER		0x0
#define		EVP_CIPH_ECB_MODE		0x1
#define		EVP_CIPH_CBC_MODE		0x2
#define		EVP_CIPH_CFB_MODE		0x3
#define		EVP_CIPH_OFB_MODE		0x4
#define 	EVP_CIPH_MODE			0x7
    /* Set if variable length cipher */
#define 	EVP_CIPH_VARIABLE_LENGTH	0x8
    /* Set if the iv handling should be done by the cipher itself */
#define 	EVP_CIPH_CUSTOM_IV		0x10
    /* Set if the cipher's init() function should be called if key is NULL */
#define 	EVP_CIPH_ALWAYS_CALL_INIT	0x20
    /* Call ctrl() to init cipher parameters */
#define 	EVP_CIPH_CTRL_INIT		0x40
    /* Don't use standard key length function */
#define 	EVP_CIPH_CUSTOM_KEY_LENGTH	0x80
    /* Don't use standard block padding */
#define 	EVP_CIPH_NO_PADDING		0x100
    /* cipher handles random key generation */
#define 	EVP_CIPH_RAND_KEY		0x200
    /* Note if suitable for use in FIPS mode */
#define		EVP_CIPH_FLAG_FIPS		0x400
    /* Allow non FIPS cipher in FIPS mode */
#define		EVP_CIPH_FLAG_NON_FIPS_ALLOW	0x800
    /* Allow use default ASN1 get/set iv */
#define		EVP_CIPH_FLAG_DEFAULT_ASN1	0x1000
    /* Buffer length in bits not bytes: CFB1 mode only */
#define		EVP_CIPH_FLAG_LENGTH_BITS	0x2000

    /* ctrl() values */

#define		EVP_CTRL_INIT			0x0
#define 	EVP_CTRL_SET_KEY_LENGTH		0x1
#define 	EVP_CTRL_GET_RC2_KEY_BITS	0x2
#define 	EVP_CTRL_SET_RC2_KEY_BITS	0x3
#define 	EVP_CTRL_GET_RC5_ROUNDS		0x4
#define 	EVP_CTRL_SET_RC5_ROUNDS		0x5
#define 	EVP_CTRL_RAND_KEY		0x6

    void EVP_CIPHER_CTX_init(EVP_CIPHER_CTX *a);
    int EVP_CIPHER_CTX_cleanup(EVP_CIPHER_CTX *a);

    int EVP_CIPHER_CTX_iv_length(const EVP_CIPHER_CTX *ctx);
    int EVP_CIPHER_CTX_set_key_length(EVP_CIPHER_CTX *x, int keylen);
    int EVP_CIPHER_CTX_set_padding(EVP_CIPHER_CTX *c, int pad);
    int EVP_CIPHER_CTX_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr);
    int EVP_CIPHER_CTX_rand_key(EVP_CIPHER_CTX *ctx, unsigned char *key);

#define EVP_CIPHER_CTX_mode(e)		(EVP_CIPHER_CTX_flags(e) & EVP_CIPH_MODE)

    int	EVP_EncryptInit(EVP_CIPHER_CTX *ctx,const EVP_CIPHER *cipher,
                        const unsigned char *key, const unsigned char *iv);
    int	EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                          int *outl, const unsigned char *in, int inl);
    int	EVP_EncryptFinal(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);


    int	EVP_DecryptInit(EVP_CIPHER_CTX *ctx,const EVP_CIPHER *cipher,
                        const unsigned char *key, const unsigned char *iv);
    int	EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                          int *outl, const unsigned char *in, int inl);
    int	EVP_DecryptFinal(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl);

#ifdef  __cplusplus
}
#endif
#endif
