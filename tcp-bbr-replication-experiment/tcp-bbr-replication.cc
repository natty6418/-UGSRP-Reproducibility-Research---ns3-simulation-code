#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/error-model.h"
#include "ns3/ping-helper.h"
#include "ns3/data-rate.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h" 
#include "ns3/trace-helper.h"


// #include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>

using namespace ns3;
// std::string dir = "results/";
Time stopTime = Seconds(60);
uint32_t segmentSize = 1448;

// std::ofstream fPlotSsthresh;
std::ofstream fPlotQueue;
// std::ofstream fPlotCwnd;

// Function to check queue length of Router 1
void
CheckQueueSize(Ptr<QueueDisc> queue)
{
    uint32_t qSize = queue->GetCurrentSize().GetValue();

    // Check queue size every 1/100 of a second
    Simulator::Schedule(Seconds(0.001), &CheckQueueSize, queue);
    fPlotQueue << Simulator::Now().GetSeconds() << " " << qSize << std::endl;
}

// Function to trace change in cwnd at n0
// Function to calculate drops in a particular Queue
static void
DropAtQueue(Ptr<OutputStreamWrapper> stream, Ptr<const QueueDiscItem> item)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " 1" << std::endl;
}
// static void
// CwndChange(uint16_t port, uint32_t oldCwnd, uint32_t newCwnd)
// {
//     //convert cwnd from bytes to number of segments
//     fPlotCwnd << Simulator::Now().GetSeconds() << " " << newCwnd / segmentSize <<" "<<port<< std::endl;
// }

// static void SsthreshChange(uint16_t port, uint32_t oldSsthresh, uint32_t newSsthresh)
// {
//     fPlotSsthresh << Simulator::Now().GetSeconds() << " " << newSsthresh / segmentSize << " "<< port<< std::endl;
// }


// Trace Function for cwnd
// void
// TraceCwnd(uint32_t node, uint32_t cwndWindow, uint16_t port)
// {
//     Config::ConnectWithoutContext("/NodeList/" + std::to_string(node) +
//                                       "/$ns3::TcpL4Protocol/SocketList/" +
//                                       std::to_string(cwndWindow) + "/CongestionWindow",
//                                     MakeBoundCallback(&CwndChange, port)
//                                   );
// }

// // Trace Function for ssthresh
// void TraceSsthresh(uint32_t node, uint32_t cwndWindow, uint16_t port)
// {    
//     Config::ConnectWithoutContext("/NodeList/" + std::to_string(node) +
//                                   "/$ns3::TcpL4Protocol/SocketList/" +
//                                   std::to_string(cwndWindow) + "/SlowStartThreshold",
//                                     MakeBoundCallback(&SsthreshChange, port)
//                                   );
// }

//Sender side
// Function to install BulkSend application
void InstallBulkSend(Ptr<Node> node, Ipv4Address address, uint16_t port,
                     std::string socketFactory,
                     uint16_t num_flows = 1)
{
    for (uint16_t i = 0; i < num_flows; ++i)
    {
        BulkSendHelper source(socketFactory, InetSocketAddress(address, port + i));
        source.SetAttribute("MaxBytes", UintegerValue(0));
        ApplicationContainer sourceApps = source.Install(node);
        sourceApps.Start(Seconds(1.0 + i * 0.1)); // Stagger the start times slightly
        // Simulator::Schedule(Seconds(1.0 + i * 0.1) + Seconds(0.001), &TraceCwnd, nodeId, cwndWindow, port + i);
        // Simulator::Schedule(Seconds(1.0 + i * 0.1) + Seconds(0.001), &TraceSsthresh, nodeId, cwndWindow, port + i);
        sourceApps.Stop(stopTime+Seconds(i * 0.1)); // Ensure stopTime is set appropriately and staggered
    }
}
void InstallOnOff(Ptr<Node> node, Ipv4Address address, uint16_t port,
                  std::string socketFactory,
                  DataRate dataRate, uint16_t num_flows = 1, double stopTime = 100.0)
{
    for (uint16_t i = 0; i < num_flows; ++i)
    {
        // Configure the OnOff application to send traffic to the specified address and port
        OnOffHelper onOffHelper(socketFactory, InetSocketAddress(address, port + i));
        onOffHelper.SetAttribute("DataRate", DataRateValue(dataRate));
        onOffHelper.SetAttribute("PacketSize", UintegerValue(1448)); // Set packet size (default is 512 bytes)
        onOffHelper.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        // Install the OnOff application on the specified node
        ApplicationContainer sourceApps = onOffHelper.Install(node);
        sourceApps.Start(Seconds(1.0 + i * 0.1)); // Stagger start times slightly
        sourceApps.Stop(Seconds(stopTime + i * 0.1)); // Stagger stop times appropriately

        // Schedule tracing functions for congestion window and slow start threshold
        // Simulator::Schedule(Seconds(1.0 + i * 0.1) + Seconds(0.001), &TraceCwnd, nodeId, cwndWindow, port + i);
        // Simulator::Schedule(Seconds(1.0 + i * 0.1) + Seconds(0.001), &TraceSsthresh, nodeId, cwndWindow, port + i);
    }
}

//Receiver side
// Function to install sink application
void
InstallPacketSink(Ptr<Node> node, uint16_t port, std::string socketFactory)
{
    PacketSinkHelper sink(socketFactory, InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(node);
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(stopTime);
}


int
main(int argc, char* argv[])
{
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("TcpL4Protocol", LOG_LEVEL_INFO);


    // uint32_t num_streams = 1;
    std::string socketFactory = "ns3::TcpSocketFactory";
    std::string tcpTypeId = "ns3::TcpCubic";
    std::string qdiscTypeId = "ns3::FifoQueueDisc";
    bool isSack = true;
    uint32_t delAckCount = 1;
    // std::string recovery = "ns3::TcpClassicRecovery";
    std::string errorModelType = "ns3::RateErrorModel";
    std::string qdiscSize = "0.1MB";
    std::string delay = "4.8ms";
    std::string bottleneck_bandwidth = "1.25Mbps";

    CommandLine cmd;
    cmd.AddValue("tcpTypeId",
                 "TCP variant to use (e.g., ns3::TcpNewReno, ns3::TcpLinuxReno, etc.)",
                 tcpTypeId);
    cmd.AddValue("qdiscTypeId", "Queue disc for gateway (e.g., ns3::CoDelQueueDisc, ns3::FifoQueueDisc)", qdiscTypeId);
    cmd.AddValue("segmentSize", "TCP segment size (bytes)", segmentSize);
    cmd.AddValue("delAckCount", "Delayed ack count", delAckCount);
    cmd.AddValue("enableSack", "Flag to enable/disable sack in TCP", isSack);
    cmd.AddValue("stopTime",
                 "Stop time for applications / simulation time will be stopTime",
                 stopTime);
    // cmd.AddValue("recovery", "Recovery algorithm type to use (e.g., ns3::TcpPrrRecovery", recovery);
    cmd.AddValue("qdiscSize", "Size of the queue", qdiscSize);
    cmd.AddValue("delay", "Delay of the link", delay);
    cmd.AddValue("bottleneck_bandwidth", "Bandwidth of the bottleneck link", bottleneck_bandwidth);
    cmd.Parse(argc, argv);

    // --dir="output_${qdiscSize}_${bottleneck_bandwidth}_${delay}_${tcpTypeId}_${trial}" 
    std::string tcpTypeIdStr;
    if (tcpTypeId == "ns3::TcpCubic"){
        tcpTypeIdStr = "Cubic";
    } else if (tcpTypeId == "ns3::TcpBbr" ){
        tcpTypeIdStr = "Bbr";
    }
    

    std::string dir = "tcp-bbr-cubic-results/" + qdiscSize + "_" + bottleneck_bandwidth + "_" + delay + "_" + tcpTypeIdStr + "/";

    // TypeId qdTid;
    // NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(qdiscTypeId, &qdTid),
    //                     "TypeId " << qdiscTypeId << " not found");

    // Set recovery algorithm and TCP variant
    // Config::SetDefault("ns3::TcpL4Protocol::RecoveryType",
    //                    TypeIdValue(TypeId::LookupByName(recovery)));
    TypeId tcpTid;
    NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(tcpTypeId, &tcpTid),
                        "TypeId " << tcpTypeId << " not found");
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TypeId::LookupByName(tcpTypeId)));


    // Set default sender and receiver buffer size as 1MB
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(2147483647));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(2147483647));

    // Set default initial congestion window as 10 segments
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));

    // Config::SetDefault("ns3::TcpSocket::InitialSlowStartThreshold", UintegerValue(segmentSize*10));

    // Set default delayed ack count to a specified value
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(delAckCount));

    // Set default segment size of TCP packet to a specified value
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segmentSize));
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("1p")));

    // Enable/Disable SACK in TCP
    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(isSack));

    // Create nodes
    NodeContainer leftNode;
    NodeContainer rightNode;
    NodeContainer router;
    router.Create(1);
    leftNode.Create(1);
    rightNode.Create(1);

    // Create the point-to-point link helpers and connect two router nodes
    PointToPointHelper accessLink;
    accessLink.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    accessLink.SetChannelAttribute("Delay", StringValue(delay));
    // accessLink.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("1p")));

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue(bottleneck_bandwidth));
    bottleneckLink.SetChannelAttribute("Delay", StringValue(delay));
    bottleneckLink.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("1p")));
    
    NetDeviceContainer leftToRouter = accessLink.Install(leftNode.Get(0), router.Get(0));
    NetDeviceContainer routerToRight = bottleneckLink.Install(router.Get(0), rightNode.Get(0));

    InternetStackHelper internetStack;

    internetStack.Install(leftNode);
    internetStack.Install(rightNode);
    internetStack.Install(router);

    // Assign IP addresses to all the network devices
    Ipv4AddressHelper ipAddresses("10.0.0.0", "255.255.255.0");

    std::vector<Ipv4InterfaceContainer> leftToRouterIPAddress;
    std::vector<Ipv4InterfaceContainer> routerToRightIPAddress;

    leftToRouterIPAddress.push_back(ipAddresses.Assign(leftToRouter));
    
    ipAddresses.NewNetwork();
    routerToRightIPAddress.push_back(ipAddresses.Assign(routerToRight));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    

    // Create directories to store dat files
    struct stat buffer;
    int retVal [[maybe_unused]];
    if ((stat(dir.c_str(), &buffer)) == 0)
    {
        std::string dirToRemove = "rm -rf " + dir;
        retVal = system(dirToRemove.c_str());
        NS_ASSERT_MSG(retVal == 0, "Error in return value");
    }

    SystemPath::MakeDirectories(dir);
    // SystemPath::MakeDirectories(dir + "/pcap/");
    SystemPath::MakeDirectories(dir + "/queueTraces/");
    // SystemPath::MakeDirectories(dir + "/cwndTraces/");

    // Install flow monitor on all the nodes
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();


    // Install queue discipline on router
    TrafficControlHelper tch;
    tch.SetRootQueueDisc(qdiscTypeId, "MaxSize", QueueSizeValue(QueueSize(qdiscSize)));
    QueueDiscContainer qd;
    tch.Uninstall(leftToRouter.Get(1));
    tch.Uninstall(routerToRight.Get(0));
    tch.Install(leftToRouter.Get(1));
    qd = tch.Install(routerToRight.Get(0));


    // Open files for writing queue size and cwnd traces
    fPlotQueue.open(dir + "queue-size.dat", std::ios::out);
    // fPlotCwnd.open(dir + "cwndTraces/n0.dat", std::ios::out);
    // fPlotSsthresh.open(dir + "cwndTraces/ssthresh.dat", std::ios::out);

    // Calls function to check queue size
    Simulator::ScheduleNow(&CheckQueueSize, qd.Get(0));

    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> streamWrapper;

    // // Create dat to store packets dropped and marked at the router
    streamWrapper = asciiTraceHelper.CreateFileStream(dir + "/queueTraces/drop-0.dat");
    qd.Get(0)->TraceConnectWithoutContext("Drop", MakeBoundCallback(&DropAtQueue, streamWrapper));

    // Install packet sink at receiver side
    uint16_t port = 50000;
    InstallPacketSink(rightNode.Get(0), port, "ns3::TcpSocketFactory");
    
    // Install BulkSend application

    InstallBulkSend(leftNode.Get(0), routerToRightIPAddress[0].GetAddress(1), port,
                        socketFactory, 1);

    // // Install OnOff application
    // InstallOnOff(leftNode.Get(0), routerToRightIPAddress[0].GetAddress(1), port,
    //                 socketFactory, leftNode.Get(0)->GetId(), 0, DataRate("100Mbps"), 1, stopTime.GetSeconds());

    // Ping from leftNode to rightNode
    // PingHelper pinghelper(routerToRightIPAddress[0].GetAddress(1), leftToRouterIPAddress[0].GetAddress(0));
    // pinghelper.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    // pinghelper.SetAttribute("Size", UintegerValue(100));
    // // pinghelper.SetAttribute("Verbose", BooleanValue(true));
    // ApplicationContainer pingApps = pinghelper.Install(leftNode.Get(0));
    // pingApps.Start(Seconds(1.0));
    // pingApps.Stop(stopTime);


    // Enable PCAP on all the point to point interfaces
    // accessLink.EnablePcapAll(dir + "pcap/ns-3", true);

    Simulator::Stop(stopTime);
    Simulator::Run();

    monitor->CheckForLostPackets(); // Optional, helps in accounting for lost packets
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    std::ofstream resultFile;
    resultFile.open(dir + "goodput_retransmission_results.txt", std::fstream::out);
    for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        resultFile << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        resultFile << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        resultFile << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        resultFile << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        resultFile << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        resultFile << "  Lost Packets: " << i->second.lostPackets << "\n";
        std::cout << "  Lost Packets: " << i->second.lostPackets << "\n";
        resultFile << "  Throughput: " << i->second.rxBytes * 8.0 / stopTime.GetSeconds() / 1024 / 1024  << " Mbps\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / stopTime.GetSeconds() / 1024 / 1024  << " Mbps\n";
        uint32_t retransmissions = i->second.txPackets - i->second.rxPackets - i->second.lostPackets;
        resultFile << "  Retransmissions: " << retransmissions << "\n";
        std::cout << "  Retransmissions: " << retransmissions << "\n";
    }
    resultFile.close();


    // Store queue stats in a file
    std::ofstream myfile;
    myfile.open(dir + "queueStats.txt", std::fstream::in | std::fstream::out | std::fstream::app);
    myfile << std::endl;
    myfile << "Stat for Queue 1";
    myfile << qd.Get(0)->GetStats();
    myfile.close();

    // Store configuration of the simulation in a file
    myfile.open(dir + "config.txt", std::fstream::in | std::fstream::out | std::fstream::app);
    myfile << "qdiscTypeId " << qdiscTypeId << "\n";
    // myfile << "stream  " << num_streams << "\n";
    myfile << "segmentSize " << segmentSize << "\n";
    myfile << "delAckCount " << delAckCount << "\n";
    myfile << "stopTime " << stopTime.As(Time::S) << "\n";
    myfile.close();

    Simulator::Destroy();

    // fPlotQueue.close();
    // fPlotCwnd.close();

    return 0;
}
