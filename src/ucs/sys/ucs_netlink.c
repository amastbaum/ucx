/**
* Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ucs_netlink.h"

#include <errno.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <sys/socket.h>
#include <ucs/sys/sock.h>
#include <ucs/type/status.h>
#include <ucs/debug/log.h>
#include <ucs/debug/memtrack_int.h>

#define NETLINK_MESSAGE_MAX_SIZE 8195

ucs_status_t
ucs_netlink_socket_create(struct netlink_socket *nl_sock, int protocol)
{
    int fd;
    struct sockaddr_nl sa;
    ucs_status_t ret;

    ret = ucs_socket_create(AF_NETLINK, SOCK_RAW, protocol, &fd);
    if (ret != UCS_OK) {
        return ret;
    }

    memset(nl_sock, 0, sizeof(*nl_sock));
    nl_sock->fd  = fd;
    sa.nl_family = AF_NETLINK;

    if (bind(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        ucs_close_fd(&fd);
        ucs_diag("failed to bind netlink socket %d", fd);
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

ucs_status_t ucs_netlink_send_msg_create(struct netlink_message *msg, int type,
                                         int flags, int nlmsg_len)
{
    struct nlmsghdr *nlh;
    size_t msg_size = NLMSG_LENGTH(nlmsg_len);
    msg->buf        = ucs_malloc(msg_size, "Netlink send message");
    if (msg->buf == NULL) {
        return UCS_ERR_NO_MEMORY;
    }

    memset(msg->buf, 0, msg_size);
    nlh              = (struct nlmsghdr*)msg->buf;
    nlh->nlmsg_len   = msg_size;
    nlh->nlmsg_type  = type;
    nlh->nlmsg_flags = flags;
    nlh->nlmsg_seq   = 1;
    nlh->nlmsg_pid   = getpid();

    return UCS_OK;
}

ucs_status_t
ucs_netlink_send(struct netlink_socket *nl_sock, struct netlink_message *msg)
{
    struct nlmsghdr *nlh = (struct nlmsghdr*)msg->buf;
    ucs_status_t ret = ucs_socket_send(nl_sock->fd, nlh, nlh->nlmsg_len);
    if (ret < 0) {
        ucs_diag("failed to send netlink message. returned %d", ret);
        ucs_close_fd(&nl_sock->fd);
        return UCS_ERR_IO_ERROR;
    }

    return UCS_OK;
}

void ucs_netlink_msg_destroy(struct netlink_message *msg)
{
    if (msg->buf != NULL) {
        ucs_free(msg->buf);
        msg->buf      = NULL;
    }
}

ucs_status_t ucs_netlink_recv(struct netlink_socket *nl_sock,
                              struct netlink_message *msg, size_t *len)
{
    msg->buf = ucs_malloc(NETLINK_MESSAGE_MAX_SIZE, "netlink recv message");
    if (msg->buf == NULL) {
        return UCS_ERR_NO_MEMORY;
    }

    return ucs_socket_recv(nl_sock->fd, msg->buf, len, 0);
}

ucs_nl_parse_status_t
ucs_netlink_parse_msg(struct netlink_message *msg, size_t msg_len,
                      void (*parse_cb)(struct nlmsghdr *h, void *arg),
                      void *arg)
{
    struct nlmsghdr *nlh;

    for (nlh = (struct nlmsghdr*)msg->buf; NLMSG_OK(nlh, msg_len);
         nlh = NLMSG_NEXT(nlh, msg_len)) {
        if (nlh->nlmsg_type == NLMSG_DONE) {
            return UCS_NL_STATUS_DONE;
        }

        if (nlh->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(nlh);
            ucs_diag("failed to parse netlink message header (%d)", err->error);
            return UCS_NL_STATUS_ERROR;
        }

        parse_cb(nlh, arg);
    }

    return UCS_NL_STATUS_OK;
}
