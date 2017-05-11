#include "net.hpp"
#include "utils.hpp"
#include "utils_ns.hpp"

#include <utility>
#include <tuple>
#include <arpa/inet.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

using std::pair;
using std::string;
using std::to_string;
using std::tie;
using namespace std::literals;

string const CONTAINER_VETH_NAME("eth0");

static pair<string, string> get_veth_names(container_opts const& opts) {
	return {"v0-"s + opts.id.name(), "v1-"s + opts.id.name()};
}

int get_host_addr(string const& cont_addr, string& host_addr) {
	struct in_addr addr;
	int ret;
	CALLv(1, ret, inet_pton(AF_INET, cont_addr.c_str(), static_cast<void*>(&addr)),
			"Failed to parse IPv4 address", return -1);

	char buffer[INET_ADDRSTRLEN];
	addr.s_addr = htonl(ntohl(addr.s_addr) + 1); // Endianess...
	char const* tmp;
	CALLv(buffer, tmp, inet_ntop(AF_INET, static_cast<void const*>(&addr), buffer, INET_ADDRSTRLEN),
			"Failed to write IPv4 address", return -1);
	host_addr = buffer;

	return 0;
}

static string get_masquerade_rule(string const& source, string const& veth) {
	return "-s "s + source + " ! -o "s + veth + " -j MASQUERADE"s;
}

static string get_forward_out_rule(string const& veth) {
	return "-i "s + veth + " -j ACCEPT"s;
}

static string get_forward_in_rule(string const& veth) {
	return "-o "s + veth + " -j ACCEPT"s;
}

int net_setup_ns(container_opts const& opts) {
	int ret(0);

	string ipname(opts.id.name());
	string nspath("/proc/"s + to_string(opts.id.pid) + "/ns/net"s);
	string ippath("/run/netns/"s + ipname);
	string veth0, veth1;
	tie(veth0, veth1) = get_veth_names(opts);

	string const& ip_cont(opts.net_ip);
	string ip_host;
	CALL_(ret, get_host_addr(ip_cont, ip_host),
			"Failed to get host IP", return ret);
	string addr_cont(ip_cont + "/24"s), addr_host(ip_host + "/24");

	int fd;
	CALL(fd, open(ippath.c_str(), O_CREAT, S_IRWXU),
			"Failed to create namespace mountpoint", return -1);
	close(fd);
	Defer(if (ret != 0) CALL(ret, unlink(ippath.c_str()),
				"Failed to delete namespace mountpount", (void) 0));

	CALL(ret, mount(nspath.c_str(), ippath.c_str(), NULL, MS_BIND, NULL),
			"Failed to mount-bind net namespace", return ret);
	Defer(if (ret != 0) CALL(ret, umount(ippath.c_str()),
				"Failed to umount net namespace", (void) 0));

	string ns_prefix("ip netns exec "s + ipname + " "s);

	SYSTEM(ret, "ip link add "s + veth0 + " type veth peer name "s + veth1,
			"Failed to create veth pair", return ret);
	Defer(if (ret != 0) SYSTEM(ret, "ip link delete "s + veth0,
				"Failed to delete veth pair", (void) 0));

	SYSTEM(ret, "ip link set "s + veth1 + " netns "s + ipname,
			"Failed to move veth1 to net namespace", return ret);

	SYSTEM(ret, ns_prefix + "ip link set "s + veth1 + " name "s + CONTAINER_VETH_NAME,
			"Failed to rename veth1", return ret);
	veth1 = CONTAINER_VETH_NAME;

	SYSTEM(ret, "ip link set "s + veth0 + " up"s,
			"Failed to activate veth0", return ret);

	SYSTEM(ret, "ip addr add "s + addr_host + " dev "s + veth0,
			"Failed to set host IP", return ret);

	// Debian why so old, where is -netns?
	SYSTEM(ret, ns_prefix + "ip link set lo up",
			"Failed to activate loopback", return ret);

	SYSTEM(ret, ns_prefix + "ip link set "s + veth1 + " alias "s + CONTAINER_VETH_NAME,
			"Failed to set alias", return ret);

	SYSTEM(ret, ns_prefix + "ip link set "s + veth1 + " up"s,
			"Failed to activate veth1", return ret);

	SYSTEM(ret, ns_prefix + "ip addr add "s + addr_cont + " dev "s + veth1,
			"Failed to set container IP", return ret);

	SYSTEM(ret, ns_prefix + "ip route add default via "s + ip_host,
			"Failed to set default route", return ret);

	SYSTEM(ret, "iptables -t nat -A POSTROUTING "s + get_masquerade_rule(ip_cont, veth0),
			"Failed to enable NAT masquerade", return ret);
	Defer(if (ret != 0) SYSTEM(ret, "iptables -t nat -D POSTROUTING "s + get_masquerade_rule(opts.net_ip, veth0),
			"Failed to delete NAT masquerade", (void) 0));

	SYSTEM(ret, "iptables -t filter -A FORWARD "s + get_forward_in_rule(veth0),
			"Failed to enable incoming forwarding", return ret);
	Defer (if (ret != 0) SYSTEM(ret, "iptables -t filter -D FORWARD "s + get_forward_in_rule(veth0),
			"Failed to disable incoming forwarding", (void) 0));

	SYSTEM(ret, "iptables -t filter -A FORWARD "s + get_forward_out_rule(veth0),
			"Failed to enable outcoming forwarding", return ret);

	return ret = 0;
}

int net_destroy(container_opts const& opts) {
	int ret(0);

	string ipname(opts.id.name());
	string nspath("/proc/"s + to_string(opts.id.pid) + "/ns/net"s);
	string ippath("/run/netns/"s + ipname);
	string veth0, veth1;
	tie(veth0, veth1) = get_veth_names(opts);

	string addr_cont(opts.net_ip + "/24"s);
	string addr_host;
	CALL_(ret, get_host_addr(opts.net_ip, addr_host),
			"Failed to get host IP", return ret);
	addr_host += "/24"s;

	SYSTEM(ret, "iptables -t filter -D FORWARD "s + get_forward_out_rule(veth0),
			"Failed to disable outcoming forwarding", return ret);

	SYSTEM(ret, "iptables -t filter -D FORWARD "s + get_forward_in_rule(veth0),
			"Failed to disable incoming forwarding", return ret);

	SYSTEM(ret, "iptables -t nat -D POSTROUTING "s + get_masquerade_rule(opts.net_ip, veth0),
			"Failed to delete NAT masquerade", return ret);

	SYSTEM(ret, "ip link delete "s + veth0,
			"Failed to delete veth pair", return ret);

	CALL(ret, umount(ippath.c_str()),
			"Failed to umount net namespace", return ret);

	CALL(ret, unlink(ippath.c_str()),
				"Failed to delete namespace mountpount", return ret);

	return 0;
}

int net_open(container_id const& id) {
	return ns_open(id, "net");
}

int net_enter_ns(int fd) {
	return ns_enter(fd);
}
