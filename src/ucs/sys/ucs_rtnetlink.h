/**
* Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifndef UCS_RTNETLINK_H
#define UCS_RTNETLINK_H

#include <netinet/in.h>


int ucs_netlink_rule_exists(const char *iface,
                            struct sockaddr_storage *sa_remote);


#endif /* UCS_RTNETLINK_H */
