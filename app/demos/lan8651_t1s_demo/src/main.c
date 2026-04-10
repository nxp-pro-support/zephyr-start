#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(lan8651_t1s_demo, LOG_LEVEL_INF);

#define UDP_THREAD_PRIO 7
#define HTTP_THREAD_PRIO 7

#define EVENT_MASK (NET_EVENT_IF_UP | NET_EVENT_IF_DOWN | NET_EVENT_IPV4_ADDR_ADD | \
		    NET_EVENT_ETHERNET_CARRIER_ON | NET_EVENT_ETHERNET_CARRIER_OFF)

static struct net_mgmt_event_callback mgmt_cb;
static K_SEM_DEFINE(ipv4_ready, 0, 1);

K_THREAD_STACK_DEFINE(udp_stack, CONFIG_APP_SERVER_STACK_SIZE);
static struct k_thread udp_thread;

#if defined(CONFIG_APP_HTTP_SERVER)
K_THREAD_STACK_DEFINE(http_stack, CONFIG_APP_SERVER_STACK_SIZE);
static struct k_thread http_thread;
#endif

static const struct net_in_addr *get_ipv4_addr(struct net_if *iface)
{
	struct net_if_config *cfg = net_if_get_config(iface);

	if (cfg == NULL || cfg->ip.ipv4 == NULL) {
		return NULL;
	}

	for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		struct net_if_addr_ipv4 *ifaddr = &cfg->ip.ipv4->unicast[i];

		if (ifaddr->ipv4.is_used &&
		    !net_ipv4_is_addr_unspecified(&ifaddr->ipv4.address.in_addr)) {
			return &ifaddr->ipv4.address.in_addr;
		}
	}

	return NULL;
}

static void ipv4_to_text(struct net_if *iface, char *buf, size_t len)
{
	const struct net_in_addr *addr = get_ipv4_addr(iface);

	if (addr == NULL) {
		snprintk(buf, len, "0.0.0.0");
		return;
	}

	net_addr_ntop(AF_INET, addr, buf, len);
}

static void log_iface_state(struct net_if *iface)
{
	struct net_linkaddr *ll = net_if_get_link_addr(iface);
	char ip_buf[NET_IPV4_ADDR_LEN];

	ipv4_to_text(iface, ip_buf, sizeof(ip_buf));

	if (ll != NULL && ll->len >= 6U) {
		LOG_INF("iface=%p IPv4=%s MAC=%02x:%02x:%02x:%02x:%02x:%02x",
			iface, ip_buf,
			ll->addr[0], ll->addr[1], ll->addr[2],
			ll->addr[3], ll->addr[4], ll->addr[5]);
		return;
	}

	LOG_INF("iface=%p IPv4=%s", iface, ip_buf);
}

static void event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			  struct net_if *iface)
{
	ARG_UNUSED(cb);

	switch (mgmt_event) {
	case NET_EVENT_IF_UP:
		LOG_INF("Network interface is up");
		log_iface_state(iface);
		break;
	case NET_EVENT_IF_DOWN:
		LOG_WRN("Network interface is down");
		break;
	case NET_EVENT_ETHERNET_CARRIER_ON:
		LOG_INF("Ethernet carrier detected");
		break;
	case NET_EVENT_ETHERNET_CARRIER_OFF:
		LOG_WRN("Ethernet carrier lost");
		break;
	case NET_EVENT_IPV4_ADDR_ADD:
		LOG_INF("IPv4 address configured");
		log_iface_state(iface);
		k_sem_give(&ipv4_ready);
		break;
	default:
		break;
	}
}

static void wait_for_ipv4(struct net_if *iface)
{
	if (get_ipv4_addr(iface) != NULL) {
		log_iface_state(iface);
		return;
	}

	LOG_INF("Waiting for static IPv4 configuration");

	if (k_sem_take(&ipv4_ready, K_SECONDS(10)) != 0) {
		LOG_WRN("IPv4 address not configured within 10 seconds");
	}
}

static void udp_server(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (true) {
		int sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (sock < 0) {
			LOG_ERR("UDP socket create failed: %d", errno);
			k_sleep(K_SECONDS(1));
			continue;
		}

		struct sockaddr_in local = {
			.sin_family = AF_INET,
			.sin_port = htons(CONFIG_APP_UDP_PORT),
			.sin_addr = {
				.s_addr = htonl(INADDR_ANY),
			},
		};

		if (zsock_bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
			LOG_ERR("UDP bind failed: %d", errno);
			zsock_close(sock);
			k_sleep(K_SECONDS(1));
			continue;
		}

		LOG_INF("UDP echo server listening on port %d", CONFIG_APP_UDP_PORT);

		while (true) {
			uint8_t buf[256];
			struct sockaddr_in peer;
			socklen_t peer_len = sizeof(peer);
			char peer_ip[NET_IPV4_ADDR_LEN];
			int len = zsock_recvfrom(sock, buf, sizeof(buf), 0,
						 (struct sockaddr *)&peer, &peer_len);

			if (len < 0) {
				LOG_ERR("UDP recvfrom failed: %d", errno);
				break;
			}

			net_addr_ntop(AF_INET, &peer.sin_addr, peer_ip, sizeof(peer_ip));
			LOG_INF("UDP rx %d bytes from %s:%u",
				len, peer_ip, ntohs(peer.sin_port));

			if (zsock_sendto(sock, buf, len, 0,
					 (struct sockaddr *)&peer, peer_len) < 0) {
				LOG_ERR("UDP sendto failed: %d", errno);
				break;
			}
		}

		zsock_close(sock);
		k_sleep(K_MSEC(250));
	}
}

#if defined(CONFIG_APP_HTTP_SERVER)
static void http_server(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (true) {
		int server = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (server < 0) {
			LOG_ERR("HTTP socket create failed: %d", errno);
			k_sleep(K_SECONDS(1));
			continue;
		}

		struct sockaddr_in local = {
			.sin_family = AF_INET,
			.sin_port = htons(CONFIG_APP_HTTP_PORT),
			.sin_addr = {
				.s_addr = htonl(INADDR_ANY),
			},
		};

		if (zsock_bind(server, (struct sockaddr *)&local, sizeof(local)) < 0) {
			LOG_ERR("HTTP bind failed: %d", errno);
			zsock_close(server);
			k_sleep(K_SECONDS(1));
			continue;
		}

		if (zsock_listen(server, 1) < 0) {
			LOG_ERR("HTTP listen failed: %d", errno);
			zsock_close(server);
			k_sleep(K_SECONDS(1));
			continue;
		}

		LOG_INF("HTTP status server listening on port %d", CONFIG_APP_HTTP_PORT);

		while (true) {
			struct sockaddr_in peer;
			socklen_t peer_len = sizeof(peer);
			char peer_ip[NET_IPV4_ADDR_LEN];
			char request[CONFIG_APP_HTTP_MAX_REQUEST];
			char body[192];
			char response[384];
			struct net_if *iface = net_if_get_default();
			char ip_buf[NET_IPV4_ADDR_LEN];
			int client = zsock_accept(server, (struct sockaddr *)&peer, &peer_len);

			if (client < 0) {
				LOG_ERR("HTTP accept failed: %d", errno);
				break;
			}

			net_addr_ntop(AF_INET, &peer.sin_addr, peer_ip, sizeof(peer_ip));
			LOG_INF("HTTP client %s:%u connected", peer_ip, ntohs(peer.sin_port));

			(void)zsock_recv(client, request, sizeof(request), 0);

			ipv4_to_text(iface, ip_buf, sizeof(ip_buf));

			int body_len = snprintk(body, sizeof(body),
						"lan8651_t1s_demo\nipv4=%s\nudp_port=%d\nuptime_ms=%" PRId64 "\n",
						ip_buf, CONFIG_APP_UDP_PORT, k_uptime_get());
			int response_len = snprintk(response, sizeof(response),
						    "HTTP/1.1 200 OK\r\n"
						    "Content-Type: text/plain\r\n"
						    "Content-Length: %d\r\n"
						    "Connection: close\r\n"
						    "\r\n"
						    "%s",
						    body_len, body);

			(void)zsock_send(client, response, response_len, 0);
			zsock_close(client);
		}

		zsock_close(server);
		k_sleep(K_MSEC(250));
	}
}
#endif

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("No default network interface found");
		return -ENODEV;
	}

	net_mgmt_init_event_callback(&mgmt_cb, event_handler, EVENT_MASK);
	net_mgmt_add_event_callback(&mgmt_cb);

	wait_for_ipv4(iface);

	(void)k_thread_create(&udp_thread, udp_stack, K_THREAD_STACK_SIZEOF(udp_stack),
			      udp_server, NULL, NULL, NULL,
			      UDP_THREAD_PRIO, 0, K_NO_WAIT);

#if defined(CONFIG_APP_HTTP_SERVER)
	(void)k_thread_create(&http_thread, http_stack, K_THREAD_STACK_SIZEOF(http_stack),
			      http_server, NULL, NULL, NULL,
			      HTTP_THREAD_PRIO, 0, K_NO_WAIT);
#endif

	LOG_INF("LAN8651 T1S demo ready");

	while (true) {
		k_sleep(K_SECONDS(30));
		log_iface_state(iface);
	}

	return 0;
}
