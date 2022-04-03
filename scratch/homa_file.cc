/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Stanford University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Serhat Arslan <sarslan@stanford.edu>
 */

// The topology used in this simulation is provided in Homa paper [1] in detail.
//
// The topology consists of 144 hosts divided among 9 racks with a 2-level switching 
// fabric. Host links operate at 10Gbps and TOR-aggregation links operate at 40 Gbps.
//
// [1] Behnam Montazeri, Yilong Li, Mohammad Alizadeh, and John Ousterhout.  
//     2018. Homa: a receiver-driven low-latency transport protocol using  
//     network priorities. In Proceedings of the 2018 Conference of the ACM  
//     Special Interest Group on Data Communication (SIGCOMM '18). Association  
//     for Computing Machinery, New York, NY, USA, 221–235. 
//     DOI:https://doi.org/10.1145/3230543.3230564

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
// #include "ns3/imi-client-app.h"

using namespace ns3;

uint32_t m_mtu = 0;
uint32_t m_header_size = 0;
//cal_average_throughput
uint64_t flow_start_time = 0;
uint64_t last_NewDataArrivalTime = 0;
//queue length
uint16_t nPort = 160;
uint64_t Highest_Queue_Length[160] = {0};

uint64_t Global_MsgId = 0; //第一版用于判断的
uint64_t Msg_begin_id = 0;
uint64_t all_pkt_count = 0; 
uint64_t all_pkt_size_count = 0;

uint64_t pkt_count = 0;
uint64_t pkt_size_count = 0;
// uint64_t last_throughput = 0;
uint64_t last_time = 0;

uint16_t zero_times = 0;
bool last_tp_interval = false;

int active_flow = 0;

NS_LOG_COMPONENT_DEFINE ("HomaL4ProtocolPaperReproduction");

std::vector<Ipv4Address> serverAddress;
std::vector<Ipv4Address> hostAddressList;

std::vector<Ptr<ImiClientApp>> ClientApps;
// maintain port number for each host
std::unordered_map<uint32_t, uint16_t> portNumder;

struct FlowInput{
  uint32_t src, dst, MsgSize, port, pg;
  double start_time;
  uint32_t idx;
};
FlowInput flow_input = {0};
uint32_t flow_num;

std::ifstream flowf;
Ptr<OutputStreamWrapper> throughput_summary;

void ReadFlowInput(){ //用flow_input这个全局结构来存储流
  if (flow_input.idx < flow_num){
    flowf >> flow_input.src >> flow_input.dst >> flow_input.pg >> flow_input.port >> flow_input.MsgSize >> flow_input.start_time;
    // std::cout << " src " << flow_input.src <<  " dst " << flow_input.dst 
    //           << " pg " << flow_input.pg << " port " << flow_input.port
    //           <<  " MsgSize " << flow_input.MsgSize <<  " start_time " << flow_input.start_time
    //           << std::endl;
    NS_ASSERT(flow_input.src != flow_input.dst);
  }
}
void ScheduleFlowInputs(Ipv4InterfaceContainer hostTorIfs[]){
  // uint32_t bdp_t = m_mtu * 34; // 配置
  std::cout << " flow_num " << flow_num << std::endl;
  while (flow_input.idx < flow_num){
    Msg_begin_id = flow_input.idx;
    Ptr<ImiClientApp> app = ClientApps[flow_input.src];
    InetSocketAddress dst_Socket = InetSocketAddress (hostTorIfs[flow_input.dst].GetAddress (0), 10000+flow_input.dst);
    app->SetEvent(dst_Socket, flow_input.MsgSize, flow_input.start_time);
    // app->SetEvent(dst_Socket, flow_input.MsgSize*bdp_t, flow_input.start_time);

    // app->SetMsgSize (flow_input.MsgSize);
    // app->SetDst (dst_Socket);
    // app->Start(Seconds (flow_input.start_time));

    // get the next flow input
    flow_input.idx++;
    ReadFlowInput();
  }

  // schedule the next time to run this function
  if (flow_input.idx < flow_num){
    ScheduleFlowInputs(hostTorIfs);
    // Simulator::Schedule(Seconds(flow_input.start_time)-Simulator::Now(), ScheduleFlowInputs, hostTorIfs);
  }else { // no more flows, close the file
    flowf.close();
  }
}



void TraceMsgBegin (Ptr<OutputStreamWrapper> stream,
                    Ptr<const Packet> msg, Ipv4Address saddr, Ipv4Address daddr, 
                    uint16_t sport, uint16_t dport, int txMsgId)
{
  NS_LOG_DEBUG("+ " << Simulator::Now ().GetNanoSeconds ()
                << " " << msg->GetSize()
                << " " << saddr << ":" << sport 
                << " "  << daddr << ":" << dport 
                << " " << txMsgId);
    
  *stream->GetStream () << "+ " << Simulator::Now ().GetNanoSeconds () 
      << " " << msg->GetSize()
      << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
      << " " << txMsgId << std::endl;

  // zero_times++;
  std::cout << "New " << Simulator::Now ().GetNanoSeconds () 
      << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
      << " " << msg->GetSize() 
      << " " << txMsgId 
      << " " << Global_MsgId << std::endl;
  Global_MsgId++; 
  active_flow ++;
  // uint64_t now = Simulator::Now ().GetNanoSeconds ();
  // uint64_t tp_interval = now-last_time;

  // if ( tp_interval != 0)
  //   throughput = pkt_size_count*8/tp_interval; //Gbps
  // else
  //   throughput = 0;

  // last_time = now;
  // pkt_size_count = 0;
  // pkt_count = 0;

  // std::cout << "TP start " << Simulator::Now ().GetNanoSeconds () 
  //      << " all_pkt_count " << all_pkt_count
  //      << " all_pkt_size_count " << all_pkt_size_count
  //      << " TP " << throughput
  //      << " MsgId " << Global_MsgId
  //      << std::endl;

  //       pkt_size_count += msg->GetSize();
  // pkt_count++;
}

void TraceMsgFinish (Ptr<OutputStreamWrapper> stream,
                     Ptr<const Packet> msg, Ipv4Address saddr, Ipv4Address daddr, 
                     uint16_t sport, uint16_t dport, int txMsgId)
{
  NS_LOG_DEBUG("- " << Simulator::Now ().GetNanoSeconds () 
                << " " << msg->GetSize()
                << " " << saddr << ":" << sport 
                << " "  << daddr << ":" << dport 
                << " " << txMsgId);
    
  *stream->GetStream () << "- " << Simulator::Now ().GetNanoSeconds () 
      << " " << msg->GetSize()
      << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
      << " " << txMsgId << std::endl;

  active_flow --;
}

static void
BytesInQueueDiscTrace (Ptr<OutputStreamWrapper> stream, int hostIdx, 
                       uint32_t oldval, uint32_t newval)
{
  // NS_LOG_DEBUG (Simulator::Now ().GetNanoSeconds () <<
  //              " Queue Disc size from " << oldval << " to " << newval);
  if ( newval > Highest_Queue_Length[hostIdx] )
    Highest_Queue_Length[hostIdx] = newval;
  // std::cout << "Queue_change " <<Simulator::Now ().GetNanoSeconds ()
  //           << " HostIdx=" << hostIdx << " OldQueueSize=" << oldval
  //           << " NewQueueSize=" << newval << std::endl;

  *stream->GetStream () << Simulator::Now ().GetNanoSeconds ()
                        << " HostIdx=" << hostIdx << " OldQueueSize=" << oldval
                        << " NewQueueSize=" << newval << std::endl;
}

/*
void TraceDataPktArrival (Ptr<OutputStreamWrapper> stream,
                          Ptr<const Packet> msg, Ipv4Address saddr, Ipv4Address daddr, 
                          uint16_t sport, uint16_t dport, int txMsgId,
                          uint16_t pktOffset, uint8_t prio)
{
  // NS_LOG_DEBUG("- " << Simulator::Now ().GetNanoSeconds () 
  //     << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
  //     << " " << txMsgId << " " << pktOffset << " " << (uint16_t)prio);
    
  // *stream->GetStream () << "- "  <<Simulator::Now ().GetNanoSeconds () 
  //     << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
  //     << " " << txMsgId << " " << pktOffset << " " << (uint16_t)prio << std::endl;
  if ( flow_start_time==0 ) //记录第一个ack什么时候开始，通过最后一次更新
  {
    flow_start_time = Simulator::Now ().GetNanoSeconds ();
    std::cout << "all_flow_start_time " << flow_start_time << std::endl;
  } 
    
  last_NewDataArrivalTime = Simulator::Now ().GetNanoSeconds ();

  all_pkt_size_count += msg->GetSize() + m_header_size; // B
  all_pkt_count++;

  pkt_size_count += msg->GetSize() + m_header_size;
  pkt_count++;
}
*/

std::map<Ipv4Address, int> RxDataArrivalRecord;
void TraceDataPktArrival (Ptr<OutputStreamWrapper> stream,
                          Ptr<const Packet> msg, Ipv4Address saddr, Ipv4Address daddr, 
                          uint16_t sport, uint16_t dport, int txMsgId,
                          uint16_t pktOffset, uint8_t prio)
{
 
  if (RxDataArrivalRecord.find(daddr) == RxDataArrivalRecord.end())
    RxDataArrivalRecord[daddr] = 0;
    
  uint32_t pkt_size = msg->GetSize() + m_header_size; // B
  RxDataArrivalRecord[daddr] += pkt_size;

}

void TraceCtrlPktArrival (Ptr<OutputStreamWrapper> stream,
                          Ptr<const Packet> msg, Ipv4Address saddr, Ipv4Address daddr, 
                          uint16_t sport, uint16_t dport, uint8_t flag,
                          uint16_t grantOffset, uint8_t prio)
{
  if (RxDataArrivalRecord.find(daddr) == RxDataArrivalRecord.end())
    RxDataArrivalRecord[daddr] = 0;
    
  uint32_t pkt_size = msg->GetSize() + m_header_size; // B
  RxDataArrivalRecord[daddr] += pkt_size;
}


void CalculateThroughputByRx (Ptr<OutputStreamWrapper> stream, uint32_t tp_interval)
{
  uint64_t now = Simulator::Now ().GetNanoSeconds ();
  float throughput_sum = 0;

  *stream->GetStream () << now ;

  for (auto it: hostAddressList) {
    if (RxDataArrivalRecord.find(it) == RxDataArrivalRecord.end()) {
      *stream->GetStream () << '\t' << 0;
    }
    else {      
      float throughput = RxDataArrivalRecord[it]*8*1000/tp_interval; //Mbps
      throughput_sum += throughput;
      *stream->GetStream () << '\t' << throughput;
    }
  }

  *stream->GetStream () <<  "\n";
  *throughput_summary->GetStream () << now << "\t" << (int)throughput_sum << std::endl;

  // for (auto it = RxDataArrivalRecord.begin(); it != RxDataArrivalRecord.end(); it++) {
  //   *stream->GetStream () << '\t' << it->first << '\t' << it->second;
  // }
  // *stream->GetStream () <<  "\n";

  for (auto it = RxDataArrivalRecord.begin(); it != RxDataArrivalRecord.end(); it++) {
    it->second = 0;
  }

  Simulator::Schedule(NanoSeconds(tp_interval), &CalculateThroughputByRx, stream, tp_interval);

}


void TraceRxQlenByRx (Ptr<OutputStreamWrapper> stream, uint64_t qlen_interval, QueueDiscContainer* qd, int nhost)
{
  *stream->GetStream () << Simulator::Now ().GetNanoSeconds () ;

  for (int i = 0; i < nhost; i++) 
  {
    Ptr<QueueDisc> q = qd[i].Get(0);
    *stream->GetStream () << "\t" << q->GetNBytes();
  }
  *stream->GetStream () << "\n";

  Simulator::Schedule(NanoSeconds(qlen_interval), TraceRxQlenByRx, stream, qlen_interval, qd, nhost);
}


void TraceActive (Ptr<OutputStreamWrapper> stream, uint64_t active_interval) 
{
  *stream->GetStream () << Simulator::Now ().GetNanoSeconds () << "\t" << active_flow << "\n";
  Simulator::Schedule(NanoSeconds(active_interval), TraceActive, stream, active_interval);
}

void TraceDataPktDeparture (Ptr<OutputStreamWrapper> stream,
                            Ptr<const Packet> msg, Ipv4Address saddr, Ipv4Address daddr, 
                            uint16_t sport, uint16_t dport, int txMsgId,
                            uint16_t pktOffset, uint16_t prio)
{

  NS_LOG_DEBUG("+ " << Simulator::Now ().GetNanoSeconds () 
      << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
      << " " << txMsgId << " " << pktOffset);// << " " << (uint16_t)prio);
    
  *stream->GetStream () << "+ "  <<Simulator::Now ().GetNanoSeconds () 
      << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
      << " " << txMsgId << " " << pktOffset << " " << std::endl;
//       << " " << txMsgId << " " << pktOffset << " " << (uint16_t)prio << std::endl;
}
// void TraceCtrlPktArrival (Ptr<OutputStreamWrapper> stream,
//                           Ptr<const Packet> msg, Ipv4Address saddr, Ipv4Address daddr, 
//                           uint16_t sport, uint16_t dport, uint8_t flag,
//                           uint16_t grantOffset, uint8_t prio)
// {
//   NS_LOG_DEBUG("- " << Simulator::Now ().GetNanoSeconds () 
//       << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
//       << " " << HomaHeader::FlagsToString(flag) << " " << grantOffset 
//       << " " << (uint16_t)prio);
//    
//   *stream->GetStream () << "- "  <<Simulator::Now ().GetNanoSeconds () 
//       << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
//       << " " << HomaHeader::FlagsToString(flag) << " " << grantOffset 
//       << " " << (uint16_t)prio << std::endl;
// }

void cal_fct(Ptr<OutputStreamWrapper> stream,
             Ptr<const Packet> msg, Ipv4Address saddr, Ipv4Address daddr, 
             uint16_t sport, uint16_t dport,
             Time MsgStartTime, uint32_t MsgSize, uint16_t MsgId)
{
  uint64_t now = Simulator::Now ().GetNanoSeconds ();
  uint64_t msg_start_time = MsgStartTime.GetNanoSeconds();
  uint64_t fct = now - msg_start_time;

  zero_times--;

  // sip, dip, sport, dport, size (B), start_time, fct (ns), 
  std::cout << "fct "  <<Simulator::Now ().GetNanoSeconds () 
            << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
            << " " << MsgSize << " " << msg_start_time << " " << fct << " " << MsgId 
            << std::endl;

  *stream->GetStream () << "fct "  <<Simulator::Now ().GetNanoSeconds () 
            << " " << saddr << ":" << sport << " "  << daddr << ":" << dport 
            << " " << MsgSize << " " << msg_start_time << " " << fct << " " << MsgId 
            << std::endl;

  if (zero_times == 0) {
    Simulator::Stop();
  }
}

void cal_average_tp()
{
  double average_tp = all_pkt_size_count*8.0/(last_NewDataArrivalTime-flow_start_time);
  std::cout << "Average_TP_Time " << Simulator::Now ().GetNanoSeconds () 
            << " all_pkt_size_count " << all_pkt_size_count
            << " flow_start_time " << flow_start_time
            << " average_tp " << average_tp << " Gbps"
            << " last_NewDataArrivalTime " << last_NewDataArrivalTime
            << std::endl;
  std::cout << "Highest_Queue_Length ";
  for (uint16_t i = 0; i<nPort ; i++)
    std::cout << Highest_Queue_Length[i] << " ";
  std::cout << "\n";
}

/*
void cal_throughput (uint64_t schedule_tp_time)
{
  if( last_tp_interval )
    return;

  uint64_t now = Simulator::Now ().GetNanoSeconds ();
  uint64_t tp_interval = now-last_time;

  if ( tp_interval != 0)
    throughput = pkt_size_count*8/tp_interval; //Gbps
  else
    throughput = 0;

  last_time = now;
  pkt_size_count = 0;
  pkt_count = 0;

  std::cout << "TP " << Simulator::Now ().GetNanoSeconds () 
       << " all_pkt_count " << all_pkt_count
       << " all_pkt_size_count " << all_pkt_size_count
       << " TP " << throughput
       << " MsgId " << Global_MsgId
       << std::endl;
  if ( zero_times==0 )
  {
    last_tp_interval = true;
    std::cout << "end" << std::endl;
    cal_average_tp();
    Simulator::Stop();
  }
  else {
    Simulator::Schedule(NanoSeconds(tp_interval), &cal_throughput, 0);
  }
}
*/


std::map<double,int> ReadMsgSizeDist (std::string msgSizeDistFileName, double &avgMsgSizePkts)
{
  std::ifstream msgSizeDistFile;
  msgSizeDistFile.open (msgSizeDistFileName);
  NS_LOG_FUNCTION("Reading Msg Size Distribution From: " << msgSizeDistFileName);
    
  std::string line;
  std::istringstream lineBuffer;
  
  getline (msgSizeDistFile, line);
  lineBuffer.str (line);
  lineBuffer >> avgMsgSizePkts;
    
  std::map<double,int> msgSizeCDF;
  double prob;
  int msgSizePkts;
  while(getline (msgSizeDistFile, line)) 
  {
    lineBuffer.clear ();
    lineBuffer.str (line);
    lineBuffer >> msgSizePkts;
    lineBuffer >> prob;
      
    msgSizeCDF[prob] = msgSizePkts;
  }
  msgSizeDistFile.close();
    
  return msgSizeCDF;
}

int
main (int argc, char *argv[])
{
  AsciiTraceHelper asciiTraceHelper;
  double start_time = 2; // in seconds
  double duration = 5; // in seconds
  double networkLoad = 0.5;
  uint32_t simIdx = 0;
  bool traceQueues = false;
  bool disableRtx = false;
  bool debugMode = false;
  // uint32_t initialCredit = 7; // in packets MTU=1500  BW=100Gbps:RTT=4us = BDP=50KB:Credit=34
  // uint32_t initialCredit = 34; // 配置
  // uint32_t initialCredit = 48;
  uint32_t initialCredit = 96; // bdp = 100KB = 100Gbps*8us

  uint64_t inboundRtxTimeout = 1000; // in microseconds ori:1000
  uint64_t outboundRtxTimeout = 1000000; // in microseconds  ori:10000
  uint64_t tp_interval = 2000; //in nanoseconds 配置
  uint64_t qlen_interval = 2000; // nanoseconds
  uint64_t active_interval = 2000;
  uint8_t m_OvercommitLevel = 6; //配置
  uint8_t numTotalPrioBands = 8;
  std::string flow_file = "";
  std::string m_traceFileName = "logs/log";
  // std::string m_flow_file = "load_files/tencent-cbs-CBS-2-load-1.txt";
  std::string m_flow_file = "";
  // std::string m_flow_file = "./flow_file_test";
  
  CommandLine cmd (__FILE__);
  cmd.AddValue ("duration", "The duration of the simulation in seconds.", duration);
  cmd.AddValue ("load", "The network load to simulate the network at, ie 0.5 for 50%.", networkLoad);
  cmd.AddValue ("simIdx", "The index of the simulation used to identify parallel runs.", simIdx);
  cmd.AddValue ("traceQueues", "Whether to trace the queue lengths during the simulation.", traceQueues);
  cmd.AddValue ("disableRtx", "Whether to disable rtx timers during the simulation.", disableRtx);
  cmd.AddValue ("debug", "Whether to enable detailed pkt traces for debugging", debugMode);
  cmd.AddValue ("bdpPkts", "RttBytes to use in the simulation.", initialCredit);
  cmd.AddValue ("inboundRtxTimeout", "Number of microseconds before an inbound msg expires.", inboundRtxTimeout);
  cmd.AddValue ("outboundRtxTimeout", "Number of microseconds before an outbound msg expires.", outboundRtxTimeout);
  cmd.AddValue ("tp_interval", "Time interval for calculating throughput rate.", tp_interval);
  cmd.AddValue ("overcommitlevel", "Minimum number of messages to Grant at the same time.", m_OvercommitLevel);
  cmd.AddValue ("totalPrioBands", "Minimum number of messages to Grant at the same time.", numTotalPrioBands);
  cmd.AddValue ("flow_file", "flow_file.", m_flow_file);
  cmd.AddValue ("trace_file", "The file name of trace output.", m_traceFileName);
  cmd.Parse (argc, argv);
    
  if (debugMode)
  {
    NS_LOG_UNCOND("Running in DEBUG Mode!");
    SeedManager::SetRun (0);
  }
  else
    SeedManager::SetRun (simIdx);
    
  Time::SetResolution (Time::NS);
  Packet::EnablePrinting ();
  LogComponentEnable ("HomaL4ProtocolPaperReproduction", LOG_LEVEL_DEBUG);  
  // LogComponentEnable ("MsgGeneratorApp", LOG_LEVEL_WARN);  
  LogComponentEnable ("HomaSocket", LOG_LEVEL_WARN);
  LogComponentEnable ("HomaL4Protocol", LOG_LEVEL_WARN);

  // LogComponentEnable ("HomaL4ProtocolPaperReproduction", LOG_LEVEL_DEBUG);  
  // LogComponentEnable ("MsgGeneratorApp", LOG_LEVEL_FUNCTION);  
  // LogComponentEnable ("HomaSocket", LOG_LEVEL_FUNCTION);
  // LogComponentEnable ("HomaL4Protocol", LOG_LEVEL_FUNCTION);
    
  std::string msgSizeDistFileName ("inputs/homa-paper-reproduction/DCTCP-MsgSizeDist.txt");
  // std::string msgSizeDistFileName ("inputs/homa-paper-reproduction/test-MsgSizeDist.txt");
  std::string tracesFileName (m_traceFileName);
  tracesFileName += "_W5";
  tracesFileName += "_load-" + std::to_string((int)(networkLoad*100)) + "p";
  if (debugMode)
    tracesFileName += "_debug";
  else
    tracesFileName += "_" + std::to_string(simIdx);

  // std::string qStreamName = "./test_switch.qlen";
  std::string qStreamName = tracesFileName + "_qlen.txt";
  std::string qlenStreamName = tracesFileName + "_qlen_detail.txt";
  std::string msgTracesFileName = tracesFileName + ".tr";
  std::string FctTracesFileName = tracesFileName + "_fct.txt";
  std::string ThputTracesFileName = tracesFileName + "_tp-detail.txt";
  std::string activeStreamName = tracesFileName + "_active.txt";
  std::string ThputSummaryTracesFileName = tracesFileName + "_tp-summary.txt";

  int nHosts = 160;
  int nTors = 10;
  int nSpines = 4;
  // int nHosts = 160;  //Highest_Queue_Length 用到nHost的数值
  // int nTors = 1;
  // int nSpines = 1;
  
  /******** Create Nodes ********/
  NS_LOG_UNCOND("Creating Nodes...");
  NodeContainer hostNodes;
  hostNodes.Create (nHosts);
    
  NodeContainer torNodes;
  torNodes.Create (nTors);
    
  NodeContainer spineNodes;
  spineNodes.Create (nSpines);
    
  /******** Create Channels ********/
  NS_LOG_UNCOND("Configuring Channels...");
  PointToPointHelper hostLinks;
  hostLinks.SetDeviceAttribute ("DataRate", StringValue ("100Gbps")); //10 
  hostLinks.SetDeviceAttribute ("Mtu", StringValue ("1044")); //t
  hostLinks.SetChannelAttribute ("Delay", StringValue ("1000ns")); //250
  hostLinks.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
    
  PointToPointHelper aggregationLinks;
  aggregationLinks.SetDeviceAttribute ("DataRate", StringValue ("400Gbps")); //40
  aggregationLinks.SetDeviceAttribute ("Mtu", StringValue ("1044")); //t
  aggregationLinks.SetChannelAttribute ("Delay", StringValue ("1000ns")); //250
  aggregationLinks.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
    
  /******** Create NetDevices ********/
  NS_LOG_UNCOND("Creating NetDevices...");
  NetDeviceContainer hostTorDevices[nHosts];
  for (int i = 0; i < nHosts; i++)
  {
    hostTorDevices[i] = hostLinks.Install (hostNodes.Get(i), 
                                           torNodes.Get(i/(nHosts/nTors)));
  }
    
  NetDeviceContainer torSpineDevices[nTors*nSpines];
  for (int i = 0; i < nTors; i++)
  {
    for (int j = 0; j < nSpines; j++)
    {
      torSpineDevices[i*nSpines+j] = aggregationLinks.Install (torNodes.Get(i), 
                                                               spineNodes.Get(j));
    }
  }
    

  /******** Install Internet Stack ********/
  NS_LOG_UNCOND("Installing Internet Stack...");
    
  /* Set default BDP value in packets */
  Config::SetDefault("ns3::HomaL4Protocol::RttPackets", 
                     UintegerValue(initialCredit));
    
  /* Set default number of priority bands in the network */

  uint8_t numUnschedPrioBands = 2;
  if (disableRtx)
  {
    inboundRtxTimeout *= 1e9;
    outboundRtxTimeout *= 1e9;
  }
  
  
  Config::SetDefault("ns3::HomaL4Protocol::OvercommitLevel", 
                     UintegerValue(m_OvercommitLevel));
  Config::SetDefault("ns3::HomaL4Protocol::Msg_tp_interval", 
                     UintegerValue(tp_interval/1000));

  NS_LOG_UNCOND("UnschedPackets " << initialCredit);
  printf("m_OvercommitLevel %u\n", m_OvercommitLevel);
  NS_LOG_UNCOND("Deploying HomaL4Protocol Stack...");
  Config::SetDefault("ns3::HomaL4Protocol::NumTotalPrioBands", 
                     UintegerValue(numTotalPrioBands));
  Config::SetDefault("ns3::HomaL4Protocol::NumUnschedPrioBands", 
                     UintegerValue(numUnschedPrioBands));
  Config::SetDefault("ns3::HomaL4Protocol::InbndRtxTimeout", 
                     TimeValue (MicroSeconds (inboundRtxTimeout)));
  Config::SetDefault("ns3::HomaL4Protocol::OutbndRtxTimeout", 
                     TimeValue (MicroSeconds (outboundRtxTimeout)));

  InternetStackHelper stack;
  stack.Install (spineNodes);
    
  /* Enable multi-path routing */
  Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", 
                     EnumValue(Ipv4GlobalRouting::ECMP_RANDOM));
    
  stack.Install (torNodes);
  stack.Install (hostNodes);
    
  /* Link traffic control configuration for Homa compatibility */
  // TODO: The paper doesn't provide buffer sizes, so we set some large 
  //       value for rare overflows.
  TrafficControlHelper tchPfifoHoma;
  tchPfifoHoma.SetRootQueueDisc ("ns3::PfifoHomaQueueDisc",
                                 "MaxSize", StringValue("1636p"), //500 配置
                                 "NumBands", UintegerValue(numTotalPrioBands));
  QueueDiscContainer hostFacingTorQdiscs[nHosts];
  Ptr<OutputStreamWrapper> qStream;
  if (traceQueues)
    qStream = asciiTraceHelper.CreateFileStream (qStreamName);
  
  for (int i = 0; i < nHosts; i++)
  {
    hostFacingTorQdiscs[i] = tchPfifoHoma.Install (hostTorDevices[i].Get(1));
    if (traceQueues)
      hostFacingTorQdiscs[i].Get(0)->TraceConnectWithoutContext ("BytesInQueue", 
                                          MakeBoundCallback (&BytesInQueueDiscTrace, 
                                                             qStream, i));
  }
  for (int i = 0; i < nTors*nSpines; i++)
  {
    tchPfifoHoma.Install (torSpineDevices[i]);
  }
   
  /* Set IP addresses of the nodes in the network */
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  std::vector<InetSocketAddress> clientAddresses;
    
  Ipv4InterfaceContainer hostTorIfs[nHosts];
  for (int i = 0; i < nHosts; i++)
  {
    hostTorIfs[i] = address.Assign (hostTorDevices[i]);
    portNumder[i] = 10000+i; //
    clientAddresses.push_back(InetSocketAddress (hostTorIfs[i].GetAddress (0), 
                                                 10000+i));
    address.NewNetwork ();
  }
  
  Ipv4InterfaceContainer torSpineIfs[nTors*nSpines];
  for (int i = 0; i < nTors*nSpines; i++)
  {
    torSpineIfs[i] = address.Assign (torSpineDevices[i]);
    address.NewNetwork ();
  }
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  for (int i = 0; i < nHosts; i++)
  {
    Ptr<Node> node = hostNodes.Get(i);
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

    int n = ipv4->GetNAddresses(1);
    for (int k = 0; k < n; k++) {
      Ipv4Address ipv4addr = ipv4->GetAddress(1, k).GetLocal();
      hostAddressList.push_back(ipv4addr);
    }
  }

    
  /******** Read the Workload Distribution From File ********/
  NS_LOG_UNCOND("Reading Msg Size Distribution...");
  double avgMsgSizePkts;
  //         prob pkts
  std::map<double,int> msgSizeCDF = ReadMsgSizeDist(msgSizeDistFileName, avgMsgSizePkts);
    
  NS_LOG_LOGIC ("The CDF of message sizes is given below: ");
  for (auto it = msgSizeCDF.begin(); it != msgSizeCDF.end(); it++)
  {
    NS_LOG_LOGIC (it->second << " : " << it->first);
  }
  NS_LOG_LOGIC("Average Message Size is: " << avgMsgSizePkts);
    
  /******** Create Message Generator Apps on End-hosts ********/
  NS_LOG_UNCOND("Installing the Applications...");
  HomaHeader homah;
  Ipv4Header ipv4h;
  m_mtu = hostTorDevices[0].Get (0)->GetMtu();
  m_header_size = homah.GetSerializedSize () + ipv4h.GetSerializedSize ();
  uint32_t payloadSize = hostTorDevices[0].Get (0)->GetMtu() 
                         - homah.GetSerializedSize ()
                         - ipv4h.GetSerializedSize ();
  printf("mtu is %u, payloadSize is %u\n", m_mtu, payloadSize);
  std::cout << m_flow_file << "_mtu" << m_mtu << "_payload" << payloadSize
            << "_initialCredit" << initialCredit
            << "_overcommit" << int(m_OvercommitLevel) << std::endl;
  Config::SetDefault("ns3::MsgGeneratorApp::PayloadSize", 
                     UintegerValue(payloadSize));
    
  // for (int i = 0; i < nHosts; i++)
  // // for (int i = 0; i < 1; i++)
  // {
  //   Ptr<MsgGeneratorApp> app = CreateObject<MsgGeneratorApp>(hostTorIfs[i].GetAddress (0),
  //                                                            1000 + i);
  //   app->Install (hostNodes.Get (i), clientAddresses);
  //   app->SetWorkload (networkLoad, msgSizeCDF, avgMsgSizePkts);
    
  //   app->Start(Seconds (start_time));
  //   app->Stop(Seconds (start_time + duration));
  // }
  
  for (int i = 0; i < nHosts; ++i) //安装好本端自己的程序
  {
    Ptr<ImiClientApp> app = CreateObject<ImiClientApp>(hostTorIfs[i].GetAddress (0),10000+i);
    app->Install(hostNodes.Get (i));
    ClientApps.push_back(app);
    app->Stop(Seconds (3.0 + duration));
  }

  NS_LOG_UNCOND("Scheduli FlowInput...");
  NS_LOG_UNCOND("Load File " << m_flow_file);
  // std::cout << "Load File " << m_flow_file;
  flowf.open(m_flow_file.c_str());
  flowf >> flow_num;
  zero_times = int(flow_num);
  flow_input.idx = 0;
  if (flow_num > 0){
    NS_LOG_UNCOND("Testing ReadFlowInput...");
    ReadFlowInput();
    NS_LOG_UNCOND("Testing ScheduleFlowInputs...");
    ScheduleFlowInputs(hostTorIfs);
    // Simulator::Schedule(Seconds(flow_input.start_time)-Simulator::Now(), ScheduleFlowInputs, hostTorIfs);
  }
  flowf.close(); //1.14新加的


  // {
  //   int i = 0;
  //   Ptr<MsgGeneratorApp> app = CreateObject<MsgGeneratorApp>(hostTorIfs[i].GetAddress (0),
  //                                                            1000 + i);
  //   app->Install (hostNodes.Get (i), clientAddresses);
  //   app->SetWorkload (networkLoad, msgSizeCDF, avgMsgSizePkts);
    
  //   app->Start(Seconds (3.0));
  //   app->Stop(Seconds (3.0 + duration));
  // }

  // {
  //   int i = 143;
  //   Ptr<MsgGeneratorApp> app = CreateObject<MsgGeneratorApp>(hostTorIfs[i].GetAddress (0),
  //                                                            1000 + i);
  //   app->Install (hostNodes.Get (i), clientAddresses);
  //   app->SetWorkload (networkLoad, msgSizeCDF, avgMsgSizePkts);
    
  //   app->Start(Seconds (1.0));
  //   app->Stop(Seconds (5.0 + duration));
  // }

  /* Set the message traces for the Homa clients*/
  Ptr<OutputStreamWrapper> msgStream;
  msgStream = asciiTraceHelper.CreateFileStream (msgTracesFileName);
  Config::ConnectWithoutContext("/NodeList/*/$ns3::HomaL4Protocol/MsgBegin", 
                                MakeBoundCallback(&TraceMsgBegin, msgStream));
  Config::ConnectWithoutContext("/NodeList/*/$ns3::HomaL4Protocol/MsgFinish", 
                                MakeBoundCallback(&TraceMsgFinish, msgStream));
  

  Ptr<OutputStreamWrapper> FctStream;
  FctStream = asciiTraceHelper.CreateFileStream (FctTracesFileName);
  Config::ConnectWithoutContext("/NodeList/*/$ns3::HomaL4Protocol/MsgComplete", 
                                MakeBoundCallback(&cal_fct, FctStream));



  // 计算吞吐
  throughput_summary = asciiTraceHelper.CreateFileStream (ThputSummaryTracesFileName);
  Config::ConnectWithoutContext("/NodeList/*/$ns3::HomaL4Protocol/DataPktArrival", 
                                MakeBoundCallback(&TraceDataPktArrival,msgStream));
  // Config::ConnectWithoutContext("/NodeList/*/$ns3::HomaL4Protocol/CtrlPktArrival", 
  //                               MakeBoundCallback(&TraceCtrlPktArrival,msgStream));


  // Simulator::Schedule(NanoSeconds(start_time), &cal_throughput, 0);


  
  if (debugMode)
  {
    Ptr<OutputStreamWrapper> pktStream;
    std::string pktTraceFileName ("outputs/homa-paper-reproduction/debug-pktTrace.tr"); 
    pktStream = asciiTraceHelper.CreateFileStream (pktTraceFileName);
      
    Config::ConnectWithoutContext("/NodeList/45/$ns3::HomaL4Protocol/DataPktDeparture", 
                                MakeBoundCallback(&TraceDataPktDeparture,pktStream));
    // Config::ConnectWithoutContext("/NodeList/45/$ns3::HomaL4Protocol/DataPktArrival", 
    //                             MakeBoundCallback(&TraceDataPktArrival,pktStream));
    Config::ConnectWithoutContext("/NodeList/45/$ns3::HomaL4Protocol/CtrlPktArrival", 
                                MakeBoundCallback(&TraceCtrlPktArrival,pktStream));
  
//     std::string pcapFileName ("outputs/homa-paper-reproduction/pcaps/tor-spine");
//     aggregationLinks.EnablePcapAll (pcapFileName, false);
  }
  Ptr<OutputStreamWrapper> ThputStream, QlenStream, ActiveStream;
  ThputStream = asciiTraceHelper.CreateFileStream (ThputTracesFileName);
  QlenStream = asciiTraceHelper.CreateFileStream (qlenStreamName);
  ActiveStream = asciiTraceHelper.CreateFileStream (activeStreamName);

  Simulator::Schedule(Seconds(start_time), &CalculateThroughputByRx, ThputStream, tp_interval);
  Simulator::Schedule(Seconds(start_time), &TraceRxQlenByRx, QlenStream, qlen_interval, (QueueDiscContainer*) hostFacingTorQdiscs, nHosts);
  Simulator::Schedule(Seconds(start_time), &TraceActive, ActiveStream, active_interval);
  Simulator::Schedule(Seconds(2+duration), &cal_average_tp); 
  /******** Run the Actual Simulation ********/
  NS_LOG_UNCOND("Running the Simulation...");
  Simulator::Stop(Seconds(2+duration));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}