int
proxyv2_read_with_timeout(int fp, void *buf, size_t count, struct timeval *timeout)
{
    fd_set set;
    int rv;

    FD_ZERO(&set);
    FD_SET(fp, &set);

    rv = select(fp + 1, &set, NULL, NULL, &timeout);

    if (0 > rv)
        perror("select");
    else if (0 == rv)
        errno = ETIMEDOUT;
        rv = -1;
    else
        rv = read(fp, &buf, count);

    return rv;
}

const char *
proxyv2_strerror(int errno)
{
    const char *errstr;

    switch(errno) {
        case PROXYv2_ETIMEDOUT:
            errstr = "PROXYv2 read timeout";
            break;
        case PROXYv2_EBADSIG:
            errstr = "bad PROXYv2 sig";
            break;
        case PROXYv2_EBADVERSION:
            errstr = "bad PROXYv2 version";
            break;
        case PROXYv2_EBADCOMMAND:
            errstr = "bad PROXYv2 command";
            break;
        case PROXYv2_EBADFAMILY:
            errstr = "bad PROXYv2 family";
            break;
        case PROXYv2_EBADPROTO:
            errstr = "bad PROXYv2 protocol";
            break;
        case PROXYv2_EREAD:
            errstr = "PROXYv2 read failed";
            break;
        else
            errstr = "unknown PROXYv2 error";
            break;
    }

    return errstr;
}

int
proxyv2_read(int sock, struct proxyv2_info *result)
{
    struct proxyv2_head proxy_head;
    struct proxyv2_addr proxy_addr;
    int tlv_block_len;
    int rv;
    struct timeval timeout;

    memset(&proxy_head, 0, sizeof(proxy_head));
    memset(&proxy_addr, 0, sizeof(proxy_addr));

    /* Timeout to complete proxy init; don't reset */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    /* Grab header */
    rv = proxyv2_read_with_timeout(sock, proxy_head, sizeof(proxy_head), &timeout);
    if ( 0 > rv )
      return PROXYv2_ETIMEDOUT;

    /* Validate header */
    if ( ! PROXYv2_VALID_SIG(head) )
        return PROXYv2_EBADSIG;
    if ( ! PROXYv2_VALID_VERSION(head) )
        return PROXYv2_EBADVERSION;
    if ( ! PROXYv2_VALID_COMMAND(head) )
        return PROXYv2_EBADCOMMAND;
    if ( ! PROXYv2_VALID_FAMILY(head) )
        return PROXYv2_EBADFAMILY;
    if ( ! PROXYv2_VALID_PROTO(head) )
        return PROXYv2_EBADPROTO;

    /* Grab proxy addr block */
    tlv_block_len = ntohs(head->len);
    rv = proxyv2_read_with_timeout(sock, &proxy_addr, UMIN(tlv_block_len, sizeof(proxy_addr)), &timeout);
    if ( 0 > rv )
        return PROXYv2_ETIMEDOUT;

    /* Consume remaining TLV block if any */
    if (tlv_block_len > sizeof(proxy_addr))
        if ( 0 > lseek(sock, ( tlv_block_len - sizeof(proxy_addr) ), SEEK_CUR) )
            return PROXYv2_EREAD;

    memcpy(&result->head, &proxy_head, sizeof(proxy_head));
    memcpy(&result->addr, &proxy_addr, sizeof(proxy_addr));
    return 0;
}
