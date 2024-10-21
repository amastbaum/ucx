/**
* Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <ucs/arch/bitops.h>
#include <ucs/debug/log.h>
#include <ucs/debug/memtrack_int.h>
#include <ucs/sys/ucs_netlink.h>
#include <ucs/sys/ucs_rtnetlink.h>
#include <ucs/type/status.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <stddef.h>

#define NETLINK_MESSAGE_MAX_SIZE 8195

struct route_info {
    struct sockaddr_storage *sa_remote;
    int if_index;
    int matching;
};


static void ucs_rtnetlink_get_route_info(int **if_idx, void **dst_in_addr,
                                         struct rtattr *rta, int len)
{
    *if_idx      = NULL;
    *dst_in_addr = NULL;

    for (; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
        if (rta->rta_type == RTA_OIF) {
            *if_idx = RTA_DATA(rta);
        } else if (rta->rta_type == RTA_DST) {
            *dst_in_addr = RTA_DATA(rta);
        }
    }
}

static void ucs_rtnetlink_create_ipv6_mask(struct in6_addr *mask,
                                           unsigned char prefix_len)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (prefix_len >= 8) {
            mask->s6_addr[i] = 0xFF;
            prefix_len      -= 8;
        } else if (prefix_len > 0) {
            mask->s6_addr[i] = (0xFF00 >> prefix_len) & 0xFF;
            prefix_len       = 0;
        } else {
            mask->s6_addr[i] = 0;
        }
    }
}

static int ucs_rtnetlink_is_rule_matching(struct rtmsg *rtm, size_t rtm_len,
                                          struct sockaddr_storage *sa_remote,
                                          int oif)
{
    int  *rule_iface;
    void *dst_in_addr;

    if (rtm->rtm_family != sa_remote->ss_family) {
        return 0;
    }

    ucs_rtnetlink_get_route_info(&rule_iface, &dst_in_addr, RTM_RTA(rtm), rtm_len);
    if (rule_iface == NULL || dst_in_addr == NULL) {
        return 0;
    }

    if (*rule_iface == oif) {
        if (sa_remote->ss_family == AF_INET) {
            struct in_addr *addr = (struct in_addr *)dst_in_addr;
            uint32_t mask = UCS_MASK(rtm->rtm_dst_len);
            if ((((struct sockaddr_in *)sa_remote)->sin_addr.s_addr & mask) ==
                (addr->s_addr & mask)) {
                return 1;
            }
        } else { /* AF_INET6 */
            int i;
            struct in6_addr *addr = (struct in6_addr *)dst_in_addr;
            struct in6_addr *dest = &((struct sockaddr_in6 *)sa_remote)->sin6_addr;
            struct in6_addr mask, masked_dest, masked_network;
            ucs_rtnetlink_create_ipv6_mask(&mask, rtm->rtm_dst_len);

            for (i = 0; i < 16; i++) {
                masked_dest.s6_addr[i]    = dest->s6_addr[i] & mask.s6_addr[i];
                masked_network.s6_addr[i] = addr->s6_addr[i] &
                                            mask.s6_addr[i];
            }

            if (ucs_bitwise_is_equal(&masked_dest, &masked_network, 
                                     sizeof(struct in6_addr))) {
                return 1;
            }
        }
    }

    return 0;
}

ucs_status_t
ucs_rtnetlink_parse_entry(struct nlmsghdr *nlh, void *nl_msg, void *arg)
{
    struct route_info *info = (struct route_info *)arg;
    if (ucs_rtnetlink_is_rule_matching((struct rtmsg *)nl_msg, RTM_PAYLOAD(nlh),
                                       info->sa_remote, info->if_index)) {
        info->matching = 1;
        return UCS_OK;
    }

    return UCS_INPROGRESS;
}

int ucs_netlink_rule_exists(const char *iface, struct sockaddr_storage *sa_remote)
{
    char *recv_msg         = NULL;
    struct route_info info = {0};
    struct rtmsg rtm       = {0};
    ucs_status_t status;
    size_t recv_msg_len;
    int oif;

    rtm.rtm_family = sa_remote->ss_family;
    rtm.rtm_table  = RT_TABLE_MAIN;

    recv_msg_len = NETLINK_MESSAGE_MAX_SIZE;
    recv_msg     = ucs_malloc(NETLINK_MESSAGE_MAX_SIZE, "netlink recv message");
    if (recv_msg == NULL) {
        ucs_error("failed to allocate a buffer for netlink receive message");
        goto out;
    }

    status = ucs_netlink_send_cmd(NETLINK_ROUTE, RTM_GETROUTE, &rtm,
                                  sizeof(rtm), recv_msg, &recv_msg_len);
    if (status != UCS_OK) {
        ucs_error("failed to send netlink route message (%d)", status);
        goto out;
    }

    oif = if_nametoindex(iface);
    if (oif == 0) {
        ucs_error("failed to get interface index");
        goto out;
    }

    info.if_index  = oif;
    info.sa_remote = sa_remote;

    status = ucs_netlink_parse_msg(recv_msg, recv_msg_len,
                                   ucs_rtnetlink_parse_entry, &info);
    if (status != UCS_OK) {
        ucs_error("failed to parse netlink route message (%d)", status);
        goto out;
    }

out:
    if (recv_msg != NULL) {
        free(recv_msg);
    }

    return info.matching;
}
