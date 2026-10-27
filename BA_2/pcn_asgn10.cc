#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/aodv-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/rip-helper.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <map>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("Assign10");

// ============================================================
// SIMULATION PARAMETERS
// ============================================================

static const double SIMULATION_TIME = 20.0;
static const uint32_t PACKET_SIZE = 512;

// Number of intentional topology-change events.
// For LINK_FAILURE this will become 1.
static uint32_t g_routingChanges = 0;


// ============================================================
// PRINT ROUTING TABLES
// ============================================================

void
PrintRoutingTables(
    string protocol,
    string condition)
{
    string filename =
        "routing-" +
        protocol +
        "-" +
        condition +
        ".txt";

    Ptr<OutputStreamWrapper> routingStream =
        Create<OutputStreamWrapper>(
            filename,
            std::ios::out);

    Ipv4RoutingHelper::PrintRoutingTableAllAt(
        Seconds(19.0),
        routingStream);
}


// ============================================================
// LINK FAILURE
// ============================================================

void
DisableLink(
    Ptr<Ipv4> ipv4Router1,
    uint32_t interface1,
    Ptr<Ipv4> ipv4Router2,
    uint32_t interface2)
{
    cout << endl;

    cout << "************************************************"
         << endl;

    cout << "LINK FAILURE OCCURRED AT "
         << Simulator::Now().GetSeconds()
         << " seconds"
         << endl;

    cout << "Disabling R1 <----> R2 link"
         << endl;

    cout << "************************************************"
         << endl;


    // --------------------------------------------------------
    // Bring Router 1 interface DOWN
    // --------------------------------------------------------

    ipv4Router1->SetDown(
        interface1);


    // --------------------------------------------------------
    // Bring Router 2 interface DOWN
    // --------------------------------------------------------

    ipv4Router2->SetDown(
        interface2);


    // --------------------------------------------------------
    // Record topology change
    // --------------------------------------------------------

    g_routingChanges++;
}


// ============================================================
// INSTALL ROUTING PROTOCOL
// ============================================================

void
InstallRoutingProtocol(
    string protocol,
    NodeContainer routers,
    NodeContainer sources,
    NodeContainer destinations)
{
    cout << endl;

    cout << "Installing routing protocol: "
         << protocol
         << endl;


    // ========================================================
    // RIP
    // ========================================================

    if (protocol == "RIP")
    {
        RipHelper rip;

        InternetStackHelper internet;

        internet.SetRoutingHelper(
            rip);

        internet.Install(
            routers);

        internet.Install(
            sources);

        internet.Install(
            destinations);

        cout << "RIP installed."
             << endl;
    }


    // ========================================================
    // AODV
    // ========================================================

    else if (protocol == "AODV")
    {
        AodvHelper aodv;

        InternetStackHelper internet;

        internet.SetRoutingHelper(
            aodv);

        internet.Install(
            routers);

        internet.Install(
            sources);

        internet.Install(
            destinations);

        cout << "AODV installed."
             << endl;
    }


    // ========================================================
    // OSPF
    // ========================================================
    //
    // NS-3.42 does not provide a standalone OspfHelper.
    //
    // Global Routing uses a link-state / OSPF-style shortest
    // path calculation based on Link State Advertisements.
    //
    // ========================================================

    else if (protocol == "OSPF")
    {
        Config::SetDefault(
            "ns3::Ipv4GlobalRouting::RespondToInterfaceEvents",
            BooleanValue(true));

        Ipv4GlobalRoutingHelper globalRouting;

        InternetStackHelper internet;

        internet.SetRoutingHelper(
            globalRouting);

        internet.Install(
            routers);

        internet.Install(
            sources);

        internet.Install(
            destinations);

        cout << "OSPF-style Global Routing installed."
             << endl;
    }
}


// ============================================================
// INSTALL ONE UDP CBR FLOW
// ============================================================

ApplicationContainer
InstallUdpFlow(
    Ptr<Node> source,
    Ptr<Node> destination,
    Ipv4Address destinationAddress,
    uint16_t port,
    string dataRate,
    double startTime,
    double stopTime)
{
    // --------------------------------------------------------
    // PACKET SINK
    // --------------------------------------------------------

    Address sinkAddress(
        InetSocketAddress(
            destinationAddress,
            port));


    PacketSinkHelper sinkHelper(
        "ns3::UdpSocketFactory",
        sinkAddress);


    ApplicationContainer sinkApplication =
        sinkHelper.Install(
            destination);


    sinkApplication.Start(
        Seconds(0.0));

    sinkApplication.Stop(
        Seconds(stopTime));


    // --------------------------------------------------------
    // UDP CBR SOURCE
    // --------------------------------------------------------

    OnOffHelper sourceHelper(
        "ns3::UdpSocketFactory",
        sinkAddress);


    sourceHelper.SetAttribute(
        "DataRate",
        DataRateValue(
            DataRate(dataRate)));


    sourceHelper.SetAttribute(
        "PacketSize",
        UintegerValue(
            PACKET_SIZE));


    sourceHelper.SetAttribute(
        "OnTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1]"));


    sourceHelper.SetAttribute(
        "OffTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=0]"));


    ApplicationContainer sourceApplication =
        sourceHelper.Install(
            source);


    sourceApplication.Start(
        Seconds(startTime));

    sourceApplication.Stop(
        Seconds(stopTime));


    return sourceApplication;
}


// ============================================================
// RUN ONE EXPERIMENT
// ============================================================

void
RunExperiment(
    string protocol,
    string condition)
{
    cout << endl;

    cout << "===================================================="
         << endl;

    cout << " PROTOCOL  : "
         << protocol
         << endl;

    cout << " CONDITION : "
         << condition
         << endl;

    cout << "===================================================="
         << endl;


    // --------------------------------------------------------
    // Reset routing-change counter
    // --------------------------------------------------------

    g_routingChanges = 0;


    // ========================================================
    // CREATE ROUTERS
    // ========================================================

    NodeContainer routers;

    routers.Create(4);


    // ========================================================
    // CREATE SOURCE NODES
    // ========================================================

    NodeContainer sources;

    sources.Create(3);


    // ========================================================
    // CREATE DESTINATION NODES
    // ========================================================

    NodeContainer destinations;

    destinations.Create(3);


    // ========================================================
    // INSTALL ROUTING PROTOCOL
    // ========================================================

    InstallRoutingProtocol(
        protocol,
        routers,
        sources,
        destinations);


    // ========================================================
    // ROUTER-ROUTER LINK
    // ========================================================

    PointToPointHelper routerLink;

    routerLink.SetDeviceAttribute(
        "DataRate",
        StringValue("5Mbps"));

    routerLink.SetChannelAttribute(
        "Delay",
        StringValue("10ms"));


    // ========================================================
    // ACCESS LINK
    // ========================================================

    PointToPointHelper accessLink;

    accessLink.SetDeviceAttribute(
        "DataRate",
        StringValue("10Mbps"));

    accessLink.SetChannelAttribute(
        "Delay",
        StringValue("5ms"));


    // ========================================================
    // IP ADDRESS HELPER
    // ========================================================

    Ipv4AddressHelper address;


    // ========================================================
    // ROUTER TO ROUTER TOPOLOGY
    // ========================================================
    //
    //                 R0
    //                /  \
    //               /    \
    //             R1------R2
    //              \      /
    //               \    /
    //                 R3
    //
    // Links:
    //
    // R0 <--> R1
    // R1 <--> R2
    // R2 <--> R0
    // R0 <--> R3
    // R3 <--> R2
    //
    // ========================================================


    // --------------------------------------------------------
    // R0 <----> R1
    // --------------------------------------------------------

    NetDeviceContainer r0r1 =
        routerLink.Install(
            routers.Get(0),
            routers.Get(1));


    address.SetBase(
        Ipv4Address("10.0.1.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer r0r1Interfaces =
        address.Assign(
            r0r1);


    // --------------------------------------------------------
    // R1 <----> R2
    // --------------------------------------------------------

    NetDeviceContainer r1r2 =
        routerLink.Install(
            routers.Get(1),
            routers.Get(2));


    address.SetBase(
        Ipv4Address("10.0.2.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer r1r2Interfaces =
        address.Assign(
            r1r2);


    // --------------------------------------------------------
    // R2 <----> R0
    // --------------------------------------------------------

    NetDeviceContainer r2r0 =
        routerLink.Install(
            routers.Get(2),
            routers.Get(0));


    address.SetBase(
        Ipv4Address("10.0.3.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer r2r0Interfaces =
        address.Assign(
            r2r0);


    // --------------------------------------------------------
    // R0 <----> R3
    // --------------------------------------------------------

    NetDeviceContainer r0r3 =
        routerLink.Install(
            routers.Get(0),
            routers.Get(3));


    address.SetBase(
        Ipv4Address("10.0.4.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer r0r3Interfaces =
        address.Assign(
            r0r3);


    // --------------------------------------------------------
    // R3 <----> R2
    // --------------------------------------------------------

    NetDeviceContainer r3r2 =
        routerLink.Install(
            routers.Get(3),
            routers.Get(2));


    address.SetBase(
        Ipv4Address("10.0.5.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer r3r2Interfaces =
        address.Assign(
            r3r2);


    // ========================================================
    // SOURCE 0 ---- R0
    // ========================================================

    NetDeviceContainer s0r0 =
        accessLink.Install(
            sources.Get(0),
            routers.Get(0));


    address.SetBase(
        Ipv4Address("10.1.1.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer s0r0Interfaces =
        address.Assign(
            s0r0);


    // ========================================================
    // SOURCE 1 ---- R1
    // ========================================================

    NetDeviceContainer s1r1 =
        accessLink.Install(
            sources.Get(1),
            routers.Get(1));


    address.SetBase(
        Ipv4Address("10.1.2.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer s1r1Interfaces =
        address.Assign(
            s1r1);


    // ========================================================
    // SOURCE 2 ---- R3
    // ========================================================

    NetDeviceContainer s2r3 =
        accessLink.Install(
            sources.Get(2),
            routers.Get(3));


    address.SetBase(
        Ipv4Address("10.1.3.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer s2r3Interfaces =
        address.Assign(
            s2r3);


    // ========================================================
    // R0 ---- DESTINATION 0
    // ========================================================

    NetDeviceContainer r0d0 =
        accessLink.Install(
            routers.Get(0),
            destinations.Get(0));


    address.SetBase(
        Ipv4Address("10.2.1.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer d0Interface =
        address.Assign(
            r0d0);


    // ========================================================
    // R2 ---- DESTINATION 1
    // ========================================================

    NetDeviceContainer r2d1 =
        accessLink.Install(
            routers.Get(2),
            destinations.Get(1));


    address.SetBase(
        Ipv4Address("10.2.2.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer d1Interface =
        address.Assign(
            r2d1);


    // ========================================================
    // R3 ---- DESTINATION 2
    // ========================================================

    NetDeviceContainer r3d2 =
        accessLink.Install(
            routers.Get(3),
            destinations.Get(2));


    address.SetBase(
        Ipv4Address("10.2.3.0"),
        Ipv4Mask("255.255.255.0"));


    Ipv4InterfaceContainer d2Interface =
        address.Assign(
            r3d2);


    // ========================================================
    // INITIAL ROUTING TABLES FOR OSPF-STYLE GLOBAL ROUTING
    // ========================================================

    if (protocol == "OSPF")
    {
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

        cout << "Initial OSPF-style routing tables populated."
             << endl;
    }


    // ========================================================
    // NORMAL CONDITION
    // ========================================================

    if (condition == "NORMAL")
    {
        cout << "Generating normal CBR traffic..."
             << endl;


        // ----------------------------------------------------
        // FLOW 1
        //
        // S0 -> D1
        // ----------------------------------------------------

        InstallUdpFlow(
            sources.Get(0),
            destinations.Get(1),
            d1Interface.GetAddress(1),
            5000,
            "256Kbps",
            1.0,
            SIMULATION_TIME);


        // ----------------------------------------------------
        // FLOW 2
        //
        // S1 -> D2
        // ----------------------------------------------------

        InstallUdpFlow(
            sources.Get(1),
            destinations.Get(2),
            d2Interface.GetAddress(1),
            5001,
            "256Kbps",
            2.0,
            SIMULATION_TIME);
    }


    // ========================================================
    // CONGESTION CONDITION
    // ========================================================

    else if (condition == "CONGESTION")
    {
        cout << "Generating additional simultaneous flows..."
             << endl;


        // ----------------------------------------------------
        // FLOW 1
        //
        // S0 -> D1
        // ----------------------------------------------------

        InstallUdpFlow(
            sources.Get(0),
            destinations.Get(1),
            d1Interface.GetAddress(1),
            5000,
            "1Mbps",
            1.0,
            SIMULATION_TIME);


        // ----------------------------------------------------
        // FLOW 2
        //
        // S1 -> D2
        // ----------------------------------------------------

        InstallUdpFlow(
            sources.Get(1),
            destinations.Get(2),
            d2Interface.GetAddress(1),
            5001,
            "1Mbps",
            1.0,
            SIMULATION_TIME);


        // ----------------------------------------------------
        // FLOW 3
        //
        // S2 -> D0
        // ----------------------------------------------------

        InstallUdpFlow(
            sources.Get(2),
            destinations.Get(0),
            d0Interface.GetAddress(1),
            5002,
            "1Mbps",
            1.0,
            SIMULATION_TIME);


        // ----------------------------------------------------
        // FLOW 4
        //
        // S0 -> D2
        // ----------------------------------------------------

        InstallUdpFlow(
            sources.Get(0),
            destinations.Get(2),
            d2Interface.GetAddress(1),
            5003,
            "1Mbps",
            1.0,
            SIMULATION_TIME);


        // ----------------------------------------------------
        // FLOW 5
        //
        // S1 -> D0
        // ----------------------------------------------------

        InstallUdpFlow(
            sources.Get(1),
            destinations.Get(0),
            d0Interface.GetAddress(1),
            5004,
            "1Mbps",
            1.0,
            SIMULATION_TIME);
    }


    // ========================================================
    // LINK FAILURE CONDITION
    // ========================================================

    else if (condition == "LINK_FAILURE")
    {
        cout << "Generating traffic before link failure..."
             << endl;


        // ----------------------------------------------------
        // FLOW 1
        //
        // S0 -> D1
        // ----------------------------------------------------

        InstallUdpFlow(
            sources.Get(0),
            destinations.Get(1),
            d1Interface.GetAddress(1),
            5000,
            "512Kbps",
            1.0,
            SIMULATION_TIME);


        // ----------------------------------------------------
        // FLOW 2
        //
        // S1 -> D2
        // ----------------------------------------------------

        InstallUdpFlow(
            sources.Get(1),
            destinations.Get(2),
            d2Interface.GetAddress(1),
            5001,
            "512Kbps",
            1.0,
            SIMULATION_TIME);


        // ----------------------------------------------------
        // DISABLE R1 <--> R2 AT 8 SECONDS
        // ----------------------------------------------------
        //
        // IMPORTANT:
        //
        // Ipv4InterfaceContainer::Get() returns:
        //
        // pair<Ptr<Ipv4>, uint32_t>
        //
        // Therefore:
        //
        // .first  = Ptr<Ipv4>
        // .second = interface index
        //
        // ----------------------------------------------------

        Simulator::Schedule(
            Seconds(8.0),
            &DisableLink,

            r1r2Interfaces.Get(0).first,
            r1r2Interfaces.Get(0).second,

            r1r2Interfaces.Get(1).first,
            r1r2Interfaces.Get(1).second);


        // ----------------------------------------------------
        // OSPF/GLOBAL ROUTING RECOMPUTATION
        // ----------------------------------------------------

        if (protocol == "OSPF")
        {
            Simulator::Schedule(
                Seconds(8.1),
                &Ipv4GlobalRoutingHelper::RecomputeRoutingTables);

            cout << "OSPF-style route recomputation scheduled "
                 << "for 8.1 seconds."
                 << endl;
        }
    }


    // ========================================================
    // FLOW MONITOR
    // ========================================================

    FlowMonitorHelper flowmonHelper;

    Ptr<FlowMonitor> monitor =
        flowmonHelper.InstallAll();


    // ========================================================
    // ROUTING TABLE OUTPUT
    // ========================================================

    PrintRoutingTables(
        protocol,
        condition);


    // ========================================================
    // RUN SIMULATION
    // ========================================================

    Simulator::Stop(
        Seconds(SIMULATION_TIME));

    Simulator::Run();


    // ========================================================
    // CHECK LOST PACKETS
    // ========================================================

    monitor->CheckForLostPackets();


    // ========================================================
    // FLOW CLASSIFIER
    // ========================================================

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowmonHelper.GetClassifier());


    // ========================================================
    // FLOW STATISTICS
    // ========================================================

    map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();


    // ========================================================
    // TOTAL METRICS
    // ========================================================

    uint64_t totalTx = 0;

    uint64_t totalRx = 0;

    uint64_t totalLost = 0;

    double totalDelaySeconds = 0.0;


    // ========================================================
    // PRINT EACH FLOW
    // ========================================================

    for (auto const &flow : stats)
    {
        FlowId flowId =
            flow.first;


        FlowMonitor::FlowStats flowStats =
            flow.second;


        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(
                flowId);


        uint64_t txPackets =
            flowStats.txPackets;


        uint64_t rxPackets =
            flowStats.rxPackets;


        uint64_t lostPackets =
            flowStats.lostPackets;


        // ----------------------------------------------------
        // PDR
        // ----------------------------------------------------

        double pdr = 0.0;

        if (txPackets > 0)
        {
            pdr =
                ((double)rxPackets /
                 (double)txPackets) *
                100.0;
        }


        // ----------------------------------------------------
        // DELAY
        // ----------------------------------------------------

        double delayMs = 0.0;

        if (rxPackets > 0)
        {
            delayMs =
                (flowStats.delaySum.GetSeconds() /
                 (double)rxPackets) *
                1000.0;
        }


        // ----------------------------------------------------
        // DISPLAY FLOW
        // ----------------------------------------------------

        cout << endl;

        cout << "--------------------------------------------"
             << endl;

        cout << "Flow ID       : "
             << flowId
             << endl;

        cout << "Source        : "
             << tuple.sourceAddress
             << endl;

        cout << "Destination   : "
             << tuple.destinationAddress
             << endl;

        cout << "TX Packets    : "
             << txPackets
             << endl;

        cout << "RX Packets    : "
             << rxPackets
             << endl;

        cout << "Lost Packets  : "
             << lostPackets
             << endl;

        cout << "PDR           : "
             << fixed
             << setprecision(2)
             << pdr
             << " %"
             << endl;

        cout << "Delay         : "
             << delayMs
             << " ms"
             << endl;


        // ----------------------------------------------------
        // ADD TO TOTALS
        // ----------------------------------------------------

        totalTx +=
            txPackets;

        totalRx +=
            rxPackets;

        totalLost +=
            lostPackets;

        totalDelaySeconds +=
            flowStats.delaySum.GetSeconds();
    }


    // ========================================================
    // OVERALL PDR
    // ========================================================

    double overallPdr = 0.0;

    if (totalTx > 0)
    {
        overallPdr =
            ((double)totalRx /
             (double)totalTx) *
            100.0;
    }


    // ========================================================
    // OVERALL END-TO-END DELAY
    // ========================================================

    double averageDelayMs = 0.0;

    if (totalRx > 0)
    {
        averageDelayMs =
            (totalDelaySeconds /
             (double)totalRx) *
            1000.0;
    }


    // ========================================================
    // FINAL RESULTS
    // ========================================================

    cout << endl;

    cout << "================================================"
         << endl;

    cout << " FINAL RESULTS"
         << endl;

    cout << "================================================"
         << endl;

    cout << "Protocol                  : "
         << protocol
         << endl;

    cout << "Condition                 : "
         << condition
         << endl;

    cout << "Total TX Packets          : "
         << totalTx
         << endl;

    cout << "Total RX Packets          : "
         << totalRx
         << endl;

    cout << "Total Lost Packets        : "
         << totalLost
         << endl;

    cout << "Packet Delivery Ratio     : "
         << fixed
         << setprecision(2)
         << overallPdr
         << " %"
         << endl;

    cout << "Average End-to-End Delay  : "
         << averageDelayMs
         << " ms"
         << endl;

    cout << "Routing Change Events     : "
         << g_routingChanges
         << endl;

    cout << "================================================"
         << endl;


    // ========================================================
    // SAVE CSV
    // ========================================================

    ofstream csv;

    csv.open(
        "assign10-results.csv",
        ios::app);


    csv
        << protocol
        << ","
        << condition
        << ","
        << fixed
        << setprecision(4)
        << overallPdr
        << ","
        << averageDelayMs
        << ","
        << g_routingChanges
        << ","
        << totalLost
        << endl;


    csv.close();


    // ========================================================
    // SAVE FLOWMONITOR XML
    // ========================================================

    string xmlFilename =
        "assign10-" +
        protocol +
        "-" +
        condition +
        ".xml";


    monitor->SerializeToXmlFile(
        xmlFilename,
        true,
        true);


    // ========================================================
    // DESTROY SIMULATION
    // ========================================================

    Simulator::Destroy();
}


// ============================================================
// MAIN
// ============================================================

int
main()
{
    cout << endl;

    cout << "================================================"
         << endl;

    cout << "              ASSIGNMENT 10"
         << endl;

    cout << "   COMPARATIVE EVALUATION OF ROUTING PROTOCOLS"
         << endl;

    cout << "================================================"
         << endl;


    // ========================================================
    // CREATE CSV HEADER
    // ========================================================

    ofstream csv;

    csv.open(
        "assign10-results.csv");


    csv
        << "Protocol,"
        << "Condition,"
        << "PDR,"
        << "EndToEndDelayMs,"
        << "RoutingChanges,"
        << "LostPackets"
        << endl;


    csv.close();


    // ========================================================
    // RIP
    // ========================================================

    RunExperiment(
        "RIP",
        "NORMAL");

    RunExperiment(
        "RIP",
        "CONGESTION");

    RunExperiment(
        "RIP",
        "LINK_FAILURE");


    // ========================================================
    // AODV
    // ========================================================

    RunExperiment(
        "AODV",
        "NORMAL");

    RunExperiment(
        "AODV",
        "CONGESTION");

    RunExperiment(
        "AODV",
        "LINK_FAILURE");


    // ========================================================
    // OSPF
    // ========================================================

    RunExperiment(
        "OSPF",
        "NORMAL");

    RunExperiment(
        "OSPF",
        "CONGESTION");

    RunExperiment(
        "OSPF",
        "LINK_FAILURE");


    // ========================================================
    // COMPLETED
    // ========================================================

    cout << endl;

    cout << "================================================"
         << endl;

    cout << "       ALL EXPERIMENTS COMPLETED"
         << endl;

    cout << "================================================"
         << endl;

    cout << endl;

    cout << "Main CSV file:"
         << endl;

    cout << "assign10-results.csv"
         << endl;

    cout << endl;

    cout << "FlowMonitor XML files:"
         << endl;

    cout << "assign10-<Protocol>-<Condition>.xml"
         << endl;

    cout << endl;

    cout << "Routing table files:"
         << endl;

    cout << "routing-<Protocol>-<Condition>.txt"
         << endl;


    return 0;
}
