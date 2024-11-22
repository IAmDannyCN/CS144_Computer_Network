#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
  uint32_t next_hop_ipv4 = next_hop.ipv4_numeric();
  if ( _arp_cache.contains( next_hop_ipv4 ) ) {
    EthernetHeader msgheader;
    msgheader.dst = _arp_cache[next_hop_ipv4].first;
    msgheader.src = ethernet_address_;
    msgheader.type = EthernetHeader::TYPE_IPv4;
    EthernetFrame msg;
    msg.header = move( msgheader );
    msg.payload = serialize( dgram );
    transmit( move( msg ) );
    return;
  }
  _dgram_waiting[next_hop_ipv4].push_back( move( dgram ) );
  if ( !_arping_ipv4.contains( next_hop_ipv4 ) ) {
    EthernetHeader msgheader;
    msgheader.dst = ETHERNET_BROADCAST;
    msgheader.src = ethernet_address_;
    msgheader.type = EthernetHeader::TYPE_ARP;
    ARPMessage arpmsg;
    arpmsg.opcode = ARPMessage::OPCODE_REQUEST;
    arpmsg.sender_ethernet_address = ethernet_address_;
    arpmsg.sender_ip_address = ip_address_.ipv4_numeric();
    arpmsg.target_ip_address = next_hop_ipv4;
    EthernetFrame msg;
    msg.header = move( msgheader );
    msg.payload = serialize( move( arpmsg ) );
    transmit( move( msg ) );
    _arping_ipv4[next_hop_ipv4] = _clk._time;
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      datagrams_received_.push( move( dgram ) );
    }
    return;
  }
  assert( frame.header.type == EthernetHeader::TYPE_ARP );
  ARPMessage arpmsg;
  if ( !parse( arpmsg, frame.payload ) ) {
    return;
  }

  _arp_cache[arpmsg.sender_ip_address] = make_pair( arpmsg.sender_ethernet_address, _clk._time );

  if ( arpmsg.target_ip_address == ip_address_.ipv4_numeric() && arpmsg.opcode == ARPMessage::OPCODE_REQUEST ) {
    EthernetHeader reply_msgheader;
    reply_msgheader.dst = arpmsg.sender_ethernet_address;
    reply_msgheader.src = ethernet_address_;
    reply_msgheader.type = EthernetHeader::TYPE_ARP;
    ARPMessage reply_arpmsg;
    reply_arpmsg.opcode = ARPMessage::OPCODE_REPLY;
    reply_arpmsg.sender_ethernet_address = ethernet_address_;
    reply_arpmsg.sender_ip_address = ip_address_.ipv4_numeric();
    reply_arpmsg.target_ethernet_address = arpmsg.sender_ethernet_address;
    reply_arpmsg.target_ip_address = arpmsg.sender_ip_address;
    EthernetFrame reply_msg;
    reply_msg.header = move( reply_msgheader );
    reply_msg.payload = serialize( move( reply_arpmsg ) );
    transmit( move( reply_msg ) );
  }
  if ( _dgram_waiting.contains( arpmsg.sender_ip_address ) ) {
    for ( auto& dgram : _dgram_waiting[arpmsg.sender_ip_address] ) {
      EthernetHeader reply_msgheader;
      reply_msgheader.dst = arpmsg.sender_ethernet_address;
      reply_msgheader.src = ethernet_address_;
      reply_msgheader.type = EthernetHeader::TYPE_IPv4;
      EthernetFrame reply_msg;
      reply_msg.header = move( reply_msgheader );
      reply_msg.payload = serialize( dgram );
      transmit( move( reply_msg ) );
    }
    _dgram_waiting.erase( arpmsg.sender_ip_address );
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  _clk.tick( ms_since_last_tick );
  for ( auto it = _arp_cache.begin(); it != _arp_cache.end(); ) {
    if ( _clk.expired( it->second.second, 30000 ) ) {
      it = _arp_cache.erase( it );
    } else {
      ++it;
    }
  }
  for ( auto it = _arping_ipv4.begin(); it != _arping_ipv4.end(); ) {
    if ( _clk.expired( it->second, 5000 ) ) {
      it = _arping_ipv4.erase( it );
    } else {
      ++it;
    }
  }
}
