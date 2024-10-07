/**
* Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "ucs_netlink.h"

#include <ucs/sys/sock.h>
#include <ucs/type/status.h>
#include <ucs/debug/log.h>
#include <ucs/debug/memtrack_int.h>

#include <errno.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <unistd.h>


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

ucs_status_t
ucs_netlink_send_cmd(int protocol, const void *nl_protocol_hdr,
                     size_t nl_protocol_hdr_size, char *recv_msg_buf,
                     size_t *recv_msg_buf_len, unsigned short nlmsg_type)
{
    ucs_status_t ret;
    struct netlink_socket nl_sock;
    struct nlmsghdr *nlh;
    char *send_msg = NULL;

    ret = ucs_netlink_socket_create(&nl_sock, NETLINK_ROUTE);
    if (ret != UCS_OK) {
        ucs_diag("failed to open netlink socket");
        return ret;
    }

    send_msg = ucs_malloc(NLMSG_LENGTH(nl_protocol_hdr_size), "Netlink send message");
    if (send_msg == NULL) {
        goto out;
    }

    nlh              = (struct nlmsghdr *)send_msg;
    nlh->nlmsg_len   = NLMSG_LENGTH(nl_protocol_hdr_size);
    nlh->nlmsg_type  = nlmsg_type;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq   = 1;
    nlh->nlmsg_pid   = getpid();
    memcpy(NLMSG_DATA(send_msg), nl_protocol_hdr, nl_protocol_hdr_size);

    ret = ucs_socket_send(nl_sock.fd, nlh, nlh->nlmsg_len);
    if (ret < 0) {
        ucs_diag("failed to send netlink message. returned %d", ret);
        goto out;
    }

    ret = ucs_socket_recv(nl_sock.fd, recv_msg_buf, recv_msg_buf_len);
    if (ret != UCS_OK) {
        ucs_diag("failed to receive route netlink message");
        goto out;
    }

out:
    if (send_msg != NULL) {
        free(send_msg);
    }

    ucs_netlink_socket_close(&nl_sock);
    return ret;
}
