/*
 * SPDX-FileCopyrightText: Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_bridge_sample, CONFIG_NET_L2_ETHERNET_LOG_LEVEL);

#include <zephyr/net/ethernet_bridge.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

struct ud {
	struct net_if *bridge;
	struct net_if *iface[CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT];
#if defined(CONFIG_NET_MGMT_EVENT) && defined(CONFIG_NET_DHCPV4)
	struct net_mgmt_event_callback mgmt_cb;
#endif
};

struct ud g_user_data = {0};

static void bridge_find_cb(struct eth_bridge_iface_context *br, void *user_data)
{
	struct ud *u = user_data;

	if (u->bridge == NULL) {
		u->bridge = br->iface;
		LOG_INF("Find bridge iface %d.", net_if_get_by_iface(br->iface));
	}
}

static void bridge_add_iface_cb(struct net_if *iface, void *user_data)
{
#if defined(CONFIG_NET_DSA)
	struct ethernet_context *eth_ctx;
#endif
	struct ud *u = user_data;
	int i;
	int ret;

	for (i = 0; i < CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT; i++) {
		if (u->iface[i] == NULL) {
			break;
		}
	}

	if (i == CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT) {
		return;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

#if defined(CONFIG_NET_DSA)
	eth_ctx = net_if_l2_data(iface);

	if (eth_ctx->dsa_port == DSA_USER_PORT || eth_ctx->dsa_port == NON_DSA_PORT) {
		u->iface[i] = iface;
	}
#else
	u->iface[i] = iface;
#endif
	LOG_INF("Find iface %d. Add into bridge.", net_if_get_by_iface(iface));

	ret = eth_bridge_iface_add(u->bridge, iface);
	if (ret < 0) {
		LOG_ERR("eth_bridge_iface_add failed: %d", ret);
	}
}

#if defined(CONFIG_NET_MGMT_EVENT) && defined(CONFIG_NET_DHCPV4)
static void event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			  struct net_if *iface)
{
	struct ethernet_context *eth_ctx;

	ARG_UNUSED(cb);

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	eth_ctx = net_if_l2_data(iface);

	if (net_eth_iface_is_bridged(eth_ctx) && mgmt_event == NET_EVENT_IF_UP) {
		net_dhcpv4_restart(net_eth_get_bridge(eth_ctx));
		return;
	}
}
#endif

static int iface_set_static(struct net_if *iface, const char *ip,
			    const char *mask, const char *gw)
{
	struct net_in_addr a, m, g;

	if (iface == NULL) {
		LOG_ERR("iface_set_static: NULL interface");
		return -ENODEV;
	}

	if (net_addr_pton(AF_INET, ip, &a) < 0 ||
	    net_addr_pton(AF_INET, mask, &m) < 0) {
		LOG_ERR("iface_set_static: invalid ip/mask (%s / %s)", ip, mask);
		return -EINVAL;
	}

	if (net_if_ipv4_addr_add(iface, &a, NET_ADDR_MANUAL, 0) == NULL) {
		LOG_ERR("iface_set_static: failed to add %s", ip);
		return -ENOMEM;
	}

	/* Netmask is associated with the address, so set it after addr_add. */
	net_if_ipv4_set_netmask_by_addr(iface, &a, &m);

	if (gw != NULL && net_addr_pton(AF_INET, gw, &g) == 0) {
		net_if_ipv4_set_gw(iface, &g);
	}

	LOG_INF("iface %d: %s/%s gw=%s", net_if_get_by_iface(iface), ip, mask,
		gw ? gw : "(none)");
	return 0;
}

int main(void)
{
	struct ud *u = &g_user_data;

	net_eth_bridge_foreach(bridge_find_cb, u);
	net_if_foreach(bridge_add_iface_cb, u);
	net_if_up(u->bridge);

#if defined(CONFIG_NET_MGMT_EVENT) && defined(CONFIG_NET_DHCPV4)
	net_mgmt_init_event_callback(&u->mgmt_cb, event_handler, NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&u->mgmt_cb);
#endif

	/* Option A - true bridge: ONE IP on the bridge interface, NOT on the
	 * members. The RJ45 and T1S ports form a single L2 network (2.2.2.0/24),
	 * so the board is reachable at 2.2.2.5 from either side, and frames are
	 * forwarded between the two ports.
	 */
	iface_set_static(u->bridge, "2.2.2.5", "255.255.255.0", NULL);

	while(1)
	{
		k_sleep(K_MSEC(10));
	}
	
	return 0;
}
