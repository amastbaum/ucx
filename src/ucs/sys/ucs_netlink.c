#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "ucs_netlink.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <ucs/type/status.h>
#include <ucs/debug/log.h>

ucs_status_t netlink_socket_create(struct netlink_socket *nl_sock, int protocol)
{
    int fd;

    fd = socket(AF_NETLINK, SOCK_RAW, protocol);
    if (fd < 0) {
        ucs_diag("failed to create a socket\n");
        return UCS_ERR_IO_ERROR;
    }

    memset(nl_sock, 0, sizeof(*nl_sock));
    nl_sock->fd = fd;
    nl_sock->local.nl_family = AF_NETLINK;

    if (bind(fd, (struct sockaddr *)&nl_sock->local,
             sizeof(nl_sock->local)) < 0) {
        close(fd);
        ucs_diag("failed to bind netlink socket %d\n", fd);
        return UCS_ERR_IO_ERROR;
    }

    return UCS_OK;
}

void netlink_socket_close(struct netlink_socket *nl_sock)
{
    if (nl_sock->fd >= 0) {
        close(nl_sock->fd);
        nl_sock->fd = -1;
    }
}

void netlink_msg_init(struct netlink_message *msg, int type,
                      int flags, int nlmsg_len)
{
    struct nlmsghdr *nlh;

    memset(msg, 0, sizeof(*msg));
    nlh = (struct nlmsghdr *)msg->buf;
    nlh->nlmsg_len = NLMSG_LENGTH(nlmsg_len);
    nlh->nlmsg_type = type;
    nlh->nlmsg_flags = flags;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = getpid();
}

ucs_status_t netlink_send(struct netlink_socket *nl_sock,
                          struct netlink_message *msg)
{
    struct nlmsghdr *nlh = (struct nlmsghdr *)msg->buf;

    /* send the request */
    if (send(nl_sock->fd, nlh, nlh->nlmsg_len, 0) < 0) {
        ucs_diag("failed to send netlink message\n");
        close(nl_sock->fd);
        return UCS_ERR_IO_ERROR;
    }

    return UCS_OK;
}

int netlink_recv(struct netlink_socket *nl_sock, struct netlink_message *msg)
{
    int ret;

    memset(&msg->buf, 0, sizeof(msg->buf));
    ret = recv(nl_sock->fd, &msg->buf, sizeof(msg->buf), 0);
    if (ret < 0) {
        return -errno;
    }

    if (ret == 0) {
        return -ENODATA;
    }

    return ret;
}

ucs_nl_parse_status_t netlink_parse_msg(
                            struct netlink_message *msg, int msg_len,
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
