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


#define ucs_netlink_foreach(elem, msg, len) \
    for (elem = (struct nlmsghdr *)msg; \
         NLMSG_OK(elem, len) && (elem->nlmsg_type != NLMSG_DONE) && \
         (elem->nlmsg_type != NLMSG_ERROR); \
         elem = NLMSG_NEXT(elem, len))

#define ucs_netlink_handle_parse_error(nlh, action) \
    do { \
        if (nlh->nlmsg_type == NLMSG_ERROR) { \
            ucs_diag("failed to parse netlink message header (%d)", \
                    ((struct nlmsgerr*)NLMSG_DATA(nlh))->error); \
            action; \
        } \
    } while (0)


typedef enum ucs_nl_parse_status {
    UCS_NL_STATUS_OK    = 0,
    UCS_NL_STATUS_DONE  = 1,
    UCS_NL_STATUS_ERROR = 2,
} ucs_nl_parse_status_t;


struct netlink_socket {
    int fd;
};


/**
 * Creates a netlink socket.
 *
 * @param [out] nl_sock   Pointer to a netlink socket structure that will be
 *                        filled with socket information.
 * @param [in]  protocol  The communication protocol to be used.
 *
 * @return UCS_OK if created successfully, or error code otherwise.
 */
ucs_status_t
ucs_netlink_socket_create(struct netlink_socket *nl_sock, int protocol);


/**
 * Creates a netlink socket.
 *
 * @param [in]  nl_sock   Pointer to the netlink socket to be closed.
 */
void ucs_netlink_socket_close(struct netlink_socket *nl_sock);


/**
 * Sends a netlink request through a specified socket.
 *
 * @param [in]  nl_sock  Pointer to the netlink socket used for sending
 *                       the request.
 * @param [in]  msg      Pointer to the netlink message to be sent.
 *
 * @return UCS_OK if sent successfully, or error code otherwise.
 */
ucs_status_t ucs_netlink_send(struct netlink_socket *nl_sock, char *msg);


/**
 * Receives a netlink response and allocates a buffer for it.
 * IMPORTANT NOTE: The user is responsible for freeing this buffer after
 *                 use by calling ucs_netlink_msg_destroy.
 *
 * @param [in]   nl_sock  Pointer to the netlink socket from which to receive
 *                        the response.
 * @param [out]  msg      The struct that will hold the received message.
 * @param [out]  len      Pointer to store the length of the received message.
 *
 * @return UCS_OK if received successfully, or error code otherwise.
 */
ucs_status_t ucs_netlink_recv(struct netlink_socket *nl_sock, char *msg_buf,
                              size_t *msg_buf_len);


#endif // UCS_NETLINK_H
