

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <map>
#include <string>
#include <vector>

using namespace ns3;
using namespace std;


// ============================================================
// CONSTANTS
// ============================================================

static const double SIMULATION_TIME = 20.0;
static const double APPLICATION_START = 1.0;
static const double APPLICATION_STOP = 20.0;
static const double MEASUREMENT_TIME = 19.0;

static const uint16_t WIFI_PORT = 5000;
static const uint16_t ZIGBEE_PORT = 6000;
static const uint16_t BLUETOOTH_PORT = 7000;


// ============================================================
// RESULT STRUCTURE
// ============================================================

struct Result
{
    string technology;
    double speed;

    double receivedPower;
    double throughput;
    double packetLoss;
    double delay;
};


// Store all results
vector<Result> allResults;


// ============================================================
// CALCULATE RECEIVED POWER
//
// Friis free-space path-loss equation:
//
// PL(dB) = 20log10(d)
//        + 20log10(f)
//        - 147.55
//
// Received Power = Tx Power - Path Loss
// ============================================================

double
CalculateReceivedPower(
    Ptr<Node> mobile,
    Ptr<Node> gateway,
    double txPowerDbm,
    double frequencyHz)
{
    Ptr<MobilityModel> mobileMobility =
        mobile->GetObject<MobilityModel>();

    Ptr<MobilityModel> gatewayMobility =
        gateway->GetObject<MobilityModel>();

    Vector mobilePosition =
        mobileMobility->GetPosition();

    Vector gatewayPosition =
        gatewayMobility->GetPosition();

    double dx =
        mobilePosition.x - gatewayPosition.x;

    double dy =
        mobilePosition.y - gatewayPosition.y;

    double dz =
        mobilePosition.z - gatewayPosition.z;

    double distance =
        sqrt(
            dx * dx +
            dy * dy +
            dz * dz);

    // Prevent log10(0)
    if (distance < 0.1)
    {
        distance = 0.1;
    }

    double pathLoss =
        20.0 * log10(distance)
        + 20.0 * log10(frequencyHz)
        - 147.55;

    double receivedPower =
        txPowerDbm - pathLoss;

    return receivedPower;
}


// ============================================================
// CALCULATE AVERAGE RECEIVED POWER
// ============================================================

double
CalculateAverageReceivedPower(
    NodeContainer mobileNodes,
    NodeContainer gateway,
    double txPowerDbm,
    double frequencyHz)
{
    double totalPower = 0.0;

    for (uint32_t i = 0;
         i < mobileNodes.GetN();
         i++)
    {
        totalPower +=
            CalculateReceivedPower(
                mobileNodes.Get(i),
                gateway.Get(0),
                txPowerDbm,
                frequencyHz);
    }

    if (mobileNodes.GetN() == 0)
    {
        return 0.0;
    }

    return totalPower /
           static_cast<double>(
               mobileNodes.GetN());
}


// ============================================================
// INSTALL GATEWAY MOBILITY
//
// Gateway is fixed at:
//
// (50, 50, 0)
// ============================================================

void
InstallGatewayMobility(
    NodeContainer gateway)
{
    MobilityHelper mobility;

    Ptr<ListPositionAllocator> positionAllocator =
        CreateObject<ListPositionAllocator>();

    positionAllocator->Add(
        Vector(50.0, 50.0, 0.0));

    mobility.SetPositionAllocator(
        positionAllocator);

    mobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");

    mobility.Install(gateway);
}


// ============================================================
// INSTALL MOBILE NODE MOBILITY
//
// RandomWalk2dMobilityModel
//
// Speeds:
//   1 m/s
//   5 m/s
//   10 m/s
// ============================================================

void
InstallMobileMobility(
    NodeContainer mobileNodes,
    double speed)
{
    MobilityHelper mobility;

    mobility.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",

        "X",
        StringValue(
            "ns3::UniformRandomVariable[Min=20|Max=80]"),

        "Y",
        StringValue(
            "ns3::UniformRandomVariable[Min=20|Max=80]"));

    mobility.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",

        "Mode",
        StringValue("Time"),

        "Time",
        TimeValue(
            Seconds(2.0)),

        "Speed",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1.0]"),

        "Bounds",
        RectangleValue(
            Rectangle(
                0.0,
                100.0,
                0.0,
                100.0)));

    mobility.Install(mobileNodes);

    // Set requested speed

    for (uint32_t i = 0;
         i < mobileNodes.GetN();
         i++)
    {
        Ptr<RandomWalk2dMobilityModel> model =
            mobileNodes.Get(i)
                ->GetObject<RandomWalk2dMobilityModel>();

        model->SetAttribute(
            "Speed",
            StringValue(
                "ns3::ConstantRandomVariable[Constant="
                + to_string(speed)
                + "]"));
    }
}


// ============================================================
// STORE RESULT
// ============================================================

void
StoreResult(
    string technology,
    double speed,
    double receivedPower,
    double throughput,
    double packetLoss,
    double delay)
{
    Result result;

    result.technology = technology;
    result.speed = speed;
    result.receivedPower = receivedPower;
    result.throughput = throughput;
    result.packetLoss = packetLoss;
    result.delay = delay;

    allResults.push_back(result);
}


// ============================================================
// PRINT RESULT
// ============================================================

void
PrintResult(
    string technology,
    double speed,
    double receivedPower,
    double throughput,
    double packetLoss,
    double delay)
{
    cout << endl;

    cout << "----------------------------------------------------"
         << endl;

    cout << "Technology       : "
         << technology
         << endl;

    cout << "Mobility Speed   : "
         << speed
         << " m/s"
         << endl;

    cout << fixed
         << setprecision(3);

    cout << "Received Power   : "
         << receivedPower
         << " dBm"
         << endl;

    cout << "Throughput       : "
         << throughput / 1000.0
         << " Kbps"
         << endl;

    cout << "Packet Loss      : "
         << packetLoss
         << " %"
         << endl;

    cout << "End-to-End Delay : "
         << delay
         << " ms"
         << endl;

    cout << "----------------------------------------------------"
         << endl;
}


// ============================================================
// WI-FI EXPERIMENT
// ============================================================

void
RunWifiExperiment(double speed)
{
    cout << endl;

    cout << "===================================================="
         << endl;

    cout << "              WI-FI EXPERIMENT"
         << endl;

    cout << "              Speed = "
         << speed
         << " m/s"
         << endl;

    cout << "===================================================="
         << endl;


    // --------------------------------------------------------
    // NODES
    // --------------------------------------------------------

    NodeContainer mobile;
    mobile.Create(3);

    NodeContainer gateway;
    gateway.Create(1);


    // --------------------------------------------------------
    // MOBILITY
    // --------------------------------------------------------

    InstallGatewayMobility(gateway);

    InstallMobileMobility(
        mobile,
        speed);


    // --------------------------------------------------------
    // WIFI CHANNEL
    // --------------------------------------------------------

    YansWifiChannelHelper channel =
        YansWifiChannelHelper::Default();

    channel.AddPropagationLoss(
        "ns3::FriisPropagationLossModel");


    YansWifiPhyHelper phy;

    phy.SetChannel(
        channel.Create());

    phy.Set(
        "TxPowerStart",
        DoubleValue(16.0));

    phy.Set(
        "TxPowerEnd",
        DoubleValue(16.0));


    // --------------------------------------------------------
    // WIFI
    // --------------------------------------------------------

    WifiHelper wifi;

    wifi.SetStandard(
        WIFI_STANDARD_80211g);

    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",

        "DataMode",
        StringValue("ErpOfdmRate6Mbps"),

        "ControlMode",
        StringValue("ErpOfdmRate6Mbps"));


    WifiMacHelper mac;

    Ssid ssid =
        Ssid("Campus-WiFi");


    // Mobile nodes = STA

    mac.SetType(
        "ns3::StaWifiMac",

        "Ssid",
        SsidValue(ssid),

        "ActiveProbing",
        BooleanValue(false));


    NetDeviceContainer mobileDevices =
        wifi.Install(
            phy,
            mac,
            mobile);


    // Gateway = AP

    mac.SetType(
        "ns3::ApWifiMac",

        "Ssid",
        SsidValue(ssid));


    NetDeviceContainer gatewayDevice =
        wifi.Install(
            phy,
            mac,
            gateway);


    // --------------------------------------------------------
    // INTERNET
    // --------------------------------------------------------

    InternetStackHelper internet;

    internet.Install(mobile);

    internet.Install(gateway);


    // --------------------------------------------------------
    // IPV4 ADDRESSING
    // --------------------------------------------------------

    Ipv4AddressHelper address;

    address.SetBase(
        "10.1.0.0",
        "255.255.255.0");


    NetDeviceContainer allWifiDevices;
    allWifiDevices.Add(mobileDevices);
    allWifiDevices.Add(gatewayDevice);

    Ipv4InterfaceContainer wifiInterfaces =
        address.Assign(allWifiDevices);

    // Device/interface order: mobile nodes 0-2, gateway 3.


    // --------------------------------------------------------
    // UDP SINK
    // --------------------------------------------------------

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",

        InetSocketAddress(
            Ipv4Address::GetAny(),
            WIFI_PORT));


    ApplicationContainer sinkApp =
        sink.Install(
            gateway.Get(0));

    sinkApp.Start(
        Seconds(APPLICATION_START));

    sinkApp.Stop(
        Seconds(APPLICATION_STOP));


    // --------------------------------------------------------
    // UDP SOURCE
    // --------------------------------------------------------

    OnOffHelper source(
        "ns3::UdpSocketFactory",

        InetSocketAddress(
            wifiInterfaces.GetAddress(3),
            WIFI_PORT));


    source.SetAttribute(
        "DataRate",
        DataRateValue(
            DataRate("1Mbps")));

    source.SetAttribute(
        "PacketSize",
        UintegerValue(512));

    source.SetAttribute(
        "OnTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1]"));

    source.SetAttribute(
        "OffTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=0]"));


    ApplicationContainer sourceApp =
        source.Install(mobile);

    sourceApp.Start(
        Seconds(APPLICATION_START));

    sourceApp.Stop(
        Seconds(APPLICATION_STOP));


    // --------------------------------------------------------
    // FLOW MONITOR
    // --------------------------------------------------------

    FlowMonitorHelper flowHelper;

    Ptr<FlowMonitor> monitor =
        flowHelper.InstallAll();


    Simulator::Stop(
        Seconds(SIMULATION_TIME));

    Simulator::Run();


    // --------------------------------------------------------
    // FLOW STATISTICS
    // --------------------------------------------------------

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());

    map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();


    uint64_t totalTx = 0;
    uint64_t totalRx = 0;

    double totalDelay = 0.0;


    for (const auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(
                flow.first);

        // UDP protocol = 17

        if (tuple.protocol != 17)
        {
            continue;
        }

        totalTx +=
            flow.second.txPackets;

        totalRx +=
            flow.second.rxPackets;

        totalDelay +=
            flow.second.delaySum.GetSeconds();
    }


    // --------------------------------------------------------
    // THROUGHPUT
    // --------------------------------------------------------

    double throughput = 0.0;

    if (totalRx > 0)
    {
        throughput =
            static_cast<double>(totalRx)
            * 512.0
            * 8.0
            / MEASUREMENT_TIME;
    }


    // --------------------------------------------------------
    // PACKET LOSS
    // --------------------------------------------------------

    double packetLoss = 0.0;

    if (totalTx > 0)
    {
        packetLoss =
            (
                static_cast<double>(
                    totalTx - totalRx)
                / static_cast<double>(totalTx)
            )
            * 100.0;
    }


    // --------------------------------------------------------
    // DELAY
    // --------------------------------------------------------

    double delay = 0.0;

    if (totalRx > 0)
    {
        delay =
            (
                totalDelay
                / static_cast<double>(totalRx)
            )
            * 1000.0;
    }


    // --------------------------------------------------------
    // RECEIVED POWER
    // --------------------------------------------------------

    double receivedPower =
        CalculateAverageReceivedPower(
            mobile,
            gateway,
            16.0,
            2.4e9);


    // --------------------------------------------------------
    // PRINT
    // --------------------------------------------------------

    PrintResult(
        "WiFi",
        speed,
        receivedPower,
        throughput,
        packetLoss,
        delay);


    // --------------------------------------------------------
    // STORE
    // --------------------------------------------------------

    StoreResult(
        "WiFi",
        speed,
        receivedPower,
        throughput,
        packetLoss,
        delay);


    Simulator::Destroy();
}


// ============================================================
// ZIGBEE EXPERIMENT
//
// IEEE 802.15.4 LR-WPAN + 6LoWPAN
// ============================================================

void
RunZigbeeExperiment(double speed)
{
    cout << endl;

    cout << "===================================================="
         << endl;

    cout << "              ZIGBEE EXPERIMENT"
         << endl;

    cout << "              Speed = "
         << speed
         << " m/s"
         << endl;

    cout << "===================================================="
         << endl;


    // --------------------------------------------------------
    // NODES
    // --------------------------------------------------------

    NodeContainer mobile;
    mobile.Create(3);

    NodeContainer gateway;
    gateway.Create(1);

    NodeContainer allNodes;

    allNodes.Add(mobile);
    allNodes.Add(gateway);


    // --------------------------------------------------------
    // MOBILITY
    // --------------------------------------------------------

    InstallGatewayMobility(gateway);

    InstallMobileMobility(
        mobile,
        speed);


    // --------------------------------------------------------
    // IEEE 802.15.4 LR-WPAN
    // --------------------------------------------------------

    LrWpanHelper lrWpan;

    NetDeviceContainer lrWpanDevices =
        lrWpan.Install(allNodes);


    // --------------------------------------------------------
    // PAN ID
    //
    // IMPORTANT:
    // NS-3.42 places LrWpanNetDevice in:
    //
    // ns3::lrwpan
    //
    // Therefore:
    //
    // Ptr<lrwpan::LrWpanNetDevice>
    // --------------------------------------------------------

    for (uint32_t i = 0;
         i < lrWpanDevices.GetN();
         i++)
    {
        Ptr<lrwpan::LrWpanNetDevice> device =
            DynamicCast<lrwpan::LrWpanNetDevice>(
                lrWpanDevices.Get(i));

        if (device)
        {
            // Keep every LR-WPAN short address unique. If devices retain
            // the default 0xffff address, 6LoWPAN can generate duplicate
            // IPv6 interface identifiers and abort with Address Collision.
            device->GetMac()->SetPanId(0);
            device->GetMac()->SetShortAddress(
                Mac16Address(static_cast<uint16_t>(i + 1)));
        }
    }


    // --------------------------------------------------------
    // 6LoWPAN
    // --------------------------------------------------------

    SixLowPanHelper sixlowpan;

    NetDeviceContainer sixlowpanDevices =
        sixlowpan.Install(
            lrWpanDevices);


    // --------------------------------------------------------
    // INTERNET STACK
    // --------------------------------------------------------

    InternetStackHelper internet;

    internet.Install(allNodes);


    // --------------------------------------------------------
    // IPV6 ADDRESSING
    // --------------------------------------------------------

    Ipv6AddressHelper ipv6;

    // Use a distinct prefix for each speed run as well, so the IPv6
    // address generator cannot collide when experiments are repeated.
    uint32_t prefixId = static_cast<uint32_t>(speed);
    string ipv6Prefix = "2001:db8:" + to_string(prefixId) + "::";

    ipv6.SetBase(
        Ipv6Address(ipv6Prefix.c_str()),
        Ipv6Prefix(64));


    Ipv6InterfaceContainer interfaces =
        ipv6.Assign(
            sixlowpanDevices);


    // --------------------------------------------------------
    // ENABLE IPV6 INTERFACES
    // --------------------------------------------------------

    for (uint32_t i = 0;
         i < allNodes.GetN();
         i++)
    {
        Ptr<Ipv6> ipv6Protocol =
            allNodes.Get(i)
                ->GetObject<Ipv6>();

        if (ipv6Protocol)
        {
            for (uint32_t j = 0;
                 j < ipv6Protocol->GetNInterfaces();
                 j++)
            {
                ipv6Protocol->SetUp(j);
            }
        }
    }


    // --------------------------------------------------------
    // GATEWAY IPV6 ADDRESS
    //
    // Node index:
    //
    // 0 = mobile 0
    // 1 = mobile 1
    // 2 = mobile 2
    // 3 = gateway
    //
    // Address index 1 = global IPv6 address
    // --------------------------------------------------------

    Ipv6Address gatewayAddress =
        interfaces.GetAddress(
            3,
            1);


    // --------------------------------------------------------
    // UDP SINK
    // --------------------------------------------------------

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",

        Inet6SocketAddress(
            Ipv6Address::GetAny(),
            ZIGBEE_PORT));


    ApplicationContainer sinkApp =
        sink.Install(
            gateway.Get(0));

    sinkApp.Start(
        Seconds(APPLICATION_START));

    sinkApp.Stop(
        Seconds(APPLICATION_STOP));


    // --------------------------------------------------------
    // UDP SOURCE
    // --------------------------------------------------------

    OnOffHelper source(
        "ns3::UdpSocketFactory",

        Inet6SocketAddress(
            gatewayAddress,
            ZIGBEE_PORT));


    source.SetAttribute(
        "DataRate",
        DataRateValue(
            DataRate("250Kbps")));

    source.SetAttribute(
        "PacketSize",
        UintegerValue(100));

    source.SetAttribute(
        "OnTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1]"));

    source.SetAttribute(
        "OffTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=0]"));


    ApplicationContainer sourceApp =
        source.Install(mobile);

    sourceApp.Start(
        Seconds(APPLICATION_START));

    sourceApp.Stop(
        Seconds(APPLICATION_STOP));


    // --------------------------------------------------------
    // FLOW MONITOR
    // --------------------------------------------------------

    FlowMonitorHelper flowHelper;

    Ptr<FlowMonitor> monitor =
        flowHelper.InstallAll();


    Simulator::Stop(
        Seconds(SIMULATION_TIME));

    Simulator::Run();


    // --------------------------------------------------------
    // FLOW STATISTICS
    // --------------------------------------------------------

    monitor->CheckForLostPackets();


    Ptr<Ipv6FlowClassifier> classifier =
        DynamicCast<Ipv6FlowClassifier>(
            flowHelper.GetClassifier6());


    map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();


    uint64_t totalTx = 0;
    uint64_t totalRx = 0;

    double totalDelay = 0.0;


    for (const auto &flow : stats)
    {
        Ipv6FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(
                flow.first);


        // UDP protocol = 17

        if (tuple.protocol != 17)
        {
            continue;
        }


        totalTx +=
            flow.second.txPackets;

        totalRx +=
            flow.second.rxPackets;

        totalDelay +=
            flow.second.delaySum.GetSeconds();
    }


    // --------------------------------------------------------
    // THROUGHPUT
    // --------------------------------------------------------

    double throughput = 0.0;

    if (totalRx > 0)
    {
        throughput =
            static_cast<double>(totalRx)
            * 100.0
            * 8.0
            / MEASUREMENT_TIME;
    }


    // --------------------------------------------------------
    // PACKET LOSS
    // --------------------------------------------------------

    double packetLoss = 0.0;

    if (totalTx > 0)
    {
        packetLoss =
            (
                static_cast<double>(
                    totalTx - totalRx)
                / static_cast<double>(totalTx)
            )
            * 100.0;
    }


    // --------------------------------------------------------
    // DELAY
    // --------------------------------------------------------

    double delay = 0.0;

    if (totalRx > 0)
    {
        delay =
            (
                totalDelay
                / static_cast<double>(totalRx)
            )
            * 1000.0;
    }


    // --------------------------------------------------------
    // RECEIVED POWER
    // --------------------------------------------------------

    double receivedPower =
        CalculateAverageReceivedPower(
            mobile,
            gateway,
            0.0,
            2.4e9);


    // --------------------------------------------------------
    // PRINT
    // --------------------------------------------------------

    PrintResult(
        "Zigbee",
        speed,
        receivedPower,
        throughput,
        packetLoss,
        delay);


    // --------------------------------------------------------
    // STORE
    // --------------------------------------------------------

    StoreResult(
        "Zigbee",
        speed,
        receivedPower,
        throughput,
        packetLoss,
        delay);


    Simulator::Destroy();
}


// ============================================================
// BLUETOOTH-LIKE EXPERIMENT
//
// NS-3.42 standard distribution does not contain a native
// Bluetooth network module.
//
// Therefore, short-range Wi-Fi is used as a
// Bluetooth-like wireless model.
// ============================================================

void
RunBluetoothExperiment(double speed)
{
    cout << endl;

    cout << "===================================================="
         << endl;

    cout << "       BLUETOOTH-LIKE EXPERIMENT"
         << endl;

    cout << "       Speed = "
         << speed
         << " m/s"
         << endl;

    cout << "===================================================="
         << endl;


    // --------------------------------------------------------
    // NODES
    // --------------------------------------------------------

    NodeContainer mobile;
    mobile.Create(3);

    NodeContainer gateway;
    gateway.Create(1);


    // --------------------------------------------------------
    // MOBILITY
    // --------------------------------------------------------

    InstallGatewayMobility(gateway);

    InstallMobileMobility(
        mobile,
        speed);


    // --------------------------------------------------------
    // SHORT-RANGE WIFI CHANNEL
    // --------------------------------------------------------

    YansWifiChannelHelper channel =
        YansWifiChannelHelper::Default();

    channel.AddPropagationLoss(
        "ns3::FriisPropagationLossModel");


    YansWifiPhyHelper phy;

    phy.SetChannel(
        channel.Create());


    // Lower power to represent
    // short-range communication

    phy.Set(
        "TxPowerStart",
        DoubleValue(4.0));

    phy.Set(
        "TxPowerEnd",
        DoubleValue(4.0));


    // --------------------------------------------------------
    // WIFI CONFIGURATION
    // --------------------------------------------------------

    WifiHelper wifi;

    wifi.SetStandard(
        WIFI_STANDARD_80211g);

    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",

        "DataMode",
        StringValue("ErpOfdmRate6Mbps"),

        "ControlMode",
        StringValue("ErpOfdmRate6Mbps"));


    WifiMacHelper mac;

    Ssid ssid =
        Ssid("Campus-Bluetooth");


    // --------------------------------------------------------
    // MOBILE NODES
    // --------------------------------------------------------

    mac.SetType(
        "ns3::StaWifiMac",

        "Ssid",
        SsidValue(ssid),

        "ActiveProbing",
        BooleanValue(false));


    NetDeviceContainer mobileDevices =
        wifi.Install(
            phy,
            mac,
            mobile);


    // --------------------------------------------------------
    // GATEWAY
    // --------------------------------------------------------

    mac.SetType(
        "ns3::ApWifiMac",

        "Ssid",
        SsidValue(ssid));


    NetDeviceContainer gatewayDevice =
        wifi.Install(
            phy,
            mac,
            gateway);


    // --------------------------------------------------------
    // INTERNET
    // --------------------------------------------------------

    InternetStackHelper internet;

    internet.Install(mobile);

    internet.Install(gateway);


    // --------------------------------------------------------
    // IPV4 ADDRESSING
    // --------------------------------------------------------

    Ipv4AddressHelper address;

    address.SetBase(
        "10.3.0.0",
        "255.255.255.0");


    NetDeviceContainer allBluetoothDevices;
    allBluetoothDevices.Add(mobileDevices);
    allBluetoothDevices.Add(gatewayDevice);

    Ipv4InterfaceContainer bluetoothInterfaces =
        address.Assign(allBluetoothDevices);

    // Device/interface order: mobile nodes 0-2, gateway 3.


    // --------------------------------------------------------
    // UDP SINK
    // --------------------------------------------------------

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",

        InetSocketAddress(
            Ipv4Address::GetAny(),
            BLUETOOTH_PORT));


    ApplicationContainer sinkApp =
        sink.Install(
            gateway.Get(0));

    sinkApp.Start(
        Seconds(APPLICATION_START));

    sinkApp.Stop(
        Seconds(APPLICATION_STOP));


    // --------------------------------------------------------
    // UDP SOURCE
    // --------------------------------------------------------

    OnOffHelper source(
        "ns3::UdpSocketFactory",

        InetSocketAddress(
            bluetoothInterfaces.GetAddress(3),
            BLUETOOTH_PORT));


    source.SetAttribute(
        "DataRate",
        DataRateValue(
            DataRate("100Kbps")));

    source.SetAttribute(
        "PacketSize",
        UintegerValue(200));

    source.SetAttribute(
        "OnTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1]"));

    source.SetAttribute(
        "OffTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=0]"));


    ApplicationContainer sourceApp =
        source.Install(mobile);

    sourceApp.Start(
        Seconds(APPLICATION_START));

    sourceApp.Stop(
        Seconds(APPLICATION_STOP));


    // --------------------------------------------------------
    // FLOW MONITOR
    // --------------------------------------------------------

    FlowMonitorHelper flowHelper;

    Ptr<FlowMonitor> monitor =
        flowHelper.InstallAll();


    Simulator::Stop(
        Seconds(SIMULATION_TIME));

    Simulator::Run();


    // --------------------------------------------------------
    // FLOW STATISTICS
    // --------------------------------------------------------

    monitor->CheckForLostPackets();


    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());


    map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();


    uint64_t totalTx = 0;
    uint64_t totalRx = 0;

    double totalDelay = 0.0;


    for (const auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(
                flow.first);


        // UDP = 17

        if (tuple.protocol != 17)
        {
            continue;
        }


        totalTx +=
            flow.second.txPackets;

        totalRx +=
            flow.second.rxPackets;

        totalDelay +=
            flow.second.delaySum.GetSeconds();
    }


    // --------------------------------------------------------
    // THROUGHPUT
    // --------------------------------------------------------

    double throughput = 0.0;

    if (totalRx > 0)
    {
        throughput =
            static_cast<double>(totalRx)
            * 200.0
            * 8.0
            / MEASUREMENT_TIME;
    }


    // --------------------------------------------------------
    // PACKET LOSS
    // --------------------------------------------------------

    double packetLoss = 0.0;

    if (totalTx > 0)
    {
        packetLoss =
            (
                static_cast<double>(
                    totalTx - totalRx)
                / static_cast<double>(totalTx)
            )
            * 100.0;
    }


    // --------------------------------------------------------
    // DELAY
    // --------------------------------------------------------

    double delay = 0.0;

    if (totalRx > 0)
    {
        delay =
            (
                totalDelay
                / static_cast<double>(totalRx)
            )
            * 1000.0;
    }


    // --------------------------------------------------------
    // RECEIVED POWER
    // --------------------------------------------------------

    double receivedPower =
        CalculateAverageReceivedPower(
            mobile,
            gateway,
            4.0,
            2.4e9);


    // --------------------------------------------------------
    // PRINT
    // --------------------------------------------------------

    PrintResult(
        "Bluetooth-like",
        speed,
        receivedPower,
        throughput,
        packetLoss,
        delay);


    // --------------------------------------------------------
    // STORE
    // --------------------------------------------------------

    StoreResult(
        "Bluetooth-like",
        speed,
        receivedPower,
        throughput,
        packetLoss,
        delay);


    Simulator::Destroy();
}


// ============================================================
// SAVE RESULTS TO CSV
// ============================================================

void
SaveResults()
{
    ofstream csv;

    csv.open(
        "assign11-results.csv");


    if (!csv.is_open())
    {
        cerr
            << "ERROR: Could not open "
            << "assign11-results.csv"
            << endl;

        return;
    }


    // CSV header

    csv
        << "Technology,"
        << "Speed_mps,"
        << "ReceivedPower_dBm,"
        << "Throughput_Kbps,"
        << "PacketLoss_Percent,"
        << "Delay_ms"
        << endl;


    // Write results

    for (const auto &result : allResults)
    {
        csv
            << result.technology
            << ","
            << result.speed
            << ","
            << result.receivedPower
            << ","
            << result.throughput / 1000.0
            << ","
            << result.packetLoss
            << ","
            << result.delay
            << endl;
    }


    csv.close();


    cout << endl;

    cout << "===================================================="
         << endl;

    cout << "Results saved to:"
         << endl;

    cout << "assign11-results.csv"
         << endl;

    cout << "===================================================="
         << endl;
}


// ============================================================
// MAIN
// ============================================================

int
main()
{
    cout << endl;

    cout << "=========================================================="
         << endl;

    cout << "                  ASSIGNMENT 11"
         << endl;

    cout << "     PERFORMANCE ANALYSIS OF A HETEROGENEOUS"
         << endl;

    cout << "                 WIRELESS NETWORK"
         << endl;

    cout << "=========================================================="
         << endl;


    cout << endl;

    cout << "Technologies:"
         << endl;

    cout << "1. Wi-Fi"
         << endl;

    cout << "2. Zigbee (IEEE 802.15.4 + 6LoWPAN)"
         << endl;

    cout << "3. Bluetooth-like short-range wireless"
         << endl;


    cout << endl;

    cout << "Mobility Model:"
         << endl;

    cout << "RandomWalk2dMobilityModel"
         << endl;


    cout << endl;

    cout << "Mobility Speeds:"
         << endl;

    cout << "1 m/s"
         << endl;

    cout << "5 m/s"
         << endl;

    cout << "10 m/s"
         << endl;


    // ========================================================
    // SPEED = 1 m/s
    // ========================================================

    cout << endl;

    cout << "##########################################################"
         << endl;

    cout << "                    SPEED = 1 m/s"
         << endl;

    cout << "##########################################################"
         << endl;


    RunWifiExperiment(1.0);

    RunZigbeeExperiment(1.0);

    RunBluetoothExperiment(1.0);


    // ========================================================
    // SPEED = 5 m/s
    // ========================================================

    cout << endl;

    cout << "##########################################################"
         << endl;

    cout << "                    SPEED = 5 m/s"
         << endl;

    cout << "##########################################################"
         << endl;


    RunWifiExperiment(5.0);

    RunZigbeeExperiment(5.0);

    RunBluetoothExperiment(5.0);


    // ========================================================
    // SPEED = 10 m/s
    // ========================================================

    cout << endl;

    cout << "##########################################################"
         << endl;

    cout << "                   SPEED = 10 m/s"
         << endl;

    cout << "##########################################################"
         << endl;


    RunWifiExperiment(10.0);

    RunZigbeeExperiment(10.0);

    RunBluetoothExperiment(10.0);


    // ========================================================
    // SAVE RESULTS
    // ========================================================

    SaveResults();


    // ========================================================
    // COMPLETE
    // ========================================================

    cout << endl;

    cout << "=========================================================="
         << endl;

    cout << "             ALL EXPERIMENTS COMPLETED"
         << endl;

    cout << "=========================================================="
         << endl;

    cout << endl;

    cout << "Total experiments : 9"
         << endl;

    cout << "Results file      : assign11-results.csv"
         << endl;

    cout << endl;


    return 0;
}
