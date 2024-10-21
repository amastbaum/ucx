/**
* Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "ucs_netlink.h"

#include <ucs/debug/log.h>
#include <ucs/sys/compiler.h>
#include <ucs/sys/sock.h>
#include <ucs/type/status.h>

#include <errno.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <unistd.h>


static ucs_status_t ucs_netlink_socket_init(int *fd, int protocol)
{
    struct sockaddr_nl sa = {0};
    ucs_status_t status;

    status = ucs_socket_create(AF_NETLINK, SOCK_RAW, protocol, fd);
    if (status != UCS_OK) {
        ucs_error("failed to create netlink socket %d (%s)",
                  status, ucs_status_string(status));
        goto err;
    }

    sa.nl_family = AF_NETLINK;

    if (bind(*fd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        ucs_error("failed to bind netlink socket %d", *fd);
        status = UCS_ERR_IO_ERROR;
        goto err_close_socket;
    }

    return UCS_OK;

err_close_socket:
    ucs_close_fd(fd);
err:
    *fd = -1;
    return status;
}

ucs_status_t
ucs_netlink_send_cmd(int protocol, unsigned short nlmsg_type,
                     void *nl_protocol_hdr, size_t nl_protocol_hdr_size,
                     char *recv_msg_buf, size_t *recv_msg_buf_len)
{
    struct nlmsghdr nlh = {0};
    ucs_status_t status;
    int fd;
    struct iovec iov[2];
    size_t dummy;

    status = ucs_netlink_socket_init(&fd, protocol);
    if (status != UCS_OK) {
        ucs_error("failed to open netlink socket");
        return status;
    }

    memset(&nlh, 0, sizeof(nlh));
    nlh.nlmsg_len   = NLMSG_LENGTH(nl_protocol_hdr_size);
    nlh.nlmsg_type  = nlmsg_type;
    nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    iov[0].iov_base = &nlh;
    iov[0].iov_len  = sizeof(nlh);
    iov[1].iov_base = nl_protocol_hdr;
    iov[1].iov_len  = nl_protocol_hdr_size;

    do {
        status = ucs_socket_sendv_nb(fd, iov, 2, &dummy);
    } while (status == UCS_ERR_NO_PROGRESS);

    if (status != UCS_OK) {
        ucs_error("failed to send netlink message %d (%s)",
                  status, ucs_status_string(status));
        goto out;
    }

    do {
        status = ucs_socket_recv_nb(fd, recv_msg_buf, recv_msg_buf_len);
    } while (status == UCS_ERR_NO_PROGRESS);

    if (status != UCS_OK) {
        ucs_error("failed to receive netlink message");
        goto out;
    }

out:
    ucs_close_fd(&fd);
    return status;
}

ucs_status_t
ucs_netlink_parse_msg(void *msg, size_t msg_len,
                      ucs_netlink_parse_cb_t parse_cb, void *arg)
{
    struct nlmsghdr *nlh;
    ucs_status_t status = UCS_INPROGRESS;

    for (nlh = (struct nlmsghdr *)msg;
         (status == UCS_INPROGRESS) && NLMSG_OK(nlh, msg_len) &&
         (nlh->nlmsg_type != NLMSG_DONE) && (nlh->nlmsg_type != NLMSG_ERROR);
         nlh = NLMSG_NEXT(nlh, msg_len)) {
        status = parse_cb(nlh, NLMSG_DATA(nlh), arg);
    }

    if (nlh->nlmsg_type == NLMSG_ERROR) {
        struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(nlh);
        ucs_error("failed to parse netlink message header (%d)", err->error);
        return UCS_ERR_IO_ERROR;
    }

    return UCS_OK;
}