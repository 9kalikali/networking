#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.cli import CLI

class RouterTopo(Topo):
    def build(self):
        h11 = self.addHost('h11')
        h12 = self.addHost('h12')
        h21 = self.addHost('h21')
        h22 = self.addHost('h22')
        h31 = self.addHost('h31')
        r1 = self.addHost('r1')
        r2 = self.addHost('r2')
        r3 = self.addHost('r3')

        self.addLink(h11, r1)
        self.addLink(h12, r1)
        self.addLink(h21, r2)
        self.addLink(h22, r2)
        self.addLink(h31, r3)
        self.addLink(r1, r2)
        self.addLink(r2, r3)

if __name__ == '__main__':
    topo = RouterTopo()
    net = Mininet(topo = topo, controller = None)

    h11, h12, h21, h22, h31 = net.get('h11', 'h12', 'h21', 'h22', 'h31')
    r1, r2, r3 = net.get('r1', 'r2', 'r3')

    h11.cmd('ifconfig h11-eth0 10.0.1.11/24')
    h12.cmd('ifconfig h11-eth0 10.0.1.12/24')
    h21.cmd('ifconfig h11-eth0 10.0.2.21/24')
    h22.cmd('ifconfig h11-eth0 10.0.2.22/24')
    h31.cmd('ifconfig h11-eth0 10.0.3.31/24')

    h11.cmd('route add default gw 10.0.1.1')
    h12.cmd('route add default gw 10.0.1.1')
    h21.cmd('route add default gw 10.0.2.1')
    h22.cmd('route add default gw 10.0.2.1')
    h31.cmd('route add default gw 10.0.3.1')

    for h in (h11, h12, h21, h22, h31):
        h.cmd('./scripts/disable_offloading.sh')
        h.cmd('./scripts/disable_ipv6.sh')
    
    r1.cmd('ifconfig r1-eth0 10.0.1.1/24')
    r1.cmd('ifconfig r1-eth1 10.0.4.1/24')
    r1.cmd('route add -net 10.0.2.0 netmask 255.255.255.0 gw 10.0.4.2 dev r1-eth1')

    r2.cmd('ifconfig r2-eth0 10.0.2.1/24')
    r2.cmd('ifconfig r2-eth1 10.0.4.2/24')
    r2.cmd('route add -net 10.0.1.0 netmask 255.255.255.0 gw 10.0.4.1 dev r2-eth1')
    r2.cmd('route add -net 10.0.3.0 netmask 255.255.255.0 gw 10.0.4.3 dev r2-eth1')

    r3.cmd('ifconfig r3-eth0 10.0.3.1/24')
    r3.cmd('ifconfig r3-eth1 10.0.4.3/24')
    r3.cmd('route add -net 10.0.2.0 netmask 255.255.255.0 gw 10.0.4.2 dev r3-eth1')

    r1.cmd('./scripts/disable_arp.sh')
    r1.cmd('./scripts/disable_icmp.sh')
    r1.cmd('./scripts/disable_ip_forward.sh')

    r2.cmd('./scripts/disable_arp.sh')
    r2.cmd('./scripts/disable_icmp.sh')
    r2.cmd('./scripts/disable_ip_forward.sh')

    r3.cmd('./scripts/disable_arp.sh')
    r3.cmd('./scripts/disable_icmp.sh')
    r3.cmd('./scripts/disable_ip_forward.sh')

    net.start()
    CLI(net)
    net.stop()

