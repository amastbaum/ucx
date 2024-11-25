/**
* Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifndef UCS_RTNETLINK_H
#define UCS_RTNETLINK_H

#include <ucs/type/status.h>

#include <netinet/in.h>

BEGIN_C_DECLS


/**
 * Check whether a routing table rule exists for a given network
 * interface name and a destination address.
 *
 * @param [in]  iface      Pointer to the name of the interface.
 * @param [in]  sa_remote  Pointer to the destination address.
 *
 * @return 1 if rule exists, or 0 otherwise.
 */
int ucs_netlink_rule_exists(const char *iface,
                            const struct sockaddr *sa_remote);


END_C_DECLS

#endif /* UCS_RTNETLINK_H */
