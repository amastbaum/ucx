#ifndef UCS_NETLINK_H
#define UCS_NETLINK_H

#include <ucs/type/status.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define NETLINK_BUFFER_SIZE 8192

typedef enum ucs_nl_parse_status {
    UCS_NL_STATUS_OK = 0,
    UCS_NL_STATUS_DONE = 1,
    UCS_NL_STATUS_ERROR = 2,
} ucs_nl_parse_status_t;

struct netlink_socket {
    int fd;
    struct sockaddr_nl local;
    struct sockaddr_nl peer;
};

struct netlink_message {
    char buf[NETLINK_BUFFER_SIZE];
};

// Socket Management
ucs_status_t netlink_socket_create(struct netlink_socket *nl_sock, int protocol);
void netlink_socket_close(struct netlink_socket *nl_sock);

// Message Construction
void netlink_msg_init(struct netlink_message *msg, int type,
                      int flags, int nlmsg_len);

// Message Sending and Receiving
ucs_status_t netlink_send(struct netlink_socket *nl_sock,
                          struct netlink_message *msg);
int netlink_recv(struct netlink_socket *nl_sock, struct netlink_message *msg);

// Message Parsing
ucs_nl_parse_status_t netlink_parse_msg(
                            struct netlink_message *msg, int msg_len,
                            void (*parse_cb)(struct nlmsghdr *h, void *arg),
                            void *arg);

#endif // UCS_NETLINK_H