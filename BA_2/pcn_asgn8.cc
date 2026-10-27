#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpUdpAssignment");

int main(int argc, char *argv[])
{
    // ============================================================
    // PARAMETERS
    // ============================================================

    double bottleneckRate = 2.0;   // Mbps
    double bottleneckDelay = 10.0; // ms

    double udpRate = 1.0;         // Mbps
    uint32_t queueSize = 50;      // packets

    double accessRate = 10.0;     // Mbps
    double accessDelay = 2.0;     // ms

    double simulationTime = 20.0;

    uint32_t tcpPacketSize = 1024;
    uint32_t udpPacketSize = 512;

    CommandLine cmd(__FILE__);

    cmd.AddValue("bottleneckRate",
                 "Bottleneck bandwidth in Mbps",
                 bottleneckRate);

    cmd.AddValue("bottleneckDelay",
                 "Bottleneck delay in milliseconds",
                 bottleneckDelay);

    cmd.AddValue("udpRate",
                 "UDP traffic rate in Mbps",
                 udpRate);

    cmd.AddValue("queueSize",
                 "DropTail queue size in packets",
                 queueSize);

    cmd.Parse(argc, argv);


    // ============================================================
    // 1. CREATE NODES
    // ============================================================

    NodeContainer nodes;

    // End nodes
    nodes.Create(4);

    // Router nodes
    NodeContainer routers;
    routers.Create(2);

    Ptr<Node> n0 = nodes.Get(0);
    Ptr<Node> n1 = nodes.Get(1);
    Ptr<Node> n2 = nodes.Get(2);
    Ptr<Node> n3 = nodes.Get(3);

    Ptr<Node> r1 = routers.Get(0);
    Ptr<Node> r2 = routers.Get(1);


    // ============================================================
    // 2. CREATE LINKS
    // ============================================================

    PointToPointHelper accessLink;

    accessLink.SetDeviceAttribute(
        "DataRate",
        StringValue("10Mbps"));

    accessLink.SetChannelAttribute(
        "Delay",
        StringValue("2ms"));


    // ------------------------------------------------------------
    // Node 0 <----> Router 1
    // ------------------------------------------------------------

    NetDeviceContainer d0r1 =
        accessLink.Install(n0, r1);


    // ------------------------------------------------------------
    // Node 1 <----> Router 1
    // ------------------------------------------------------------

    NetDeviceContainer d1r1 =
        accessLink.Install(n1, r1);


    // ------------------------------------------------------------
    // Router 2 <----> Node 3
    // ------------------------------------------------------------

    NetDeviceContainer dr2n3 =
        accessLink.Install(r2, n3);


    // ------------------------------------------------------------
    // Router 2 <----> Node 2
    // ------------------------------------------------------------

    NetDeviceContainer dr2n2 =
        accessLink.Install(r2, n2);


    // ============================================================
    // 3. BOTTLENECK LINK R1 <----> R2
    // ============================================================

    PointToPointHelper bottleneck;

    bottleneck.SetDeviceAttribute(
        "DataRate",
        StringValue(
            std::to_string(bottleneckRate) + "Mbps"));

    bottleneck.SetChannelAttribute(
        "Delay",
        StringValue(
            std::to_string(bottleneckDelay) + "ms"));


    // DropTail queue of required size
    bottleneck.SetQueue(
        "ns3::DropTailQueue<Packet>",
        "MaxSize",
        QueueSizeValue(
            QueueSize(
                std::to_string(queueSize) + "p")));


    NetDeviceContainer dr1r2 =
        bottleneck.Install(r1, r2);


    // ============================================================
    // 4. INSTALL INTERNET STACK
    // ============================================================

    InternetStackHelper internet;

    internet.Install(nodes);
    internet.Install(routers);


    // ============================================================
    // 5. ASSIGN IP ADDRESSES
    // ============================================================

    Ipv4AddressHelper address;

    // Node 0 <-> Router 1
    address.SetBase(
        "10.0.1.0",
        "255.255.255.0");

    Ipv4InterfaceContainer i0r1 =
        address.Assign(d0r1);


    // Node 1 <-> Router 1
    address.SetBase(
        "10.0.2.0",
        "255.255.255.0");

    Ipv4InterfaceContainer i1r1 =
        address.Assign(d1r1);


    // Router 1 <-> Router 2
    address.SetBase(
        "10.0.3.0",
        "255.255.255.0");

    Ipv4InterfaceContainer ir1r2 =
        address.Assign(dr1r2);


    // Router 2 <-> Node 3
    address.SetBase(
        "10.0.4.0",
        "255.255.255.0");

    Ipv4InterfaceContainer ir2n3 =
        address.Assign(dr2n3);


    // Router 2 <-> Node 2
    address.SetBase(
        "10.0.5.0",
        "255.255.255.0");

    Ipv4InterfaceContainer ir2n2 =
        address.Assign(dr2n2);


    // ============================================================
    // 6. ENABLE ROUTING
    // ============================================================

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();


    // ============================================================
    // 7. TCP FLOW
    //
    // Node 0 --> Router 1 --> Router 2 --> Node 3
    // ============================================================

    uint16_t tcpPort = 5000;

    // TCP receiver
    PacketSinkHelper tcpSink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(
            Ipv4Address::GetAny(),
            tcpPort));

    ApplicationContainer tcpServer =
        tcpSink.Install(n3);

    tcpServer.Start(Seconds(0.0));
    tcpServer.Stop(Seconds(simulationTime));


    // TCP sender
    BulkSendHelper tcpSource(
        "ns3::TcpSocketFactory",
        InetSocketAddress(
            ir2n3.GetAddress(1),
            tcpPort));

    tcpSource.SetAttribute(
        "MaxBytes",
        UintegerValue(0));

    tcpSource.SetAttribute(
        "SendSize",
        UintegerValue(tcpPacketSize));

    ApplicationContainer tcpClient =
        tcpSource.Install(n0);

    tcpClient.Start(Seconds(1.0));
    tcpClient.Stop(Seconds(simulationTime));


    // ============================================================
    // 8. UDP FLOW
    //
    // Node 1 --> Router 1 --> Router 2 --> Node 2
    // ============================================================

    uint16_t udpPort = 6000;

    // UDP receiver
    PacketSinkHelper udpSink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(
            Ipv4Address::GetAny(),
            udpPort));

    ApplicationContainer udpServer =
        udpSink.Install(n2);

    udpServer.Start(Seconds(0.0));
    udpServer.Stop(Seconds(simulationTime));


    // UDP sender
    OnOffHelper udpSource(
        "ns3::UdpSocketFactory",
        InetSocketAddress(
            ir2n2.GetAddress(1),
            udpPort));

    udpSource.SetAttribute(
        "DataRate",
        DataRateValue(
            DataRate(
                std::to_string(udpRate) + "Mbps")));

    udpSource.SetAttribute(
        "PacketSize",
        UintegerValue(udpPacketSize));

    udpSource.SetAttribute(
        "OnTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1]"));

    udpSource.SetAttribute(
        "OffTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer udpClient =
        udpSource.Install(n1);

    udpClient.Start(Seconds(1.0));
    udpClient.Stop(Seconds(simulationTime));


    // ============================================================
    // 9. ENABLE PCAP TRACING
    // ============================================================

    accessLink.EnablePcapAll(
        "tcp-udp-access",
        true);

    bottleneck.EnablePcapAll(
        "tcp-udp-bottleneck",
        true);


    // ============================================================
    // 10. FLOW MONITOR
    // ============================================================

    FlowMonitorHelper flowHelper;

    Ptr<FlowMonitor> monitor =
        flowHelper.InstallAll();


    // ============================================================
    // 11. RUN SIMULATION
    // ============================================================

    Simulator::Stop(
        Seconds(simulationTime));

    Simulator::Run();


    // ============================================================
    // 12. GET APPLICATION STATISTICS
    // ============================================================

    Ptr<PacketSink> tcpSinkPtr =
        DynamicCast<PacketSink>(
            tcpServer.Get(0));

    Ptr<PacketSink> udpSinkPtr =
        DynamicCast<PacketSink>(
            udpServer.Get(0));


    uint64_t tcpBytesReceived =
        tcpSinkPtr->GetTotalRx();

    uint64_t udpBytesReceived =
        udpSinkPtr->GetTotalRx();


    // ============================================================
    // 13. FLOW MONITOR STATISTICS
    // ============================================================

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());

    std::map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();


    double tcpTxPackets = 0;
    double tcpRxPackets = 0;
    double tcpLostPackets = 0;
    double tcpDelaySum = 0;

    double udpTxPackets = 0;
    double udpRxPackets = 0;
    double udpLostPackets = 0;
    double udpDelaySum = 0;


    for (auto const &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(flow.first);

        FlowMonitor::FlowStats st =
            flow.second;


        // TCP flow: destination port 5000
        if (tuple.destinationPort == tcpPort)
        {
            tcpTxPackets += st.txPackets;
            tcpRxPackets += st.rxPackets;
            tcpLostPackets += st.lostPackets;
            tcpDelaySum += st.delaySum.GetSeconds();
        }


        // UDP flow: destination port 6000
        if (tuple.destinationPort == udpPort)
        {
            udpTxPackets += st.txPackets;
            udpRxPackets += st.rxPackets;
            udpLostPackets += st.lostPackets;
            udpDelaySum += st.delaySum.GetSeconds();
        }
    }


    // ============================================================
    // 14. CALCULATE METRICS
    // ============================================================

    double measurementTime =
        simulationTime - 1.0;


    // ---------------- TCP ----------------

    double tcpThroughput =
        (tcpBytesReceived * 8.0) /
        measurementTime /
        1000000.0;


    double tcpPacketLoss = 0.0;

    if (tcpTxPackets > 0)
    {
        tcpPacketLoss =
            (tcpLostPackets / tcpTxPackets) * 100.0;
    }


    double tcpAverageDelay = 0.0;

    if (tcpRxPackets > 0)
    {
        tcpAverageDelay =
            (tcpDelaySum / tcpRxPackets) * 1000.0;
    }


    // FlowMonitor counts retransmitted TCP packets
    // as additional transmissions at IP level.

    double tcpOriginalPackets =
        tcpRxPackets + tcpLostPackets;

    double tcpRetransmissions =
        tcpTxPackets - tcpOriginalPackets;

    if (tcpRetransmissions < 0)
    {
        tcpRetransmissions = 0;
    }

    double tcpRetransmissionRate = 0.0;

    if (tcpOriginalPackets > 0)
    {
        tcpRetransmissionRate =
            (tcpRetransmissions /
             tcpOriginalPackets) * 100.0;
    }


    // ---------------- UDP ----------------

    double udpThroughput =
        (udpBytesReceived * 8.0) /
        measurementTime /
        1000000.0;


    double udpPacketLoss = 0.0;

    if (udpTxPackets > 0)
    {
        udpPacketLoss =
            (udpLostPackets / udpTxPackets) * 100.0;
    }


    double udpAverageDelay = 0.0;

    if (udpRxPackets > 0)
    {
        udpAverageDelay =
            (udpDelaySum / udpRxPackets) * 1000.0;
    }


    // ============================================================
    // 15. PRINT RESULTS
    // ============================================================

    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "       TCP AND UDP PERFORMANCE ANALYSIS\n";
    std::cout << "====================================================\n";

    std::cout << std::fixed
              << std::setprecision(3);

    std::cout << "Bottleneck Bandwidth : "
              << bottleneckRate
              << " Mbps\n";

    std::cout << "Bottleneck Delay     : "
              << bottleneckDelay
              << " ms\n";

    std::cout << "UDP Data Rate        : "
              << udpRate
              << " Mbps\n";

    std::cout << "Queue Size           : "
              << queueSize
              << " packets\n";

    std::cout << "Simulation Time      : "
              << simulationTime
              << " seconds\n";


    // ------------------------------------------------------------
    // TCP RESULTS
    // ------------------------------------------------------------

    std::cout << "\n---------------- TCP ----------------\n";

    std::cout << "Bytes Received       : "
              << tcpBytesReceived
              << "\n";

    std::cout << "TX Packets           : "
              << tcpTxPackets
              << "\n";

    std::cout << "RX Packets           : "
              << tcpRxPackets
              << "\n";

    std::cout << "Lost Packets         : "
              << tcpLostPackets
              << "\n";

    std::cout << "Packet Loss          : "
              << tcpPacketLoss
              << " %\n";

    std::cout << "Throughput           : "
              << tcpThroughput
              << " Mbps\n";

    std::cout << "Average Delay        : "
              << tcpAverageDelay
              << " ms\n";

    std::cout << "TCP Retransmissions  : "
              << tcpRetransmissions
              << "\n";

    std::cout << "Retransmission Rate  : "
              << tcpRetransmissionRate
              << " %\n";


    // ------------------------------------------------------------
    // UDP RESULTS
    // ------------------------------------------------------------

    std::cout << "\n---------------- UDP ----------------\n";

    std::cout << "Bytes Received       : "
              << udpBytesReceived
              << "\n";

    std::cout << "TX Packets           : "
              << udpTxPackets
              << "\n";

    std::cout << "RX Packets           : "
              << udpRxPackets
              << "\n";

    std::cout << "Lost Packets         : "
              << udpLostPackets
              << "\n";

    std::cout << "Packet Loss          : "
              << udpPacketLoss
              << " %\n";

    std::cout << "Throughput           : "
              << udpThroughput
              << " Mbps\n";

    std::cout << "Average Delay        : "
              << udpAverageDelay
              << " ms\n";


    std::cout << "====================================================\n";


    // ============================================================
    // 16. CLEANUP
    // ============================================================

    Simulator::Destroy();

    return 0;
}
