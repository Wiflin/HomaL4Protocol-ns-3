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
 * Author: xinzhang <zxzx2020@hnu.edu.cn>
 */

#ifndef IMI_CLIENT_APP_H
#define IMI_CLIENT_APP_H

#include <map>

#include "ns3/application.h"
#include "ns3/random-variable-stream.h"
// #include "ns3/random-variable-stream.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/event-id.h"

namespace ns3 {
    
class RandomVariableStream;
class InetSocketAddress;
    
/**
 * \ingroup applications 
 * \defgroup msg-generator-app ImiClientApp
 *
 * This application generates messages according 
 * to a given workload (message rate and message 
 * size) distribution. In addition to sending 
 * messages into the network, the application is 
 * also able to receive them.
 */
class ImiClientApp : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ImiClientApp (Ipv4Address localIp, uint16_t localPort=0xffff);
  
  virtual ~ImiClientApp();
  
  void Install (Ptr<Node> node);
  void SetMsgSize (uint32_t MsgSize);
  void SetDst (InetSocketAddress dst_Socket);
  void SetEvent(InetSocketAddress dst_Socket, uint32_t MsgSize,double start_time);
  uint16_t GetPort();
  void Start (Time start);
  
  void Stop (Time stop);

protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start'
  virtual void StopApplication (void);     // Called at time specified by Stop
  
  //helper
  /**
   * \brief Send a message
   */
  // void SendMessage ();
  void SendMessage (InetSocketAddress dst_Socket, uint32_t MsgSize,double start_time);
  
  /**
   * \brief Receive a message from the protocol socket
   */
  void ReceiveMessage (Ptr<Socket> socket);
  
  Ptr<Socket>       m_socket;        //!< The socket this app uses to send/receive msgs
  TypeId            m_tid;           //!< The type of the socket used

  Ipv4Address     m_localIp;         //!< Local IP address to bind 
  uint16_t        m_localPort;       //!< Local port number to bind
  InetSocketAddress m_dstSocket ; //!< client that this app can send to

  uint32_t          m_maxPayloadSize;//!< Maximum size of packet payloads
  uint32_t          m_msgSize {0};  //!<message size
  uint32_t          m_totMsgCnt;  //Count the number of msg

  //
  uint16_t          m_maxMsgs;       //!< Maximum number of messages allowed to be sent
};

} // namespace ns3
#endif