#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;

struct Result { double throughput, delay, loss, pdr; };

Result Run(bool tcp) {
    NodeContainer n; n.Create(6);
    PointToPointHelper p;
    p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p.SetChannelAttribute("Delay", StringValue("5ms"));
    p.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize",
               QueueSizeValue(QueueSize("50p")));

    NodeContainer l[5] = {NodeContainer(n.Get(0),n.Get(2)),
        NodeContainer(n.Get(1),n.Get(2)), NodeContainer(n.Get(2),n.Get(3)),
        NodeContainer(n.Get(3),n.Get(5)), NodeContainer(n.Get(2),n.Get(4))};

    InternetStackHelper internet; internet.Install(n);
    Ipv4AddressHelper a; a.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer ip[5];
    for (int i=0;i<5;i++) {
        NetDeviceContainer d=p.Install(l[i]);
        ip[i]=a.Assign(d); a.NewNetwork();
    }
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port=9;
    PacketSinkHelper sink(tcp ? "ns3::TcpSocketFactory" :
        "ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),port));
    auto apps=sink.Install(n.Get(5));
    apps.Start(Seconds(0)); apps.Stop(Seconds(30));

    OnOffHelper src(tcp ? "ns3::TcpSocketFactory" :
        "ns3::UdpSocketFactory", InetSocketAddress(ip[3].GetAddress(1),port));
    src.SetAttribute("DataRate",StringValue("2Mbps"));
    src.SetAttribute("PacketSize",UintegerValue(1024));
    auto client=src.Install(n.Get(0));
    client.Start(Seconds(1)); client.Stop(Seconds(30));

    FlowMonitorHelper fm; Ptr<FlowMonitor> monitor=fm.InstallAll();
    Simulator::Stop(Seconds(30)); Simulator::Run();
    monitor->CheckForLostPackets();

    Result r={0,0,0,0};
    Ptr<Ipv4FlowClassifier> fc=DynamicCast<Ipv4FlowClassifier>(fm.GetClassifier());
    for (auto f:monitor->GetFlowStats()) {
        auto t=fc->FindFlow(f.first);
        if (t.destinationPort==port && f.second.rxPackets>0) {
            r.throughput=f.second.rxBytes*8.0/29/1000000;
            r.delay=f.second.delaySum.GetSeconds()*1000/f.second.rxPackets;
            r.loss=f.second.txPackets-f.second.rxPackets;
            r.pdr=100.0*f.second.rxPackets/f.second.txPackets;
        }
    }
    std::cout << (tcp?"TCP":"UDP") << ": Throughput=" << r.throughput
        << " Mbps, Delay=" << r.delay << " ms, Lost packets=" << r.loss
        << ", PDR=" << r.pdr << "%\n";
    Simulator::Destroy();
    return r;
}


int main() {
    Result t = Run(true), u = Run(false);
    FILE *f = fopen("tcp_udp_results.csv", "w");
    if (!f) { perror("CSV file"); return 1; }

    fprintf(f, "Protocol,Throughput,Delay,Loss,PDR\n");
    fprintf(f, "TCP,%.6f,%.6f,%.0f,%.2f\n",
            t.throughput, t.delay, t.loss, t.pdr);
    fprintf(f, "UDP,%.6f,%.6f,%.0f,%.2f\n",
            u.throughput, u.delay, u.loss, u.pdr);
    fclose(f);

    printf("\nResults saved to tcp_udp_results.csv\n");
    return 0;
}

