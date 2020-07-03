/*
 * All these facts copied from the PROXY documentation at
 * http://www.haproxy.org/download/1.8/doc/proxy-protocol.txt
 *
 */

#ifndef PROXY_V2_H
#define PROXY_V2_H

#include "defaults.h"

extern const uint8_t proxyv2_sig[12];

struct proxyv2_head {
    uint8_t sig[12];  /* hex 0D 0A 0D 0A 00 0D 0A 51 55 49 54 0A */
    uint8_t ver_cmd;  /* protocol version and command */
    uint8_t fam;      /* protocol family and address */
    uint16_t len;     /* number of following bytes part of the header */
};

union proxyv2_addr {
    struct {        /* for TCP/UDP over IPv4, len = 12 */
        uint32_t src_addr;
        uint32_t dst_addr;
        uint16_t src_port;
        uint16_t dst_port;
    } af_inet;
    struct {        /* for TCP/UDP over IPv6, len = 36 */
         uint8_t  src_addr[16];
         uint8_t  dst_addr[16];
         uint16_t src_port;
         uint16_t dst_port;
    } af_inet6;
    struct {        /* for AF_UNIX sockets, len = 216 */
         uint8_t src_addr[108];
         uint8_t dst_addr[108];
    } af_unix;
};

struct proxyv2_tlv {
    uint8_t type;
    uint8_t length_hi;
    uint8_t length_lo;
    uint8_t value[0];
};

struct proxyv2_info {
    struct proxyv2_head head;
    union proxyv2_addr addr;
};

#define PP2_TYPE_ALPN           0x01
#define PP2_TYPE_AUTHORITY      0x02
#define PP2_TYPE_CRC32C         0x03
#define PP2_TYPE_NOOP           0x04
#define PP2_TYPE_SSL            0x20
#define PP2_SUBTYPE_SSL_VERSION 0x21
#define PP2_SUBTYPE_SSL_CN      0x22
#define PP2_SUBTYPE_SSL_CIPHER  0x23
#define PP2_SUBTYPE_SSL_SIG_ALG 0x24
#define PP2_SUBTYPE_SSL_KEY_ALG 0x25
#define PP2_TYPE_NETNS          0x30


/*
 * Convenience defines based on protocol documentation
 *
 */

#define PROXYv2_VALID_SIG(head) ( 0 == memcmp(&head.sig, &proxyv2_sig, 12) )

#define PROXYv2_VERSION(head) ( ( head.ver_cmd & 0xf0 ) >> 4 )
#define PROXYv2_VER 0x02
#define PROXYv2_VALID_VERSION(head) ( PROXYv2_VERSION(head) == PROXYv2_VER )

#define PROXYv2_COMMAND(head) ( head.ver_cmd & 0x0f )
#define PROXYv2_CMD_LOCAL 0x00
#define PROXYv2_CMD_PROXY 0x01
#define PROXYv2_VALID_COMMAND(head) ( 0 \
    || PROXYv2_COMMAND(head) == PROXYv2_CMD_LOCAL \
    || PROXYv2_COMMAND(head) == PROXYv2_CMD_PROXY \
)

#define PROXYv2_FAMILY(head) ( ( head.fam & 0xf0 ) >> 4 )
#define PROXYv2_AF_UNSPEC 0x00
#define PROXYv2_AF_INET 0x01
#define PROXYv2_AF_INET6 0x02
#define PROXYv2_AF_UNIX 0x03
#define PROXYv2_VALID_FAMILY(head) ( 0 \
    || PROXYv2_FAMILY(head) == PROXYv2_AF_UNSPEC \
    || PROXYv2_FAMILY(head) == PROXYv2_AF_INET \
    || PROXYv2_FAMILY(head) == PROXYv2_AF_INET6 \
    || PROXYv2_FAMILY(head) == PROXYv2_AF_UNIX \
)

#define PROXYv2_PROTO(head) ( head.fam & 0x0f )
#define PROXYv2_SOCK_UNSPEC 0x00
#define PROXYv2_SOCK_STREAM 0x01
#define PROXYv2_SOCK_DGRAM 0x02
#define PROXYv2_VALID_PROTO(head) ( 0 \
    || PROXYv2_PROTO(head) == PROXYv2_SOCK_UNSPEC \
    || PROXYv2_PROTO(head) == PROXYv2_SOCK_STREAM \
    || PROXYv2_PROTO(head) == PROXYv2_SOCK_DGRAM \
)

#define PROXYv2_VALIDATE(head) ( \
    PROXYv2_VALID_SIG(head) && \
    PROXYv2_VALID_VERSION(head) && \
    PROXYv2_VALID_COMMAND(head) && \
    PROXYv2_VALID_FAMILY(head) && \
    PROXYv2_VALID_PROTO(head) \
)

#define PROXYv2_EXPAND_IPV6(b, a) ( \
    sprintf(b, "[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", \
        (int)a[0], (int)a[1], (int)a[2], (int)a[3], (int)a[4], (int)a[5], (int)a[6], (int)a[7], \
        (int)a[8], (int)a[9], (int)a[10], (int)a[11], (int)a[12], (int)a[13], (int)a[14], (int)a[15] \
    ) \
)

#define PROXYv2_MIN(a,b) ((a)<(b)?(a):(b))

#define PROXYv2_ETIMEDOUT -1
#define PROXYv2_EBADSIG -2
#define PROXYv2_EBADVERSION -3
#define PROXYv2_EBADCOMMAND -4
#define PROXYv2_EBADFAMILY -5
#define PROXYv2_EBADPROTO -6
#define PROXYv2_EREAD -7

int proxyv2_read_with_timeout(int fp, void *buf, size_t count, struct timeval *timeout);
const char * proxyv2_strerror(int err_num);
int proxyv2_read(int sock, struct proxyv2_info *result);

#endif /* PROXY_V2_H */
