/**
* Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifndef UCS_NETLINK_H
#define UCS_NETLINK_H

#include <ucs/type/status.h>

#include <stddef.h>
#include <linux/netlink.h>


typedef enum ucs_nl_parse_status {
    UCS_NL_STATUS_OK    = 0,
    UCS_NL_STATUS_DONE  = 1,
    UCS_NL_STATUS_ERROR = 2,
} ucs_nl_parse_status_t;


struct netlink_socket {
    int fd;
};


struct netlink_message {
    char   *buf;
    size_t buf_size;
};


ucs_status_t
ucs_netlink_socket_create(struct netlink_socket *nl_sock, int protocol);


void ucs_netlink_socket_close(struct netlink_socket *nl_sock);


ucs_status_t ucs_netlink_send_msg_create(struct netlink_message *msg, int type,
                                         int flags, int nlmsg_len);


void ucs_netlink_msg_destroy(struct netlink_message *msg);


ucs_status_t
ucs_netlink_send(struct netlink_socket *nl_sock, struct netlink_message *msg);


ucs_status_t ucs_netlink_recv(struct netlink_socket *nl_sock,
                              struct netlink_message *msg, size_t *len);


ucs_nl_parse_status_t
ucs_netlink_parse_msg(struct netlink_message *msg, size_t msg_len,
                      void (*parse_cb)(struct nlmsghdr *h, void *arg),
                      void *arg);

#endif // UCS_NETLINK_H