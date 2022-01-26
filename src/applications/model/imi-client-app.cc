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
 * Author: Author: xinzhang <zxzx2020@hnu.edu.cn>
 */

#include "imi-client-app.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/callback.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/double.h"

#include "ns3/udp-socket-factory.h"
#include "ns3/homa-socket-factory.h"
#include "ns3/point-to-point-net-device.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ImiClientApp");

NS_OBJECT_ENSURE_REGISTERED (ImiClientApp);
    
TypeId
ImiClientApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ImiClientApp")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddAttribute ("Protocol", "The type of protocol to use. This should be "
                    "a subclass of ns3::SocketFactory",
                    TypeIdValue (HomaSocketFactory::GetTypeId ()),
                    MakeTypeIdAccessor (&ImiClientApp::m_tid),
                    // This should check for SocketFactory as a parent
                    MakeTypeIdChecker ())
    .AddAttribute ("MaxMsg", 
                   "The total number of messages to send. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ImiClientApp::m_maxMsgs),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PayloadSize", 
                   "MTU for the network interface excluding the header sizes",
                   UintegerValue (1400),
                   MakeUintegerAccessor (&ImiClientApp::m_maxPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}
    
ImiClientApp::ImiClientApp(Ipv4Address localIp, uint16_t localPort)
  :m_socket(0), 
   m_dstSocket(Ipv4Address::GetAny (), 10000)
{
  NS_LOG_FUNCTION (this << localIp << localPort);
   
  m_localIp = localIp;
  m_localPort = localPort;
}
    
ImiClientApp::~ImiClientApp()
{
  NS_LOG_FUNCTION (this);
}
    
void ImiClientApp::Install (Ptr<Node> node)
{
  NS_LOG_FUNCTION(this << node);
   
  node->AddApplication (this);
  
  m_socket = Socket::CreateSocket (node, m_tid);
  m_socket->Bind (InetSocketAddress(m_localIp, m_localPort));
  m_socket->SetRecvCallback (MakeCallback (&ImiClientApp::ReceiveMessage, this));

}
    
void ImiClientApp::SetMsgSize (uint32_t MsgSize)
{
  m_msgSize = MsgSize;
}

void ImiClientApp::SetDst (InetSocketAddress dst_Socket)
{
  m_dstSocket = dst_Socket;
}

void ImiClientApp::SetEvent(InetSocketAddress dst_Socket, uint32_t MsgSize,double start_time)
{
  Simulator::Schedule(Seconds(start_time), &ImiClientApp::SendMessage, this, dst_Socket, MsgSize, start_time);
}

uint16_t ImiClientApp::GetPort()
{
  return m_localPort;
}
void ImiClientApp::Start (Time start)
{
  NS_LOG_FUNCTION (this);
    
  SetStartTime(start);
  DoInitialize();
}
    
void ImiClientApp::Stop (Time stop)
{
  NS_LOG_FUNCTION (this);
    
  SetStopTime(stop);
}
    
void ImiClientApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  // CancelNextEvent ();
  // chain up
  Application::DoDispose ();
}
    
void ImiClientApp::StartApplication ()
{
  NS_LOG_FUNCTION (Simulator::Now ().GetNanoSeconds () << this);
    
  // NS_ASSERT_MSG( m_dstSocket.GetPort() && m_msgSize,
  //               "ImiClientApp should be installed on a node and "
  //               "the workload should be set before starting the application!");
  // SendMessage();
}
    
void ImiClientApp::StopApplication ()
{
  NS_LOG_FUNCTION (Simulator::Now ().GetNanoSeconds () << this);
}

// void ImiClientApp::SendMessage ()
// {
//   NS_LOG_FUNCTION (Simulator::Now ().GetNanoSeconds () << this);

//   NS_LOG_LOGIC(Simulator::Now ().GetNanoSeconds () << " " 
//                << m_localIp <<  ":" << m_localPort << " "
//                << "StartApplication "
//                );

//   // InetSocketAddress receiverAddr = m_dstSocket;

//   // /* Create the message to send */
//   // Ptr<Packet> msg = Create<Packet> (m_msgSize);
//   // NS_LOG_LOGIC ("ImiClientApp {" << this << ") use a message of size: "
//   //               << m_msgSize << " Bytes.");
    
//   // int sentBytes = m_socket->SendTo (msg, 0, receiverAddr);
//   // if ( sentBytes <= 0 )
//   //   NS_LOG_ERROR (Simulator::Now ().GetNanoSeconds () << " " 
//   //                 << m_localIp <<  ":" << m_localPort << " to "
//   //                 << receiverAddr.GetIpv4() << ":" << receiverAddr.GetPort() );
// }
 
void ImiClientApp::SendMessage (InetSocketAddress dst_Socket, uint32_t MsgSize, double start_time)
{
  NS_LOG_FUNCTION (Simulator::Now ().GetNanoSeconds () << this);

  InetSocketAddress receiverAddr = dst_Socket;

  /* Create the message to send */
  Ptr<Packet> msg = Create<Packet> (MsgSize);
  NS_LOG_LOGIC ("ImiClientApp {" << this << ") use a message of size: "
                << MsgSize << " Bytes.");
    
  int sentBytes = m_socket->SendTo (msg, 0, receiverAddr);
  if ( sentBytes <= 0 )
    NS_LOG_ERROR (Simulator::Now ().GetNanoSeconds () << " " 
                  << m_localIp <<  ":" << m_localPort << " to "
                  << receiverAddr.GetIpv4() << ":" << receiverAddr.GetPort() );
  else
    NS_LOG_UNCOND (Simulator::Now ().GetNanoSeconds () << " " 
                  << m_localIp <<  ":" << m_localPort << " to "
                  << receiverAddr.GetIpv4() << ":" << receiverAddr.GetPort() << "  "
                  << MsgSize << "  Bytes " << "(" << start_time << ")" 
                  );
}
    
void ImiClientApp::ReceiveMessage (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
 
  Ptr<Packet> message;
  Address from;
  while ((message = socket->RecvFrom (from)))
  {
    NS_LOG_INFO (Simulator::Now ().GetNanoSeconds () << 
                 " client received " << message->GetSize () << " bytes from " <<
                 InetSocketAddress::ConvertFrom (from).GetIpv4 () << ":" <<
                 InetSocketAddress::ConvertFrom (from).GetPort ());
    // std::cout << Simulator::Now ().GetNanoSeconds () << "  m_localPort " << m_localPort << 
    //              " client received " << message->GetSize () << " bytes from " <<
    //              InetSocketAddress::ConvertFrom (from).GetIpv4 () << ":" <<
    //              InetSocketAddress::ConvertFrom (from).GetPort () << std::endl;
  }
}
    
} // Namespace ns3