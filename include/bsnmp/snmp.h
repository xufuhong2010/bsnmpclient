/*
 * Copyright (c) 2001-2003
 *	Fraunhofer Institute for Open Communication Systems (FhG Fokus).
 *	All rights reserved.
 *
 * Author: Harti Brandt <harti@freebsd.org>
 * 
 * Copyright (c) 2010 The FreeBSD Foundation
 * All rights reserved.
 *
 * Portions of this software were developed by Shteryana Sotirova Shopova
 * under sponsorship from the FreeBSD Foundation.
 *
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Begemot: bsnmp/lib/snmp.h,v 1.30 2004/08/06 08:46:54 brandt Exp $
 *
 * Header file for SNMP functions.
 */
#ifndef snmp_h_
#define snmp_h_

#include <sys/types.h>

#define SNMP_COMMUNITY_MAXLEN		128
#define SNMP_MAX_BINDINGS		100
#define	SNMP_CONTEXT_NAME_SIZ		(32 + 1)
#define	SNMP_ENGINE_ID_SIZ		32
#define	SNMP_TIME_WINDOW		150

enum snmp_syntax {
	SNMP_SYNTAX_NULL	= 0,
	SNMP_SYNTAX_INTEGER,		/* == INTEGER32 */
	SNMP_SYNTAX_OCTETSTRING,
	SNMP_SYNTAX_OID,
	SNMP_SYNTAX_IPADDRESS,
	SNMP_SYNTAX_COUNTER,
	SNMP_SYNTAX_GAUGE,		/* == UNSIGNED32 */
	SNMP_SYNTAX_TIMETICKS,

	/* v2 additions */
	SNMP_SYNTAX_COUNTER64,
	SNMP_SYNTAX_NOSUCHOBJECT,	/* exception */
	SNMP_SYNTAX_NOSUCHINSTANCE,	/* exception */
	SNMP_SYNTAX_ENDOFMIBVIEW,	/* exception */
};

typedef union snmp_values {
	int32_t		integer;	/* also integer32 */
	struct {
	u_int		len;
	u_char		*octets;
	}			octetstring;
	asn_oid_t	oid;
	u_char		ipaddress[4];
	uint32_t		uint32;		/* also gauge32, counter32,
						unsigned32, timeticks */
	uint64_t		counter64;
} snmp_values_t;

typedef struct snmp_value {
	asn_oid_t		var;
	enum snmp_syntax	syntax;
	snmp_values_t   v;
} snmp_value_t;

enum snmp_version {
	SNMP_Verr = 0,
	SNMP_V1 = 1,
	SNMP_V2c = 2,
	SNMP_V3,
};

#define	SNMP_MPM_SNMP_V1		0
#define	SNMP_MPM_SNMP_V2c		1
#define	SNMP_MPM_SNMP_V3		3

#define	SNMP_ADM_STR32_SIZ		(32 + 1)
#define	SNMP_AUTH_KEY_SIZ		40
#define	SNMP_PRIV_KEY_SIZ		32
#define	SNMP_USM_AUTH_SIZE		12
#define	SNMP_USM_PRIV_SIZE		8
#define	SNMP_AUTH_HMACMD5_KEY_SIZ	16
#define	SNMP_AUTH_HMACSHA_KEY_SIZ	20
#define	SNMP_PRIV_AES_KEY_SIZ		16
#define	SNMP_PRIV_DES_KEY_SIZ		8


enum snmp_secmodel {
	SNMP_SECMODEL_ANY = 0,
	SNMP_SECMODEL_SNMPv1 = 1,
	SNMP_SECMODEL_SNMPv2c = 2,
	SNMP_SECMODEL_USM = 3,
	SNMP_SECMODEL_UNKNOWN
};

enum snmp_usm_level {
	SNMP_noAuthNoPriv = 1,
	SNMP_authNoPriv = 2,
	SNMP_authPriv = 3
};

enum snmp_authentication {
	SNMP_AUTH_NOAUTH = 0,
	SNMP_AUTH_HMAC_MD5,
	SNMP_AUTH_HMAC_SHA
};

enum snmp_privacy {
	SNMP_PRIV_NOPRIV = 0,
	SNMP_PRIV_DES = 1,
	SNMP_PRIV_AES
};

typedef struct snmp_engine {
	uint8_t			engine_id[SNMP_ENGINE_ID_SIZ];
	uint32_t		engine_len;
	int32_t			engine_boots;
	int32_t			engine_time;
	int32_t			max_msg_size;
} snmp_engine_t;

typedef struct snmp_user {
	enum snmp_authentication	auth_proto;
	enum snmp_privacy		priv_proto;
	uint8_t				auth_key[SNMP_AUTH_KEY_SIZ];
	size_t              auth_len;
	uint8_t				priv_key[SNMP_PRIV_KEY_SIZ];
	size_t              priv_len;
	char				sec_name[SNMP_ADM_STR32_SIZ];
} snmp_user_t;

typedef struct snmp_pdu {
	char        community[SNMP_COMMUNITY_MAXLEN + 1];
	enum snmp_version	version;
	u_int       pdu_type;

	/* SNMPv3 PDU header fields */
	int32_t			identifier;
	uint8_t			flags;
	int32_t			security_model;
	snmp_engine_t	engine;

	/* Associated USM user parameters */
	snmp_user_t	user;
	uint8_t			msg_digest[SNMP_USM_AUTH_SIZE];
	uint8_t			msg_salt[SNMP_USM_PRIV_SIZE];

	/*  View-based Access Model */
	/* XXX: put in separate structure - conflicts with struct snmp_context */
	uint32_t		context_engine_len;
	uint8_t			context_engine[SNMP_ENGINE_ID_SIZ];
	char			context_name[SNMP_CONTEXT_NAME_SIZ];

	/* trap only */
	asn_oid_t		enterprise;
	u_char			agent_addr[4];
	int32_t			generic_trap;
	int32_t			specific_trap;
	uint32_t		time_stamp;

	/* others */
	int32_t			request_id;
	int32_t			error_status;
	int32_t			error_index;

	/* fixes for encoding */
	size_t			outer_len;
	size_t			scoped_len;
	u_char			*outer_ptr;
	u_char			*digest_ptr;
	u_char			*encrypted_ptr;
	u_char			*scoped_ptr;
	u_char			*pdu_ptr;
	u_char			*vars_ptr;

    
	u_int			nbindings;
	snmp_value_t	bindings[SNMP_MAX_BINDINGS];
} snmp_pdu_t;
#define snmp_v1_pdu snmp_pdu

#define SNMP_PDU_GET		0
#define SNMP_PDU_GETNEXT	1
#define SNMP_PDU_RESPONSE	2
#define SNMP_PDU_SET		3
#define SNMP_PDU_TRAP		4	/* v1 */
#define SNMP_PDU_GETBULK	5	/* v2 */
#define SNMP_PDU_INFORM		6	/* v2 */
#define SNMP_PDU_TRAP2		7	/* v2 */
#define SNMP_PDU_REPORT		8	/* v2 */

#define SNMP_ERR_NOERROR	0
#define SNMP_ERR_TOOBIG		1
#define SNMP_ERR_NOSUCHNAME	2	/* v1 */
#define SNMP_ERR_BADVALUE	3	/* v1 */
#define SNMP_ERR_READONLY	4	/* v1 */
#define SNMP_ERR_GENERR		5
#define SNMP_ERR_NO_ACCESS	6	/* v2 */
#define SNMP_ERR_WRONG_TYPE	7	/* v2 */
#define SNMP_ERR_WRONG_LENGTH	8	/* v2 */
#define SNMP_ERR_WRONG_ENCODING	9	/* v2 */
#define SNMP_ERR_WRONG_VALUE	10	/* v2 */
#define SNMP_ERR_NO_CREATION	11	/* v2 */
#define SNMP_ERR_INCONS_VALUE	12	/* v2 */
#define SNMP_ERR_RES_UNAVAIL	13	/* v2 */
#define SNMP_ERR_COMMIT_FAILED	14	/* v2 */
#define SNMP_ERR_UNDO_FAILED	15	/* v2 */
#define SNMP_ERR_AUTH_ERR	16	/* v2 */
#define SNMP_ERR_NOT_WRITEABLE	17	/* v2 */
#define SNMP_ERR_INCONS_NAME	18	/* v2 */

#define SNMP_TRAP_COLDSTART	0
#define SNMP_TRAP_WARMSTART	1
#define SNMP_TRAP_LINKDOWN	2
#define SNMP_TRAP_LINKUP	3
#define SNMP_TRAP_AUTHENTICATION_FAILURE	4
#define SNMP_TRAP_EGP_NEIGHBOR_LOSS	5
#define SNMP_TRAP_ENTERPRISE	6

enum snmp_code {
	SNMP_CODE_OK = 0,
	SNMP_CODE_FAILED,
	SNMP_CODE_BADVERS,
	SNMP_CODE_BADLEN,
	SNMP_CODE_BADENC,
	SNMP_CODE_OORANGE,
	SNMP_CODE_BADSECLEVEL,
	SNMP_CODE_NOTINTIME,
	SNMP_CODE_BADUSER,
	SNMP_CODE_BADENGINE,
	SNMP_CODE_BADDIGEST,
	SNMP_CODE_EDECRYPT,
	SNMP_CODE_BADBINDINGNUMBER,
	SNMP_CODE_BADRESULT,
	SNMP_CODE_BADOID,

	SNMP_CODE_SYNTAX_MISMATCH,
	SNMP_CODE_SYNTAX_NOSUCHOBJECT,	/* exception */
	SNMP_CODE_SYNTAX_NOSUCHINSTANCE,	/* exception */
	SNMP_CODE_SYNTAX_ENDOFMIBVIEW,	/* exception */

	SNMP_CODE_ERR_TOOBIG,
	SNMP_CODE_ERR_NOSUCHNAME,
	SNMP_CODE_ERR_BADVALUE,
	SNMP_CODE_ERR_READONLY,
	SNMP_CODE_ERR_GENERR,
	SNMP_CODE_ERR_NO_ACCESS,
	SNMP_CODE_ERR_WRONG_TYPE,
	SNMP_CODE_ERR_WRONG_LENGTH,
	SNMP_CODE_ERR_WRONG_ENCODING,
	SNMP_CODE_ERR_WRONG_VALUE,
	SNMP_CODE_ERR_NO_CREATION,
	SNMP_CODE_ERR_INCONS_VALUE,
	SNMP_CODE_ERR_RES_UNAVAIL,
	SNMP_CODE_ERR_COMMIT_FAILED,
	SNMP_CODE_ERR_UNDO_FAILED,
	SNMP_CODE_ERR_AUTH_ERR,
	SNMP_CODE_ERR_NOT_WRITEABLE,
	SNMP_CODE_ERR_INCONS_NAME
};


#define SNMP_CODE_ERR_NOERROR	(SNMP_CODE_ERR_TOOBIG-1)

#define	SNMP_MSG_AUTH_FLAG   		0x1
#define	SNMP_MSG_PRIV_FLAG   		0x2
#define	SNMP_MSG_REPORT_FLAG 		0x4
#define	SNMP_MSG_AUTODISCOVER		0x80

void snmp_pdu_init(snmp_pdu_t *pdu);
void snmp_value_free(snmp_value_t *);
int snmp_value_parse(const char *, enum snmp_syntax, snmp_values_t *);
int snmp_value_copy(snmp_value_t *, const snmp_value_t *);

void snmp_pdu_free(snmp_pdu_t *);
void snmp_pdu_init_secparams(snmp_pdu_t *);
enum snmp_code snmp_pdu_decode(asn_buf_t *b, snmp_pdu_t *pdu, int32_t *);
enum snmp_code snmp_pdu_decode_header(asn_buf_t *, snmp_pdu_t *);
enum snmp_code snmp_pdu_decode_scoped(asn_buf_t *, snmp_pdu_t *, int32_t *);
enum snmp_code snmp_pdu_encode(snmp_pdu_t *, asn_buf_t *);
enum snmp_code snmp_pdu_decode_secmode(asn_buf_t *, snmp_pdu_t *);

int snmp_pdu_snoop(const asn_buf_t *);

void snmp_pdu_dump(const snmp_pdu_t *pdu);

enum snmp_code snmp_set_auth_passphrase(snmp_user_t *user
	                            , const char *passphrase, size_t passphrase_len);
enum snmp_code snmp_set_priv_passphrase(snmp_user_t *user
	                            , const char *passphrase, size_t passphrase_len);
enum snmp_code snmp_auth_to_localization_keys(snmp_user_t *user
	                            , const uint8_t *eid, uint32_t elen);
enum snmp_code snmp_priv_to_localization_keys(snmp_user_t *user
	                            , const uint8_t *eid, uint32_t elen);

//enum snmp_code snmp_passwd_to_keys(snmp_user_t *, char *);
//enum snmp_code snmp_get_local_keys(snmp_user_t *, uint8_t *, uint32_t);
enum snmp_code snmp_calc_keychange(snmp_user_t *, uint8_t *);

extern void (*snmp_error)(const char *, ...);
extern void (*snmp_printf)(const char *, ...);


/* check wheater the answer is valid or not */
enum snmp_code  snmp_pdu_check(const snmp_pdu_t *_req, const snmp_pdu_t *_resp);

#define TRUTH_MK(F) ((F) ? 1 : 2)
#define TRUTH_GET(T) (((T) == 1) ? 1 : 0)
#define TRUTH_OK(T)  ((T) == 1 || (T) == 2)

#endif
