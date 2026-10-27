#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpCongestionAssignment");

// ============================================================
// GLOBAL CWND DATA
// ============================================================

std::vector<double> cwndValues;
std::ofstream cwndFile;


// ============================================================
// CWND CALLBACK
// ============================================================

void
CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    double time = Simulator::Now().GetSeconds();

    cwndValues.push_back(static_cast<double>(newCwnd));

    if (cwndFile.is_open())
    {
        cwndFile << time
                 << ","
                 << newCwnd
                 << std::endl;
    }
}


// ============================================================
// CONNECT CWND TRACE
// ============================================================

void
ConnectCwndTrace(uint32_t nodeId)
{
    std::string path =
        "/NodeList/" +
        std::to_string(nodeId) +
        "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";

    Config::ConnectWithoutContext(
        path,
        MakeCallback(&CwndChange));
}


// ============================================================
// MAIN
// ============================================================

int
main(int argc, char *argv[])
{
    // ========================================================
    // EXPERIMENT PARAMETERS
    // ========================================================

    double bottleneckRate = 5.0;       // Mbps
    double bottleneckDelay = 20.0;     // ms
    uint32_t queueSize = 50;           // packets
    double simulationTime = 20.0;      // seconds

    uint32_t packetSize = 1024;        // bytes

    std::string tcpVariant = "TcpReno";


    // ========================================================
    // COMMAND LINE PARAMETERS
    // ========================================================

    CommandLine cmd(__FILE__);

    cmd.AddValue(
        "tcpVariant",
        "TCP congestion control variant",
        tcpVariant);

    cmd.AddValue(
        "bottleneckRate",
        "Bottleneck bandwidth in Mbps",
        bottleneckRate);

    cmd.AddValue(
        "bottleneckDelay",
        "Bottleneck delay in milliseconds",
        bottleneckDelay);

    cmd.AddValue(
        "queueSize",
        "DropTail queue size in packets",
        queueSize);

    cmd.Parse(argc, argv);


    // ========================================================
    // 1. SELECT TCP CONGESTION CONTROL ALGORITHM
    // ========================================================

    std::string tcpType;

    if (tcpVariant == "TcpReno")
    {
        /*
         * TcpLinuxReno is ns-3's Linux-style Reno implementation.
         */
        tcpType = "ns3::TcpLinuxReno";
    }
    else if (tcpVariant == "TcpCubic")
    {
        tcpType = "ns3::TcpCubic";
    }
    else if (tcpVariant == "TcpBbr")
    {
        tcpType = "ns3::TcpBbr";
    }
    else
    {
        std::cerr << "\nUnknown TCP variant: "
                  << tcpVariant
                  << "\n";

        std::cerr << "Use TcpReno, TcpCubic, or TcpBbr.\n";

        return 1;
    }


    Config::SetDefault(
        "ns3::TcpL4Protocol::SocketType",
        TypeIdValue(
            TypeId::LookupByName(tcpType)));


    // ========================================================
    // 2. CREATE NODES
    // ========================================================

    NodeContainer vehicles;
    vehicles.Create(5);

    NodeContainer router;
    router.Create(1);

    NodeContainer server;
    server.Create(1);


    // ========================================================
    // 3. INSTALL INTERNET STACK
    // ========================================================

    InternetStackHelper internet;

    internet.Install(vehicles);
    internet.Install(router);
    internet.Install(server);


    // ========================================================
    // 4. VEHICLE -> ROUTER ACCESS LINKS
    // ========================================================

    PointToPointHelper accessLink;

    accessLink.SetDeviceAttribute(
        "DataRate",
        StringValue("10Mbps"));

    accessLink.SetChannelAttribute(
        "Delay",
        StringValue("5ms"));


    NetDeviceContainer v0Router =
        accessLink.Install(
            vehicles.Get(0),
            router.Get(0));

    NetDeviceContainer v1Router =
        accessLink.Install(
            vehicles.Get(1),
            router.Get(0));

    NetDeviceContainer v2Router =
        accessLink.Install(
            vehicles.Get(2),
            router.Get(0));

    NetDeviceContainer v3Router =
        accessLink.Install(
            vehicles.Get(3),
            router.Get(0));

    NetDeviceContainer v4Router =
        accessLink.Install(
            vehicles.Get(4),
            router.Get(0));


    // ========================================================
    // 5. ROUTER -> SERVER BOTTLENECK
    // ========================================================

    PointToPointHelper bottleneck;

    bottleneck.SetDeviceAttribute(
        "DataRate",
        StringValue(
            std::to_string(bottleneckRate) +
            "Mbps"));

    bottleneck.SetChannelAttribute(
        "Delay",
        StringValue(
            std::to_string(bottleneckDelay) +
            "ms"));


    bottleneck.SetQueue(
        "ns3::DropTailQueue<Packet>",
        "MaxSize",
        QueueSizeValue(
            QueueSize(
                std::to_string(queueSize) +
                "p")));


    NetDeviceContainer routerServer =
        bottleneck.Install(
            router.Get(0),
            server.Get(0));


    // ========================================================
    // 6. ASSIGN IP ADDRESSES
    // ========================================================

    Ipv4AddressHelper address;


    // Vehicle 0
    address.SetBase(
        "10.0.1.0",
        "255.255.255.0");

    Ipv4InterfaceContainer i0 =
        address.Assign(v0Router);


    // Vehicle 1
    address.SetBase(
        "10.0.2.0",
        "255.255.255.0");

    Ipv4InterfaceContainer i1 =
        address.Assign(v1Router);


    // Vehicle 2
    address.SetBase(
        "10.0.3.0",
        "255.255.255.0");

    Ipv4InterfaceContainer i2 =
        address.Assign(v2Router);


    // Vehicle 3
    address.SetBase(
        "10.0.4.0",
        "255.255.255.0");

    Ipv4InterfaceContainer i3 =
        address.Assign(v3Router);


    // Vehicle 4
    address.SetBase(
        "10.0.5.0",
        "255.255.255.0");

    Ipv4InterfaceContainer i4 =
        address.Assign(v4Router);


    // Router <-> Server
    address.SetBase(
        "10.0.6.0",
        "255.255.255.0");

    Ipv4InterfaceContainer routerServerInterface =
        address.Assign(routerServer);


    // ========================================================
    // 7. ENABLE ROUTING
    // ========================================================

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();


    // ========================================================
    // 8. CWND OUTPUT FILE
    // ========================================================

    cwndValues.clear();

    std::string cwndFilename =
        "tcp-" +
        tcpVariant +
        "-cwnd.csv";

    cwndFile.open(cwndFilename);

    if (cwndFile.is_open())
    {
        cwndFile << "Time(s),CWND(bytes)"
                 << std::endl;
    }


    // ========================================================
    // 9. TCP SERVER
    // ========================================================

    uint16_t port = 5000;


    PacketSinkHelper sink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(
            Ipv4Address::GetAny(),
            port));


    ApplicationContainer serverApp =
        sink.Install(
            server.Get(0));


    serverApp.Start(
        Seconds(0.0));

    serverApp.Stop(
        Seconds(simulationTime));


    // ========================================================
    // 10. CREATE TCP FLOWS
    // ========================================================

    ApplicationContainer clients;


    for (uint32_t i = 0; i < 5; i++)
    {
        BulkSendHelper source(
            "ns3::TcpSocketFactory",
            InetSocketAddress(
                routerServerInterface.GetAddress(1),
                port));


        source.SetAttribute(
            "MaxBytes",
            UintegerValue(0));


        source.SetAttribute(
            "SendSize",
            UintegerValue(packetSize));


        ApplicationContainer app =
            source.Install(
                vehicles.Get(i));


        double startTime =
            1.0 + i * 0.2;


        app.Start(
            Seconds(startTime));

        app.Stop(
            Seconds(simulationTime));


        clients.Add(app);


        // ====================================================
        // CONNECT CWND TRACE AFTER SOCKET CREATION
        // ====================================================

        Simulator::Schedule(
            Seconds(startTime + 0.001),
            &ConnectCwndTrace,
            vehicles.Get(i)->GetId());
    }


    // ========================================================
    // 11. ENABLE PCAP TRACING
    // ========================================================

    accessLink.EnablePcapAll(
        "tcp-congestion-access",
        true);

    bottleneck.EnablePcapAll(
        "tcp-congestion-bottleneck",
        true);


    // ========================================================
    // 12. FLOW MONITOR
    // ========================================================

    FlowMonitorHelper flowHelper;

    Ptr<FlowMonitor> monitor =
        flowHelper.InstallAll();


    // ========================================================
    // 13. RUN SIMULATION
    // ========================================================

    std::cout << "\n";
    std::cout << "Starting simulation...\n";
    std::cout << "TCP Variant: "
              << tcpVariant
              << "\n";


    Simulator::Stop(
        Seconds(simulationTime));

    Simulator::Run();


    // ========================================================
    // 14. FLOW MONITOR STATISTICS
    // ========================================================

    monitor->CheckForLostPackets();


    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());


    std::map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();


    // ========================================================
    // RESULT VARIABLES
    // ========================================================

    double totalThroughput = 0.0;

    double totalDelay = 0.0;

    uint64_t totalRxPackets = 0;

    uint64_t totalTxPackets = 0;

    uint64_t totalLostPackets = 0;

    std::vector<double> flowThroughputs;


    // ========================================================
    // 15. PROCESS FLOW RESULTS
    // ========================================================

    for (auto const &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(flow.first);


        FlowMonitor::FlowStats st =
            flow.second;


        // Only consider flows going to our TCP server
        if (tuple.destinationPort != port)
        {
            continue;
        }


        // ====================================================
        // THROUGHPUT
        // ====================================================

        double throughput = 0.0;


        if (st.rxBytes > 0)
        {
            double activeTime =
                st.timeLastRxPacket.GetSeconds()
                -
                st.timeFirstTxPacket.GetSeconds();


            if (activeTime > 0)
            {
                throughput =
                    (st.rxBytes * 8.0)
                    /
                    activeTime
                    /
                    1000000.0;
            }
        }


        // ====================================================
        // DELAY
        // ====================================================

        double delay = 0.0;


        if (st.rxPackets > 0)
        {
            delay =
                st.delaySum.GetSeconds()
                /
                st.rxPackets
                *
                1000.0;
        }


        // ====================================================
        // ACCUMULATE RESULTS
        // ====================================================

        totalThroughput +=
            throughput;


        totalDelay +=
            delay * st.rxPackets;


        totalRxPackets +=
            st.rxPackets;


        totalTxPackets +=
            st.txPackets;


        totalLostPackets +=
            st.lostPackets;


        flowThroughputs.push_back(
            throughput);


        // ====================================================
        // DISPLAY INDIVIDUAL FLOW
        // ====================================================

        std::cout << "\n--------------------------------------------\n";

        std::cout << "Flow ID       : "
                  << flow.first
                  << "\n";

        std::cout << "Source        : "
                  << tuple.sourceAddress
                  << "\n";

        std::cout << "Destination   : "
                  << tuple.destinationAddress
                  << "\n";

        std::cout << "Throughput    : "
                  << std::fixed
                  << std::setprecision(3)
                  << throughput
                  << " Mbps\n";

        std::cout << "Delay         : "
                  << delay
                  << " ms\n";

        std::cout << "TX Packets    : "
                  << st.txPackets
                  << "\n";

        std::cout << "RX Packets    : "
                  << st.rxPackets
                  << "\n";

        std::cout << "Lost Packets  : "
                  << st.lostPackets
                  << "\n";
    }


    // ========================================================
    // 16. AVERAGE DELAY
    // ========================================================

    double averageDelay = 0.0;


    if (totalRxPackets > 0)
    {
        averageDelay =
            totalDelay /
            totalRxPackets;
    }


    // ========================================================
    // 17. PACKET LOSS
    // ========================================================

    double packetLoss = 0.0;


    if (totalTxPackets > 0)
    {
        packetLoss =
            static_cast<double>(
                totalLostPackets)
            /
            totalTxPackets
            *
            100.0;
    }


    // ========================================================
    // 18. JAIN'S FAIRNESS INDEX
    // ========================================================

    double fairness = 0.0;


    if (!flowThroughputs.empty())
    {
        double sum = 0.0;

        double sumSquares = 0.0;


        for (double value :
             flowThroughputs)
        {
            sum += value;

            sumSquares +=
                value * value;
        }


        double n =
            flowThroughputs.size();


        if (sumSquares > 0)
        {
            fairness =
                (sum * sum)
                /
                (n * sumSquares);
        }
    }


    // ========================================================
    // 19. CWND STATISTICS
    // ========================================================

    double averageCwnd = 0.0;

    double maximumCwnd = 0.0;


    if (!cwndValues.empty())
    {
        double cwndSum = 0.0;


        for (double value :
             cwndValues)
        {
            cwndSum += value;


            if (value > maximumCwnd)
            {
                maximumCwnd = value;
            }
        }


        averageCwnd =
            cwndSum /
            cwndValues.size();
    }


    // ========================================================
    // 20. FINAL RESULTS
    // ========================================================

    std::cout << "\n\n";

    std::cout
        << "====================================================\n";

    std::cout
        << "        TCP CONGESTION CONTROL ANALYSIS\n";

    std::cout
        << "====================================================\n";


    std::cout
        << std::fixed
        << std::setprecision(3);


    std::cout
        << "TCP Variant        : "
        << tcpVariant
        << "\n";


    std::cout
        << "Bottleneck Rate    : "
        << bottleneckRate
        << " Mbps\n";


    std::cout
        << "Bottleneck Delay   : "
        << bottleneckDelay
        << " ms\n";


    std::cout
        << "Queue Size         : "
        << queueSize
        << " packets\n";


    std::cout
        << "Simulation Time    : "
        << simulationTime
        << " seconds\n";


    std::cout
        << "\n---------------- RESULTS ----------------\n";


    std::cout
        << "Total Throughput   : "
        << totalThroughput
        << " Mbps\n";


    std::cout
        << "Average Delay      : "
        << averageDelay
        << " ms\n";


    std::cout
        << "Packet Loss        : "
        << packetLoss
        << " %\n";


    std::cout
        << "Fairness Index     : "
        << fairness
        << "\n";


    std::cout
        << "Average CWND       : "
        << averageCwnd
        << " bytes\n";


    std::cout
        << "Maximum CWND       : "
        << maximumCwnd
        << " bytes\n";


    std::cout
        << "TX Packets         : "
        << totalTxPackets
        << "\n";


    std::cout
        << "RX Packets         : "
        << totalRxPackets
        << "\n";


    std::cout
        << "Lost Packets       : "
        << totalLostPackets
        << "\n";


    std::cout
        << "====================================================\n";


    // ========================================================
    // 21. SAVE FLOWMONITOR XML
    // ========================================================

    std::string xmlFilename =
        "tcp-" +
        tcpVariant +
        "-results.xml";


    monitor->SerializeToXmlFile(
        xmlFilename,
        true,
        true);


    // ========================================================
    // 22. CLOSE CWND FILE
    // ========================================================

    if (cwndFile.is_open())
    {
        cwndFile.close();
    }


    // ========================================================
    // 23. DESTROY SIMULATION
    // ========================================================

    Simulator::Destroy();


    return 0;
}
