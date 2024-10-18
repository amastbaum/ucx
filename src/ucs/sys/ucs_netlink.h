/**
* Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifndef UCS_NETLINK_H
#define UCS_NETLINK_H

#include <ucs/type/status.h>

#include <linux/netlink.h>
#include <stddef.h>


#define ucs_netlink_foreach(_elem, _msg, _len) \
    for (_elem = (struct nlmsghdr *)_msg; \
         NLMSG_OK(_elem, _len) && (_elem->nlmsg_type != NLMSG_DONE) && \
         (_elem->nlmsg_type != NLMSG_ERROR); \
         _elem = NLMSG_NEXT(_elem, _len))


/**
 * Sends and receives a netlink message using a user allocated buffer.
 *
 * @param [in]    protocol             The communication protocol to be used
 *                                     (NETLINK_ROUTE, NETLINK_NETFILTER, etc.).
 * @param [in]    nlmsg_type           Netlink message type (RTM_GETROUTE,
 *                                     RTM_GETNEIGH, etc.).
 * @param [in]    nl_protocol_hdr      A struct that holds nl protocol specific
 *                                     details and is placed in nlmsghdr.
 * @param [in]    nl_protocol_hdr_size Protocol struct size.
 * @param [out]   recv_msg_buf         The buffer that will hold the received
 *                                     message.
 * @param [inout] recv_msg_buf_len     Pointer to the size of the buffer and to
 *                                     store the length of the received message.
 *
 * @return UCS_OK if received successfully, or error code otherwise.
 */
ucs_status_t
ucs_netlink_send_cmd(int protocol, unsigned short nlmsg_type,
                     void *nl_protocol_hdr, size_t nl_protocol_hdr_size,
                     char *recv_msg_buf, size_t *recv_msg_buf_len);


#endif /* UCS_NETLINK_H */
