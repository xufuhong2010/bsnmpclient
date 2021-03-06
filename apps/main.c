/*-
 * Copyright (c) 2005-2006 The FreeBSD Project
 * All rights reserved.
 *
 * Author: Shteryana Shopova <syrinx@FreeBSD.org>
 *
 * Redistribution of this software and documentation and use in source and
 * binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *
 * 1. Redistributions of source code or documentation must retain the above
 *    copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
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
 * Bsnmpget and bsnmpwalk are simple tools for querying SNMP agents,
 * bsnmpset can be used to set MIB objects in an agent.
 */

#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "getopt.h"
#include <compat/sys/queue.h>
#else
#include <syslog.h>
#include <unistd.h>
#include <sys/queue.h>
#endif

#include <bsnmp/asn1.h>
#include <bsnmp/snmp.h>
#include <bsnmp/client.h>
#include "bsnmptc.h"
#include "bsnmptools.h"
#ifdef _WIN32

#define socket_t    SOCKET
#define ssize_t     int


#define vsnprintf   _vsnprintf
#define strtoll     _strtoi64
#define strtoull    _strtoui64
#define random      rand
#define strncasecmp _strnicmp
#define strcasecmp  _stricmp

#define MAXPATHLEN  MAX_PATH

#else

#define socket_t    int
#define closesocket close

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#endif

#ifndef HAVE_ERR_H

void warnx(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "warning: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void warn(const char *fmt, ...) {
    va_list ap;
    int e = errno;

    va_start(ap, fmt);
    fprintf(stderr, "warning: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, ": %s\n", strerror(e));
    va_end(ap);
}

void errx(int code, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(code);
}

void err(int code, const char *fmt, ...) {
    va_list ap;
    int e = errno;

    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, ": %s\n", strerror(e));
    va_end(ap);
    exit(code);
}

#endif

#ifndef HAVE_STRLCPY

size_t strlcpy(char *dst, const char *src, size_t len);

#endif

static char *program_name = NULL;
static enum program_e {
    BSNMPGET,
    BSNMPWALK,
    BSNMPSET
} program;

/* *****************************************************************************
 * Common bsnmptools functions.
 */

static void usage_help() {
    fprintf(stderr,"A snmp tools\n"
            "Usage:\n"
            "  %s -h/--help\n"
            "  %s -v/--version\n"
            "  %s command [options...]\n"
            "Examples:\n"
            "  %s get -v 2 -s udp::public@127.0.0.1:161 1.3.6.1.2.1.1.2.0\n"
            "  %s get -v 3 -s udp::mfk@noauth#127.0.0.1:161 1.3.6.1.2.1.1.2.0\n"
            "  %s get -v 3 -s udp::mfk1@md5%%mfk123456#127.0.0.1:161 1.3.6.1.2.1.1.2.0\n"
            "  %s get -v 3 -s udp::mfk2@md5%%mfk123456#des%%mfk123456#127.0.0.1:161 1.3.6.1.2.1.1.2.0\n"
            , program_name, program_name, program_name, program_name, program_name, program_name, program_name);
}

static void usage(int program) {
    fprintf(stderr,
            "Usage:\n"
            "%s %s [-b buffersize] [-I options] [-i filelist]\n"
            "\t[-l filename]%s [-o output]\n"
            "\t%s[-r retries] [-s [trans::][community@][server][:port]]\n"
            "\t[-t timeout] [-v version]%s\n"
            " options:\n"
            "%s"
            " -d  Increase debugging level\n"
            "%s"
            " -h  Print this help\n"
            " -n  Only use numerical representations for input and output OIDs\n"
            " -b buffersize	 Change size of receive/transmit buffer, default 10000\n"
            " -I options  Load each MIB description file from the given list with\n"
            "             possible non-default options:\n"
            "    cut=OID        An initial OID that was cut from the file to be appended\n"
            "    path=pathname  Path where to read the files from\n"
            "    file=filelist  Comma separated list of files to which the two options\n"
            "                   above will apply\n"
            " -i filelist  Comma separated list of file to read symbolic object names "
            "from\n"
            " -l filename  Path of the posix local for local transport\n"
            "%s"
            " -o output  Output format: [quiet|short|verbose], default: short\n"
            "%s"
            " -r retries  Number of retries resending a request, default: 3\n"
            " -s [trans::][community@][server][:port] \n"
            "    [trans::][name@[noauth#]][server][:port]\n"
            "    [trans::][name@[auth_proto%%auth_pass#]][server][:port]\n"
            "    [trans::][name@[auth_proto%%auth_pass#[priv_proto%%priv_pass#]]][server][:port]\n"
            "        Server specification:\n"
            "            trans      Transport type: [udp|stream|dgram], default: udp\n"
            "            community  Community name, default: public\n"
            "            noauth     set auth protocel = noauth, set priv protocel = nopriv\n"
            "            auth_proto auth protocel: [md5|sha]\n"
            "            auth_pass  auth passphrase\n"
            "            priv_proto priv protocel: [des|aes]\n"
            "            priv_pass  priv passphrase\n"
            "            server     SNMP agent name or IP address, default: localhost\n"
            "            port       Agent port, default: snmp=161\n"
            " -t timeout  Number of seconds before resending a request packet, default: 3\n"
            " -v version  SNMP version to use: [1|2|3], default: 2\n"
            "%s",
            program_name,
            (program == BSNMPGET) ? "get [-adehn]" :
            (program == BSNMPWALK) ? "walk [-dhn]" :
            (program == BSNMPSET) ? "set [-adehn]" :
            "",
            (program == BSNMPGET) ? " [-M max-repetitions] [-N non-repeaters]" : "",
            (program == BSNMPGET) ? "[-p pdu] " : "",
            (program == BSNMPGET) ? " OID [OID ...]" :
            (program == BSNMPWALK || program == BSNMPSET) ? " [OID ...]" :
            "",
            (program == BSNMPGET || program == BSNMPSET) ?
            " -a  Skip any sanity checks when adding OIDs to a PDU\n" : "",
            (program == BSNMPGET || program == BSNMPSET) ?
            " -e  On error resend request without the variable which caused the error\n" :
            "",
            (program == BSNMPGET) ?
            " -M max-repetitions  Value for max-repetitions (for GetBulk only), "
            "default: 1\n"
            " -N non-repeaters  Value for non-repeaters (for GetBulk only), default: 0\n" :
            "",
            (program == BSNMPGET) ?
            " -p pdu  PDU type to send: [get|getbulk|getnext], default: get\n" : "",
            (program == BSNMPGET) ?  " OID [OID ...]  Object identifier(s)\n" :
            (program == BSNMPWALK || program == BSNMPSET) ?
            " [OID ...]  Object identifier(s), default: mib2\n" :
            ""
           );
}


static int32_t
parse_max_repetitions(struct snmp_toolinfo *snmptoolctx, char *opt_arg) {
    uint32_t v;

    assert(opt_arg != NULL);

    v = strtoul(opt_arg, (void *) NULL, 10);

    if (v > SNMP_MAX_BINDINGS) {
        warnx("Max repetitions value greater than %d maximum allowed.",
              SNMP_MAX_BINDINGS);
        return (-1);
    }

    SET_MAXREP(snmptoolctx, v);
    return (2);
}

static int32_t
parse_non_repeaters(struct snmp_toolinfo *snmptoolctx, char *opt_arg) {
    uint32_t v;

    assert(opt_arg != NULL);

    v = strtoul(opt_arg, (void *) NULL, 10);

    if (v > SNMP_MAX_BINDINGS) {
        warnx("Non repeaters value greater than %d maximum allowed.",
              SNMP_MAX_BINDINGS);
        return (-1);
    }

    SET_NONREP(snmptoolctx, v);
    return (2);
}

static int32_t
parse_pdu_type(struct snmp_toolinfo *snmptoolctx, char *opt_arg) {
    assert(opt_arg != NULL);

    if (strcasecmp(opt_arg, "getbulk") == 0)
        SET_PDUTYPE(snmptoolctx, SNMP_PDU_GETBULK);
    else if (strcasecmp(opt_arg, "getnext") == 0)
        SET_PDUTYPE(snmptoolctx, SNMP_PDU_GETNEXT);
    else if (strcasecmp(opt_arg, "get") == 0)
        SET_PDUTYPE(snmptoolctx, SNMP_PDU_GET);
    else {
        warnx("PDU type '%s' not supported.", opt_arg);
        return (-1);
    }

    return (2);
}

static int32_t
snmptool_parse_options(struct snmp_toolinfo *snmptoolctx, int argc, char **argv) {
    int32_t count, optnum = 0;
    int ch;
    char *opts;

    switch (program) {
    case BSNMPWALK:
        opts = "dhnb:I:i:l:o:r:s:t:v:";
        break;
    case BSNMPGET:
        opts = "adehnb:I:i:l:M:N:o:p:r:s:t:v:";
        break;
    case BSNMPSET:
        opts = "adehnb:I:i:l:o:r:s:t:v:";
        break;
    default:
        return (-1);
    }

    while ((ch = getopt(argc, argv, opts)) != EOF) {
        switch (ch) {
        case 'a':
            if ((count = parse_skip_access(snmptoolctx)) < 0)
                return (-1);
            break;
        case 'b':
            if ((count = parse_buflen(&snmptoolctx->client, optarg)) < 0)
                return (-1);
            break;
        case 'd':
            if ((count = parse_debug(&snmptoolctx->client)) < 0)
                return (-1);
            break;
        case 'e':
            if ((count = parse_errors(snmptoolctx)) < 0)
                return (-1);
            break;
        case 'h':
            usage(program);
            return (-2);
        case 'I':
            if ((count = parse_include(snmptoolctx, optarg)) < 0)
                return (-1);
            break;
        case 'i':
            if ((count = parse_file(snmptoolctx, optarg)) < 0)
                return (-1);
            break;
        case 'l':
            if ((count = parse_local_path(&snmptoolctx->client, optarg)) < 0)
                return (-1);
            break;
        case 'M':
            if ((count = parse_max_repetitions(snmptoolctx, optarg)) < 0)
                return (-1);
            break;
        case 'N':
            if ((count = parse_non_repeaters(snmptoolctx, optarg)) < 0)
                return (-1);
            break;
        case 'n':
            if ((count = parse_num_oids(snmptoolctx)) < 0)
                return (-1);
            break;
        case 'o':
            if ((count = parse_output(snmptoolctx, optarg)) < 0)
                return (-1);
            break;
        case 'p':
            if ((count = parse_pdu_type(snmptoolctx, optarg)) < 0)
                return (-1);
            break;
        case 'r':
            if ((count = parse_retry(&snmptoolctx->client, optarg)) < 0)
                return (-1);
            break;
        case 's':
            if ((count = parse_server(&snmptoolctx->client, optarg)) < 0)
                return (-1);
            break;
        case 't':
            if ((count = parse_timeout(&snmptoolctx->client, optarg)) < 0)
                return (-1);
            break;
        case 'v':
            if ((count = parse_version(&snmptoolctx->client, optarg)) < 0)
                return (-1);
            break;
        case '?':
        default:
            usage(program);
            return (-1);
        }
        optnum += count;
    }

    return (optnum);
}

/*
 * Read user input OID - one of following formats:
 * 1) 1.2.1.1.2.1.0 - that is if option numeric was giveni;
 * 2) string - in such case append .0 to the asn_oid subs;
 * 3) string.1 - no additional proccessing required in such case.
 */
static char *
snmptools_parse_stroid(struct snmp_toolinfo *snmptoolctx,
                       struct snmp_object *obj, char *argv) {
    char string[MAXSTR], *str;
    int32_t i = 0;
    asn_oid_t in_oid;

    str = argv;

    if (*str == '.')
        str++;

    while (isalpha(*str) || *str == '_' || (i != 0 && isdigit(*str))) {
        str++;
        i++;
    }

    if (i <= 0 || i >= MAXSTR)
        return (NULL);

    memset(&in_oid, 0, sizeof(asn_oid_t));
    if ((str = snmp_parse_suboid((argv + i), &in_oid)) == NULL) {
        warnx("Invalid OID - %s", argv);
        return (NULL);
    }

    strlcpy(string, argv, i + 1);
    if (snmp_lookup_oidall(snmptoolctx, obj, string) < 0) {
        warnx("No entry for %s in mapping lists", string);
        return (NULL);
    }

    /* If OID given on command line append it. */
    if (in_oid.len > 0)
        asn_append_oid(&(obj->val.oid), &in_oid);
    else if (*str == '[') {
        if ((str = snmp_parse_index(snmptoolctx, str + 1, obj)) == NULL)
            return (NULL);
    } else if (obj->val.syntax > 0 && GET_PDUTYPE(snmptoolctx) ==
               SNMP_PDU_GET) {
        if (snmp_suboid_append(&(obj->val.oid), (asn_subid_t) 0) < 0)
            return (NULL);
    }

    return (str);
}

static int32_t
snmptools_parse_oid(struct snmp_toolinfo *snmptoolctx,
                    struct snmp_object *obj, char *argv) {
    if (argv == NULL)
        return (-1);

    if (ISSET_NUMERIC(snmptoolctx)) {
        if (snmp_parse_numoid(argv, &(obj->val.oid)) < 0)
            return (-1);
    } else {
        if (snmptools_parse_stroid(snmptoolctx, obj, argv) == NULL &&
                snmp_parse_numoid(argv, &(obj->val.oid)) < 0)
            return (-1);
    }

    return (1);
}

static int32_t
snmptool_add_vbind(snmp_pdu_t *pdu, struct snmp_object *obj) {
    if (obj->error > 0)
        return (0);

    asn_append_oid(&(pdu->bindings[pdu->nbindings].oid), &(obj->val.oid));
    pdu->nbindings++;

    return (pdu->nbindings);
}

/* *****************************************************************************
 * bsnmpget private functions.
 */
static int32_t
snmpget_verify_vbind(struct snmp_toolinfo *snmptoolctx, snmp_pdu_t *pdu,
                     struct snmp_object *obj) {
    if (pdu->version == SNMP_V1 && obj->val.syntax ==
            SNMP_SYNTAX_COUNTER64) {
        warnx("64-bit counters are not supported in SNMPv1 PDU");
        return (-1);
    }


    return (1);
}

/*
 * In case of a getbulk PDU, the error_status and error_index fields are used by
 * libbsnmp to hold the values of the non-repeaters and max-repetitions fields
 * that are present only in the getbulk - so before sending the PDU make sure
 * these have correct values as well.
 */
static void
snmpget_fix_getbulk(snmp_pdu_t *pdu, uint32_t max_rep, uint32_t non_rep) {
    assert(pdu != NULL);

    if (pdu->nbindings < non_rep)
        pdu->error_status = pdu->nbindings;
    else
        pdu->error_status = non_rep;

    if (max_rep > 0)
        pdu->error_index = max_rep;
    else
        pdu->error_index = 1;
}

static int
snmptool_get(struct snmp_toolinfo *snmptoolctx) {
    snmp_pdu_t req, resp;

    snmp_pdu_create(&snmptoolctx->client, &req, SNMP_PDU_GET);
    snmp_pdu_create(&snmptoolctx->client, &resp, SNMP_PDU_GET);

    while ((snmp_pdu_add_bindings(snmptoolctx, snmpget_verify_vbind,
                                  snmptool_add_vbind, &req, SNMP_MAX_BINDINGS)) > 0) {

        if (GET_PDUTYPE(snmptoolctx) == SNMP_PDU_GETBULK)
            snmpget_fix_getbulk(&req, GET_MAXREP(snmptoolctx),
                                GET_NONREP(snmptoolctx));

        if (snmp_dialog(&snmptoolctx->client, &req, &resp) == -1) {
            warnx("Snmp dialog - %s", strerror(errno));
            break;
        }

        if (snmp_pdu_check(&resp, &req) == SNMP_CODE_OK) {
            snmp_output_resp(snmptoolctx, &resp);
            break;
        }

        snmp_output_err_resp(snmptoolctx, &resp);
        if (GET_PDUTYPE(snmptoolctx) == SNMP_PDU_GETBULK ||
                !ISSET_RETRY(snmptoolctx))
            break;

        /*
         * Loop through the object list and set object->error to the
         * varbinding that caused the error.
         */
        if (snmp_object_seterror(snmptoolctx,
                                 &(resp.bindings[resp.error_index - 1]),
                                 resp.error_status) <= 0)
            break;

        fprintf(stderr, "Retrying...\n");
        snmp_pdu_free(&resp);
        snmp_pdu_create(&snmptoolctx->client, &req, GET_PDUTYPE(snmptoolctx));
    }

    snmp_pdu_free(&resp);

    return (0);
}


/* *****************************************************************************
 * bsnmpwalk private functions.
 */
/* The default tree to walk. */
static const asn_oid_t snmp_mibII_OID = {
    6 , { 1, 3, 6, 1, 2, 1 }
};

static int32_t
snmpwalk_add_default(struct snmp_toolinfo *snmptoolctx,
                     struct snmp_object *obj, char *string) {
    asn_append_oid(&(obj->val.oid), &snmp_mibII_OID);
    return (1);
}

/*
 * Prepare the next GetNext/Get PDU to send.
 */
static void
snmpwalk_nextpdu_create(struct snmp_client* client, uint32_t op, asn_oid_t *var, snmp_pdu_t *pdu) {
    snmp_pdu_create(client, pdu, op);
    asn_append_oid(&(pdu->bindings[0].oid), var);
    pdu->nbindings = 1;
}

static int
snmptool_walk(struct snmp_toolinfo *snmptoolctx) {
    snmp_pdu_t req, resp;
    asn_oid_t root;	/* Keep the inital oid. */
    int32_t outputs, rc;

    snmp_pdu_create(&snmptoolctx->client, &req, SNMP_PDU_GETNEXT);

    while ((rc = snmp_pdu_add_bindings(snmptoolctx, NULL,
                                       snmptool_add_vbind, &req, 1)) > 0) {

        /* Remember the root where the walk started from. */
        memset(&root, 0, sizeof(asn_oid_t));
        asn_append_oid(&root, &(req.bindings[0].oid));

        outputs = 0;
        while (snmp_dialog(&snmptoolctx->client, &req, &resp) >= 0) {
            int continue_ok = 1;
            switch(snmp_pdu_check(&resp, &req)) {
            case SNMP_CODE_OK:
                continue_ok = 1;
                break;
            default:
                continue_ok = 0;
                break;
            }

            if(continue_ok == 0) {
                snmp_output_err_resp(snmptoolctx, &resp);
                snmp_pdu_free(&resp);
                outputs = -1;
                break;
            }

            if (!(asn_is_suboid(&root, &(resp.bindings[0].oid)))) {
                snmp_pdu_free(&resp);
                break;
            }

            if (snmp_output_resp(snmptoolctx, &resp)!= 0) {
                snmp_pdu_free(&resp);
                outputs = -1;
                break;
            }
            outputs++;
            snmp_pdu_free(&resp);

            snmpwalk_nextpdu_create(&snmptoolctx->client, SNMP_PDU_GETNEXT,
                                    &(resp.bindings[0].oid), &req);
        }

        /* Just in case our root was a leaf. */
        if (outputs == 0) {
            snmpwalk_nextpdu_create(&snmptoolctx->client, SNMP_PDU_GET, &root, &req);
            if (snmp_dialog(&snmptoolctx->client, &req, &resp) == SNMP_CODE_OK) {
                if (snmp_pdu_check(&resp, &req) == SNMP_CODE_OK)
                    snmp_output_err_resp(snmptoolctx, &resp);
                else
                    snmp_output_resp(snmptoolctx, &(resp));

                snmp_pdu_free(&resp);
            } else
                warnx("Snmp dialog - %s", strerror(errno));
        }

        if (snmp_object_remove(snmptoolctx, &root) < 0) {
            warnx("snmp_object_remove");
            break;
        }

        snmp_pdu_create(&snmptoolctx->client, &req, SNMP_PDU_GETNEXT);
    }

    if (rc == 0)
        return (0);
    else
        return (1);
}

/* *****************************************************************************
 * bsnmpset private functions.
 */

static int32_t
parse_oid_numeric(snmp_value_t *value, char *val) {
    char *endptr;
    int32_t saved_errno;
    asn_subid_t suboid;

    do {
        saved_errno = errno;
        errno = 0;
        suboid = strtoul(val, &endptr, 10);
        if (errno != 0) {
            warnx("Value %s not supported - %s", val,
                  strerror(errno));
            errno = saved_errno;
            return (-1);
        }
        errno = saved_errno;
        if ((asn_subid_t) suboid > ASN_MAXID) {
            warnx("Suboid %u > ASN_MAXID", suboid);
            return (-1);
        }
        if (snmp_suboid_append(&(value->v.oid), suboid) < 0)
            return (-1);
        val = endptr + 1;
    } while (*endptr == '.');

    if (*endptr != '\0')
        warnx("OID value %s not supported", val);

    value->syntax = SNMP_SYNTAX_OID;
    return (0);
}

/*
 * Allow OID leaf in both forms:
 * 1) 1.3.6.1.2... ->  in such case call directly the function reading raw OIDs;
 * 2) begemotSnmpdAgentFreeBSD -> lookup the ASN OID corresponding to that.
 */
static int32_t
parse_oid_string(struct snmp_toolinfo *snmptoolctx,
                 snmp_value_t *value, char *string) {
    struct snmp_object obj;

    if (isdigit(string[0]))
        return (parse_oid_numeric(value, string));

    memset(&obj, 0, sizeof(struct snmp_object));
    if (snmp_lookup_enumoid(snmptoolctx, &obj, string) < 0) {
        warnx("Unknown OID enum string - %s", string);
        return (-1);
    }

    asn_append_oid(&(value->v.oid), &(obj.val.oid));
    return (1);
}

static int32_t
parse_ip(snmp_value_t * value, char * val) {
    uint32_t v;
    int32_t i;
    char *endptr, *str;

    str = val;
    for (i = 0; i < 4; i++) {
        v = strtoul(str, &endptr, 10);
        if (v > 0xff)
            return (-1);
        if (*endptr != '.' && *endptr != '\0' && i != 3)
            break;
        str = endptr + 1;
        value->v.ipaddress[i] = (uint8_t) v;
    }

    value->syntax = SNMP_SYNTAX_IPADDRESS;
    return (0);
}

static int32_t
parse_int(snmp_value_t *value, char *val) {
    char *endptr;
    int32_t v, saved_errno;

    saved_errno = errno;
    errno = 0;

    v = strtol(val, &endptr, 10);

    if (errno != 0) {
        warnx("Value %s not supported - %s", val, strerror(errno));
        errno = saved_errno;
        return (-1);
    }

    value->syntax = SNMP_SYNTAX_INTEGER;
    value->v.integer = v;
    errno = saved_errno;

    return (0);
}

static int32_t
parse_int_string(struct snmp_object *object, char *val) {
    int32_t	v;

    if (isdigit(val[0]))
        return ((parse_int(&(object->val), val)));

    if (object->info == NULL) {
        warnx("Unknown enumerated integer type - %s", val);
        return (-1);
    }
    if ((v = enum_number_lookup(object->info->snmp_enum, val)) < 0)
        warnx("Unknown enumerated integer type - %s", val);

    object->val.v.integer = v;
    return (1);
}

/*
 * Here syntax may be one of SNMP_SYNTAX_COUNTER, SNMP_SYNTAX_GAUGE,
 * SNMP_SYNTAX_TIMETICKS.
 */
static int32_t
parse_uint(snmp_value_t *value, char *val) {
    char *endptr;
    uint32_t v = 0;
    int32_t saved_errno;

    saved_errno = errno;
    errno = 0;

    v = strtoul(val, &endptr, 10);

    if (errno != 0) {
        warnx("Value %s not supported - %s", val, strerror(errno));
        errno = saved_errno;
        return (-1);
    }

    value->v.uint32 = v;
    errno = saved_errno;

    return (0);
}

static int32_t
parse_ticks(snmp_value_t *value, char *val) {
    if (parse_uint(value, val) < 0)
        return (-1);

    value->syntax = SNMP_SYNTAX_TIMETICKS;
    return (0);
}

static int32_t
parse_gauge(snmp_value_t *value, char *val) {
    if (parse_uint(value, val) < 0)
        return (-1);

    value->syntax = SNMP_SYNTAX_GAUGE;
    return (0);
}

static int32_t
parse_counter(snmp_value_t *value, char *val) {
    if (parse_uint(value, val) < 0)
        return (-1);

    value->syntax = SNMP_SYNTAX_COUNTER;
    return (0);
}

static int32_t
parse_uint64(snmp_value_t *value, char *val) {
    char *endptr;
    int32_t saved_errno;
    uint64_t v;

    saved_errno = errno;
    errno = 0;

    v = strtoull(val, &endptr, 10);

    if (errno != 0) {
        warnx("Value %s not supported - %s", val, strerror(errno));
        errno = saved_errno;
        return (-1);
    }

    value->syntax = SNMP_SYNTAX_COUNTER64;
    value->v.counter64 = v;
    errno = saved_errno;

    return (0);
}

static int32_t
parse_syntax_val(snmp_value_t *value, enum snmp_syntax syntax, char *val) {
    switch (syntax) {
    case SNMP_SYNTAX_INTEGER:
        return (parse_int(value, val));
    case SNMP_SYNTAX_IPADDRESS:
        return (parse_ip(value, val));
    case SNMP_SYNTAX_COUNTER:
        return (parse_counter(value, val));
    case SNMP_SYNTAX_GAUGE:
        return (parse_gauge(value, val));
    case SNMP_SYNTAX_TIMETICKS:
        return (parse_ticks(value, val));
    case SNMP_SYNTAX_COUNTER64:
        return (parse_uint64(value, val));
    case SNMP_SYNTAX_OCTETSTRING:
        return (snmp_tc2oct(SNMP_STRING, value, val));
    case SNMP_SYNTAX_OID:
        return (parse_oid_numeric(value, val));
    default:
        /* NOTREACHED */
        break;
    }

    return (-1);
}

/*
 * Parse a command line argument of type OID=syntax:value and fill in whatever
 * fields can be derived from the input into snmp_value structure. Reads numeric
 * OIDs.
 */
static int32_t
parse_pair_numoid_val(char *str, snmp_value_t *snmp_val) {
    int32_t cnt;
    char *ptr;
    enum snmp_syntax syntax;
    char oid_str[ASN_OIDSTRLEN];

    ptr = str;
    for (cnt = 0; cnt < ASN_OIDSTRLEN; cnt++)
        if (ptr[cnt] == '=')
            break;

    if (cnt >= ASN_OIDSTRLEN) {
        warnx("OID too long - %s", str);
        return (-1);
    }
    strlcpy(oid_str, ptr, (size_t) (cnt + 1));

    ptr = str + cnt + 1;
    for (cnt = 0; cnt < MAX_CMD_SYNTAX_LEN; cnt++)
        if(ptr[cnt] == ':')
            break;

    if (cnt >= MAX_CMD_SYNTAX_LEN) {
        warnx("Unknown syntax in OID - %s", str);
        return (-1);
    }

    if ((syntax = parse_syntax(ptr)) <= SNMP_SYNTAX_NULL) {
        warnx("Unknown syntax in OID - %s", ptr);
        return (-1);
    }

    ptr = ptr + cnt + 1;
    for (cnt = 0; cnt < MAX_OCTSTRING_LEN; cnt++)
        if (ptr[cnt] == '\0')
            break;

    if (ptr[cnt] != '\0') {
        warnx("Value string too long - %s",ptr);
        return (-1);
    }

    /*
     * Here try parsing the OIDs and syntaxes and then check values - have
     * to know syntax to check value boundaries.
     */
    if (snmp_parse_numoid(oid_str, &(snmp_val->oid)) < 0) {
        warnx("Error parsing OID %s",oid_str);
        return (-1);
    }

    if (parse_syntax_val(snmp_val, syntax, ptr) < 0)
        return (-1);

    return (1);
}

/* XXX-BZ aruments should be swapped. */
static int32_t
parse_syntax_strval(struct snmp_toolinfo *snmptoolctx, char *str,
                    struct snmp_object *object) {
    uint32_t len;
    enum snmp_syntax syn;

    /*
     * Syntax string here not required  - still may be present.
     */

    if (GET_OUTPUT(snmptoolctx) == OUTPUT_VERBOSE) {
        for (len = 0 ; *(str + len) != ':'; len++) {
            if (*(str + len) == '\0') {
                warnx("Syntax missing in value - %s", str);
                return (-1);
            }
        }
        if ((syn = parse_syntax(str)) <= SNMP_SYNTAX_NULL) {
            warnx("Unknown syntax in - %s", str);
            return (-1);
        }
        if (syn != object->val.syntax) {
            if (!ISSET_ERRIGNORE(snmptoolctx)) {
                warnx("Bad syntax in - %s", str);
                return (-1);
            } else
                object->val.syntax = syn;
        }
        len++;
    } else
        len = 0;

    switch (object->val.syntax) {
    case SNMP_SYNTAX_INTEGER:
        return (parse_int_string(object, str + len));
    case SNMP_SYNTAX_IPADDRESS:
        return (parse_ip(&(object->val), str + len));
    case SNMP_SYNTAX_COUNTER:
        return (parse_counter(&(object->val), str + len));
    case SNMP_SYNTAX_GAUGE:
        return (parse_gauge(&(object->val), str + len));
    case SNMP_SYNTAX_TIMETICKS:
        return (parse_ticks(&(object->val), str + len));
    case SNMP_SYNTAX_COUNTER64:
        return (parse_uint64(&(object->val), str + len));
    case SNMP_SYNTAX_OCTETSTRING:
        return (snmp_tc2oct(object->info->tc, &(object->val),
                            str + len));
    case SNMP_SYNTAX_OID:
        return (parse_oid_string(snmptoolctx, &(object->val),
                                 str + len));
    default:
        /* NOTREACHED */
        break;
    }

    return (-1);
}

static int32_t
parse_pair_stroid_val(struct snmp_toolinfo *snmptoolctx,
                      struct snmp_object *obj, char *argv) {
    char *ptr;

    if ((ptr = snmptools_parse_stroid(snmptoolctx, obj, argv)) == NULL)
        return (-1);

    if (*ptr != '=') {
        warnx("Value to set expected after OID");
        return (-1);
    }

    if (parse_syntax_strval(snmptoolctx, ptr + 1, obj) < 0)
        return (-1);

    return (1);
}


static int32_t
snmpset_parse_oid(struct snmp_toolinfo *snmptoolctx,
                  struct snmp_object *obj, char *argv) {
    if (argv == NULL)
        return (-1);

    if (ISSET_NUMERIC(snmptoolctx)) {
        if (parse_pair_numoid_val(argv, &(obj->val)) < 0)
            return (-1);
    } else {
        if (parse_pair_stroid_val(snmptoolctx, obj, argv) < 0)
            return (-1);
    }

    return (1);
}

static int32_t
add_ip_syntax(snmp_value_t *dst, snmp_value_t *src) {
    int8_t i;

    dst->syntax = SNMP_SYNTAX_IPADDRESS;
    for (i = 0; i < 4; i++)
        dst->v.ipaddress[i] = src->v.ipaddress[i];

    return (1);
}

static int32_t
add_octstring_syntax(snmp_value_t *dst, snmp_value_t *src) {
    if (src->v.octetstring.len > ASN_MAXOCTETSTRING) {
        warnx("OctetString len too big - %u",src->v.octetstring.len);
        return (-1);
    }

    if ((dst->v.octetstring.octets = malloc(src->v.octetstring.len)) == NULL) {
        return (-1);
    }

    memcpy(dst->v.octetstring.octets, src->v.octetstring.octets,
           src->v.octetstring.len);
    dst->syntax = SNMP_SYNTAX_OCTETSTRING;
    dst->v.octetstring.len = src->v.octetstring.len;

    return(0);
}

static int32_t
add_oid_syntax(snmp_value_t *dst, snmp_value_t *src) {
    asn_append_oid(&(dst->v.oid), &(src->v.oid));
    dst->syntax = SNMP_SYNTAX_OID;
    return (0);
}

/*
 * Check syntax - if one of SNMP_SYNTAX_NULL, SNMP_SYNTAX_NOSUCHOBJECT,
 * SNMP_SYNTAX_NOSUCHINSTANCE, SNMP_SYNTAX_ENDOFMIBVIEW or anything not known -
 * return error.
 */
static int32_t
snmpset_add_value(snmp_value_t *dst, snmp_value_t *src) {
    if (dst == NULL || src == NULL)
        return (-1);

    switch (src->syntax) {
    case SNMP_SYNTAX_INTEGER:
        dst->v.integer = src->v.integer;
        dst->syntax = SNMP_SYNTAX_INTEGER;
        break;
    case SNMP_SYNTAX_TIMETICKS:
        dst->v.uint32 = src->v.uint32;
        dst->syntax = SNMP_SYNTAX_TIMETICKS;
        break;
    case SNMP_SYNTAX_GAUGE:
        dst->v.uint32 = src->v.uint32;
        dst->syntax = SNMP_SYNTAX_GAUGE;
        break;
    case SNMP_SYNTAX_COUNTER:
        dst->v.uint32 = src->v.uint32;
        dst->syntax = SNMP_SYNTAX_COUNTER;
        break;
    case SNMP_SYNTAX_COUNTER64:
        dst->v.counter64 = src->v.counter64;
        dst->syntax = SNMP_SYNTAX_COUNTER64;
        break;
    case SNMP_SYNTAX_IPADDRESS:
        add_ip_syntax(dst, src);
        break;
    case SNMP_SYNTAX_OCTETSTRING:
        add_octstring_syntax(dst, src);
        break;
    case SNMP_SYNTAX_OID:
        add_oid_syntax(dst, src);
        break;
    default:
        warnx("Unknown syntax %d", src->syntax);
        return (-1);
    }

    return (0);
}

static int32_t
snmpset_verify_vbind(struct snmp_toolinfo *snmptoolctx, snmp_pdu_t *pdu,
                     struct snmp_object *obj) {
    if (pdu->version == SNMP_V1 && obj->val.syntax ==
            SNMP_SYNTAX_COUNTER64) {
        warnx("64-bit counters are not supported in SNMPv1 PDU");
        return (-1);
    }

    if (ISSET_NUMERIC(snmptoolctx) || ISSET_ERRIGNORE(snmptoolctx))
        return (1);

    if (obj->info->access < SNMP_ACCESS_SET) {
        warnx("Object %s not accessible for set - try 'bsnmpset -a'",
              obj->info->string);
        return (-1);
    }

    return (1);
}

static int32_t
snmpset_add_vbind(snmp_pdu_t *pdu, struct snmp_object *obj) {
    if (pdu->nbindings > SNMP_MAX_BINDINGS) {
        warnx("Too many OIDs for one PDU");
        return (-1);
    }

    if (obj->error > 0)
        return (0);

    if (snmpset_add_value(&(pdu->bindings[pdu->nbindings]), &(obj->val))
            < 0)
        return (-1);

    asn_append_oid(&(pdu->bindings[pdu->nbindings].oid), &(obj->val.oid));
    pdu->nbindings++;

    return (pdu->nbindings);
}

static int
snmptool_set(struct snmp_toolinfo *snmptoolctx) {
    snmp_pdu_t req, resp;

    snmp_pdu_create(&snmptoolctx->client, &req, SNMP_PDU_SET);

    while ((snmp_pdu_add_bindings(snmptoolctx, snmpset_verify_vbind,
                                  snmpset_add_vbind, &req, SNMP_MAX_BINDINGS)) > 0) {
        if (snmp_dialog(&snmptoolctx->client, &req, &resp)) {
            warnx("Snmp dialog - %s", strerror(errno));
            break;
        }

        if (snmp_pdu_check(&req, &resp) == SNMP_CODE_OK) {
            if (GET_OUTPUT(snmptoolctx) != OUTPUT_QUIET)
                snmp_output_resp(snmptoolctx, &resp);
            break;
        }

        snmp_output_err_resp(snmptoolctx, &resp);
        if (!ISSET_RETRY(snmptoolctx))
            break;

        if (snmp_object_seterror(snmptoolctx,
                                 &(resp.bindings[resp.error_index - 1]),
                                 resp.error_status) <= 0)
            break;

        fprintf(stderr, "Retrying...\n");
        snmp_pdu_free(&req);
        snmp_pdu_free(&resp);
        snmp_pdu_create(&snmptoolctx->client, &req, SNMP_PDU_SET);
    }

    snmp_pdu_free(&resp);

    return (0);
}


/* *****************************************************************************
 * main
 */
/*
 * According to command line options prepare SNMP Get | GetNext | GetBulk PDU.
 * Wait for a responce and print it.
 */
/*
 * Do a 'snmp walk' - according to command line options request for values
 * lexicographically subsequent and subrooted at a common node. Send a GetNext
 * PDU requesting the value for each next variable and print the responce. Stop
 * when a Responce PDU is received that contains the value of a variable not
 * subrooted at the variable the walk started.
 */
int
main(int argc, char ** argv) {
    struct snmp_toolinfo snmptoolctx;
    int32_t oid_cnt, last_oid, opt_num;
    int rc, offset;


#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    /* Make sure program_name is set and valid. */
    if (*argv == NULL)
        program_name = "snmptools";
    else {
        program_name = strrchr(*argv, '/');
        if (program_name != NULL)
            program_name++;
        else
            program_name = *argv;
    }

    offset = 0;

    if (program_name == NULL) {
        fprintf(stderr, "Error: No program name?\n");
        exit (1);
    } else if (strcmp(program_name, "bsnmpget") == 0)
        program = BSNMPGET;
    else if (strcmp(program_name, "bsnmpwalk") == 0)
        program = BSNMPWALK;
    else if (strcmp(program_name, "bsnmpset") == 0)
        program = BSNMPSET;
    else {
        if(argc < 2) {
            fprintf(stderr, "Unknown command '%s'.\n", program_name);
            usage_help();
            exit (1);
        }

        if (strcmp(argv[1], "get") == 0) {
            program = BSNMPGET;
        } else if (strcmp(argv[1], "walk") == 0) {
            program = BSNMPWALK;
        } else if (strcmp(argv[1], "set") == 0) {
            program = BSNMPSET;
        } else if (strcmp(argv[1], "help") == 0 && argc == 3) {
            if (strcmp(argv[2], "get") == 0) {
                program = BSNMPGET;
            } else if (strcmp(argv[2], "walk") == 0) {
                program = BSNMPWALK;
            } else if (strcmp(argv[2], "set") == 0) {
                program = BSNMPSET;
            } else {
                fprintf(stderr, "Unknown command '%s'.\n", program_name);
                usage_help();
                exit (1);
            }
            usage(program);
            exit(1);
        } else if (strcasecmp(argv[1], "help") == 0
                   || strcasecmp(argv[1], "--help") == 0
                   || strcasecmp(argv[1], "-h") == 0) {
            usage_help();
            exit (1);
        } else {
            fprintf(stderr, "Unknown command '%s'.\n", program_name);
            usage_help();
            exit (1);
        }
        offset = 1;
    }

    /* Initialize. */
    snmptool_init(&snmptoolctx);

    if ((opt_num = snmptool_parse_options(&snmptoolctx, argc - offset, argv + offset)) < 0) {
        snmp_tool_freeall(&snmptoolctx);
        /* On -h (help) exit without error. */
        if (opt_num == -2)
            exit(0);
        else
            exit(1);
    }

    oid_cnt = argc - offset - opt_num - 1;
    if (oid_cnt == 0) {
        switch (program) {
        case BSNMPGET:
            fprintf(stderr, "No OID given.\n");
            usage(program);
            snmp_tool_freeall(&snmptoolctx);
            exit(1);
        case BSNMPWALK:
            if (snmp_object_add(&snmptoolctx, snmpwalk_add_default,
                                NULL) < 0) {
                fprintf(stderr,
                        "Error setting default subtree.\n");
                snmp_tool_freeall(&snmptoolctx);
                exit(1);
            }
            break;
        case BSNMPSET:
            fprintf(stderr, "No OID given.\n");
            usage(program);
            snmp_tool_freeall(&snmptoolctx);
            exit(1);
        }
    }

    snmp_import_all(&snmptoolctx);

    /* A simple sanity check - can not send GETBULK when using SNMPv1. */
    if (snmptoolctx.client.version != SNMP_V3 && 0 == snmptoolctx.client.read_community[0]) {
        fprintf(stderr, "No community given.\n");
        snmp_tool_freeall(&snmptoolctx);
        exit(1);
    }

    /* A simple sanity check - can not send GETBULK when using SNMPv1. */
    if (snmptoolctx.client.version == SNMP_V3 && 0 == snmptoolctx.client.user.sec_name[0]) {
        fprintf(stderr, "No security name given.\n");
        snmp_tool_freeall(&snmptoolctx);
        exit(1);
    }

    /* A simple sanity check - can not send GETBULK when using SNMPv1. */
    if (program == BSNMPGET && snmptoolctx.client.version == SNMP_V1 &&
            GET_PDUTYPE(&snmptoolctx) == SNMP_PDU_GETBULK) {
        fprintf(stderr, "Cannot send GETBULK PDU with SNMPv1.\n");
        snmp_tool_freeall(&snmptoolctx);
        exit(1);
    }

    for (last_oid = argc - 1; oid_cnt > 0; last_oid--, oid_cnt--) {
        if ((snmp_object_add(&snmptoolctx, (program == BSNMPSET) ?
                             snmpset_parse_oid : snmptools_parse_oid,
                             argv[last_oid])) < 0) {
            fprintf(stderr, "Error parsing OID string '%s'.\n",
                    argv[last_oid]);
            snmp_tool_freeall(&snmptoolctx);
            exit(1);
        }
    }

    if (snmp_open(&snmptoolctx.client, NULL, NULL, NULL, NULL)) {
        warnx("Failed to open snmp session: %s.", strerror(errno));
        snmp_tool_freeall(&snmptoolctx);
        exit(1);
    }

	if (snmptoolctx.client.version == SNMP_V3 && snmp_discover_engine(&snmptoolctx.client, NULL, NULL, NULL)) {
        warnx("Failed to discover engine: %s.", snmptoolctx.client.error);
        snmp_tool_freeall(&snmptoolctx);
        exit(1);
    }

    switch (program) {
    case BSNMPGET:
        rc = snmptool_get(&snmptoolctx);
        break;
    case BSNMPWALK:
        rc = snmptool_walk(&snmptoolctx);
        break;
    case BSNMPSET:
        rc = snmptool_set(&snmptoolctx);
        break;
    }

    snmp_tool_freeall(&snmptoolctx);
    snmp_close(&snmptoolctx.client);

    exit(rc);
}
