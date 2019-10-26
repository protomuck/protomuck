#include "proxy_v2.h"

const uint8_t proxyv2_sig[12] = {
  0x0d, 0x0a, 0x0d, 0x0a, 0x00, 0x0d,
  0x0a, 0x51, 0x55, 0x49, 0x54, 0x0a
};

int
proxyv2_read_with_timeout(int fp, void *buf, size_t count, struct timeval *timeout)
{
    fd_set set;
    int rv;

    FD_ZERO(&set);
    FD_SET(fp, &set);

    rv = select(fp + 1, &set, NULL, NULL, timeout);

    if (0 > rv) {
        perror("select");
    } else if (0 == rv) {
        errno = ETIMEDOUT;
        rv = -1;
    } else {
        rv = read(fp, &buf, count);
    }

    return rv;
}

const char *
proxyv2_strerror(int err_num)
{
    const char *err_str;

    switch(err_num) {
        case PROXYv2_ETIMEDOUT:
            err_str = "PROXYv2 read timeout";
            break;
        case PROXYv2_EBADSIG:
            err_str = "bad PROXYv2 sig";
            break;
        case PROXYv2_EBADVERSION:
            err_str = "bad PROXYv2 version";
            break;
        case PROXYv2_EBADCOMMAND:
            err_str = "bad PROXYv2 command";
            break;
        case PROXYv2_EBADFAMILY:
            err_str = "bad PROXYv2 family";
            break;
        case PROXYv2_EBADPROTO:
            err_str = "bad PROXYv2 protocol";
            break;
        case PROXYv2_EREAD:
            err_str = "PROXYv2 read failed";
            break;
        default:
            err_str = "unknown PROXYv2 error";
            break;
    }

    return err_str;
}

int
proxyv2_read(int sock, struct proxyv2_info *result)
{
    struct proxyv2_info proxy;
    int tlv_block_len;
    int rv;
    struct timeval timeout;

    memset(&proxy, 0, sizeof(proxy));

    /* Timeout to complete proxy init; don't reset */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    /* Grab header */
    rv = proxyv2_read_with_timeout(sock, &proxy, sizeof(proxy.head), &timeout);
    if ( 0 > rv )
      return PROXYv2_ETIMEDOUT;

    /* Validate header */
    if ( ! PROXYv2_VALID_SIG(proxy.head) )
        return PROXYv2_EBADSIG;
    if ( ! PROXYv2_VALID_VERSION(proxy.head) )
        return PROXYv2_EBADVERSION;
    if ( ! PROXYv2_VALID_COMMAND(proxy.head) )
        return PROXYv2_EBADCOMMAND;
    if ( ! PROXYv2_VALID_FAMILY(proxy.head) )
        return PROXYv2_EBADFAMILY;
    if ( ! PROXYv2_VALID_PROTO(proxy.head) )
        return PROXYv2_EBADPROTO;

    /* Grab proxy addr block */
    tlv_block_len = ntohs(proxy.head.len);
    rv = proxyv2_read_with_timeout(sock, &proxy.addr, PROXYv2_MIN(tlv_block_len, sizeof(proxy.addr)), &timeout);
    if ( 0 > rv )
        return PROXYv2_ETIMEDOUT;

    /* Consume remaining TLV block if any */
    if (tlv_block_len > sizeof(proxy.addr))
        if ( 0 > lseek(sock, ( tlv_block_len - sizeof(proxy.addr) ), SEEK_CUR) )
            return PROXYv2_EREAD;

    memcpy(result, &proxy, sizeof(proxy));
    return 0;
}
