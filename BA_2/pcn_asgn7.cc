#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UdpAssignment");

int main(int argc, char *argv[])
{
    // ------------------------------------------------------------
    // Simulation parameters
    // ------------------------------------------------------------

    double latency = 2.0;       // Link latency in milliseconds
    double rate1 = 5.0;         // Flow 1 data rate in Mbps
    double rate2 = 5.0;         // Flow 2 data rate in Mbps
    uint32_t packetSize = 1024; // UDP packet size in bytes
    double simulationTime = 10.0;

    CommandLine cmd(__FILE__);

    cmd.AddValue("latency",
                 "Point-to-point link latency in milliseconds",
                 latency);

    cmd.AddValue("rate1",
                 "Data rate of UDP Flow 1",
                 rate1);

    cmd.AddValue("rate2",
                 "Data rate of UDP Flow 2",
                 rate2);

    cmd.Parse(argc, argv);

    // ------------------------------------------------------------
    // 1. Create two nodes
    // ------------------------------------------------------------

    NodeContainer nodes;
    nodes.Create(2);

    // ------------------------------------------------------------
    // 2. Create Point-to-Point link
    // ------------------------------------------------------------

    PointToPointHelper pointToPoint;

    pointToPoint.SetDeviceAttribute(
        "DataRate",
        StringValue("20Mbps"));

    pointToPoint.SetChannelAttribute(
        "Delay",
        TimeValue(MilliSeconds(latency)));

    NetDeviceContainer devices =
        pointToPoint.Install(nodes);

    // ------------------------------------------------------------
    // 3. Install Internet stack
    // ------------------------------------------------------------

    InternetStackHelper internet;
    internet.Install(nodes);

    // ------------------------------------------------------------
    // 4. Assign IP addresses
    // ------------------------------------------------------------

    Ipv4AddressHelper address;

    address.SetBase(
        "10.1.1.0",
        "255.255.255.0");

    Ipv4InterfaceContainer interfaces =
        address.Assign(devices);

    // Node 0 = 10.1.1.1
    // Node 1 = 10.1.1.2

    // ------------------------------------------------------------
    // 5. UDP Server 1 - Port 5000
    // ------------------------------------------------------------

    uint16_t port1 = 5000;

    UdpServerHelper server1(port1);

    ApplicationContainer serverApp1 =
        server1.Install(nodes.Get(1));

    serverApp1.Start(Seconds(1.0));
    serverApp1.Stop(Seconds(simulationTime));

    // ------------------------------------------------------------
    // 6. UDP Client 1 - Port 5000
    // ------------------------------------------------------------

    UdpClientHelper client1(
        interfaces.GetAddress(1),
        port1);

    client1.SetAttribute(
        "MaxPackets",
        UintegerValue(1000000));

    client1.SetAttribute(
        "Interval",
        TimeValue(
            Seconds(
                (packetSize * 8.0) /
                (rate1 * 1000000.0))));

    client1.SetAttribute(
        "PacketSize",
        UintegerValue(packetSize));

    ApplicationContainer clientApp1 =
        client1.Install(nodes.Get(0));

    clientApp1.Start(Seconds(1.0));
    clientApp1.Stop(Seconds(simulationTime));

    // ------------------------------------------------------------
    // 7. UDP Server 2 - Port 6000
    // ------------------------------------------------------------

    uint16_t port2 = 6000;

    UdpServerHelper server2(port2);

    ApplicationContainer serverApp2 =
        server2.Install(nodes.Get(1));

    serverApp2.Start(Seconds(1.0));
    serverApp2.Stop(Seconds(simulationTime));

    // ------------------------------------------------------------
    // 8. UDP Client 2 - Port 6000
    // ------------------------------------------------------------

    UdpClientHelper client2(
        interfaces.GetAddress(1),
        port2);

    client2.SetAttribute(
        "MaxPackets",
        UintegerValue(1000000));

    client2.SetAttribute(
        "Interval",
        TimeValue(
            Seconds(
                (packetSize * 8.0) /
                (rate2 * 1000000.0))));

    client2.SetAttribute(
        "PacketSize",
        UintegerValue(packetSize));

    ApplicationContainer clientApp2 =
        client2.Install(nodes.Get(0));

    clientApp2.Start(Seconds(1.0));
    clientApp2.Stop(Seconds(simulationTime));

    // ------------------------------------------------------------
    // 9. Enable PCAP tracing
    // ------------------------------------------------------------

    pointToPoint.EnablePcapAll(
        "udp-assignment",
        true);

    // ------------------------------------------------------------
    // 10. Run simulation
    // ------------------------------------------------------------

    Simulator::Stop(
        Seconds(simulationTime));

    Simulator::Run();

    // ------------------------------------------------------------
    // 11. Calculate throughput
    // ------------------------------------------------------------

    Ptr<UdpServer> serverPtr1 =
        DynamicCast<UdpServer>(
            serverApp1.Get(0));

    Ptr<UdpServer> serverPtr2 =
        DynamicCast<UdpServer>(
            serverApp2.Get(0));

    uint64_t received1 =
        serverPtr1->GetReceived();

    uint64_t received2 =
        serverPtr2->GetReceived();

    double throughput1 =
        (received1 * packetSize * 8.0) /
        (simulationTime - 1.0) /
        1000000.0;

    double throughput2 =
        (received2 * packetSize * 8.0) /
        (simulationTime - 1.0) /
        1000000.0;

    // ------------------------------------------------------------
    // 12. Print results
    // ------------------------------------------------------------

    std::cout << "\n========================================\n";
    std::cout << "       UDP ASSIGNMENT RESULTS\n";
    std::cout << "========================================\n";

    std::cout << "Link Latency : "
              << latency
              << " ms\n";

    std::cout << "Flow 1 Rate  : "
              << rate1
              << " Mbps\n";

    std::cout << "Flow 2 Rate  : "
              << rate2
              << " Mbps\n";

    std::cout << "\nFlow 1 - Port 5000\n";
    std::cout << "Packets Received : "
              << received1
              << "\n";

    std::cout << "Throughput       : "
              << throughput1
              << " Mbps\n";

    std::cout << "\nFlow 2 - Port 6000\n";
    std::cout << "Packets Received : "
              << received2
              << "\n";

    std::cout << "Throughput       : "
              << throughput2
              << " Mbps\n";

    std::cout << "========================================\n";

    Simulator::Destroy();

    return 0;
}
