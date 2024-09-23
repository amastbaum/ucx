/**
* Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "ucs_netlink.h"

#include <errno.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <sys/socket.h>
#include <ucs/sys/sock.h>
#include <ucs/type/status.h>
#include <ucs/debug/log.h>

ucs_status_t ucs_netlink_socket_create(struct netlink_socket *nl_sock,
                                       int protocol)
{
    int fd;
    struct sockaddr_nl sa;
    ucs_status_t ret;

    ret = ucs_socket_create(AF_NETLINK, SOCK_RAW, protocol, &fd);
    if (ret != UCS_OK) {
        return UCS_ERR_IO_ERROR;
    }

    memset(nl_sock, 0, sizeof(*nl_sock));
    nl_sock->fd = fd;
    sa.nl_family = AF_NETLINK;

    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        ucs_close_fd(&fd);
        ucs_diag("failed to bind netlink socket %d\n", fd);
        return UCS_ERR_IO_ERROR;
    }

    return UCS_OK;
}

void ucs_netlink_socket_close(struct netlink_socket *nl_sock)
{
    if (nl_sock->fd >= 0) {
        ucs_close_fd(&nl_sock->fd);
        nl_sock->fd = -1;
    }
}

void ucs_netlink_msg_init(struct netlink_message *msg, char *buf,
                          size_t buf_size, int type, int flags, int nlmsg_len)
{
    struct nlmsghdr *nlh;

    msg->buf      = buf;
    msg->buf_size = buf_size;
    memset(msg->buf, 0, msg->buf_size);

    nlh = (struct nlmsghdr *)msg->buf;
    nlh->nlmsg_len = NLMSG_LENGTH(nlmsg_len);
    nlh->nlmsg_type = type;
    nlh->nlmsg_flags = flags;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = getpid();
}

ucs_status_t ucs_netlink_send(struct netlink_socket *nl_sock,
                              struct netlink_message *msg)
{
    struct nlmsghdr *nlh = (struct nlmsghdr *)msg->buf;

    /* send the request */
    if (ucs_socket_send(nl_sock->fd, nlh, nlh->nlmsg_len) < 0) {
        ucs_diag("failed to send netlink message\n");
        ucs_close_fd(&nl_sock->fd);
        return UCS_ERR_IO_ERROR;
    }

    return UCS_OK;
}

<<<<<<< HEAD
ucs_status_t ucs_netlink_recv(struct netlink_socket *nl_sock,
                              struct netlink_message *msg, size_t *len)
{
    *len = msg->buf_size;
    memset(msg->buf, 0, msg->buf_size);
    return ucs_socket_recv(nl_sock->fd, msg->buf, len);
=======
static ssize_t peek_nlmsg_size(int sock_fd) {
    struct msghdr msg = {0};
    struct iovec iov = {0};
    ssize_t len;
    char buf[sizeof(struct msghdr)];

    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    printf("calling recv(..., PEEK) with length %ld\n", sizeof(struct msghdr));
    len = recv(sock_fd, &msg, sizeof(struct msghdr), MSG_PEEK | MSG_TRUNC);
    printf("recv(..., PEEK) returned %ld\n", len);
    return len;
}

ucs_status_t ucs_netlink_recv(struct netlink_socket *nl_sock,
                              struct netlink_message *msg, size_t *len)
{
    *len = peek_nlmsg_size(nl_sock->fd);
    memset(&msg->buf, 0, sizeof(msg->buf));
    return ucs_socket_recv(nl_sock->fd, &msg->buf, len, 0);
>>>>>>> 5c9a286ec (Add 'flags' to PEEK - Not working yet)
}

ucs_nl_parse_status_t ucs_netlink_parse_msg(
                            struct netlink_message *msg, size_t msg_len,
                            void (*parse_cb)(struct nlmsghdr *h, void *arg),
                            void *arg)
{
    struct nlmsghdr *nlh;

    for (nlh = (struct nlmsghdr *)msg->buf; NLMSG_OK(nlh, msg_len);
         nlh = NLMSG_NEXT(nlh, msg_len)) {
        if (nlh->nlmsg_type == NLMSG_DONE) {
            return UCS_NL_STATUS_DONE;
        }

        if (nlh->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(nlh);
            ucs_diag("failed to parse netlink message header (%d)", err->error);
            return UCS_NL_STATUS_ERROR;
        }

        parse_cb(nlh, arg);
    }

    return UCS_NL_STATUS_OK;
}
