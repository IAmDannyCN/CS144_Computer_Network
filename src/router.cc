#include "router.hh"

#include <bitset>
#include <iostream>
#include <limits>

using namespace std;

void Out_as_Binary( uint32_t u )
{
  cerr << bitset<32>( u ) << endl;
}

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  // uint32_t mask = (prefix_length == 0) ? 0 : (0xffffffffu << (32 - prefix_length));
  // Out_as_Binary(route_prefix);
  // Out_as_Binary(mask);
  // cerr << "=================================" << endl;
  _routing_table.push_back( RoutingRecord( route_prefix, prefix_length, next_hop, interface_num ) );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  for ( auto& interface : _interfaces ) {
    queue<InternetDatagram>& datagrams = interface->datagrams_received();
    while ( !datagrams.empty() ) {
      InternetDatagram datagram = move( datagrams.front() );
      datagrams.pop();

      if ( datagram.header.ttl <= 1 ) {
        continue;
      }

      uint32_t dst = datagram.header.dst;
      int matching_idx = -1;
      size_t cur_len = 0;
      for ( int i = 0; i < (int)_routing_table.size(); i++ ) {
        RoutingRecord& record = _routing_table[i];
        uint32_t len = record.prefix_length;
        uint32_t mask = ( len == 0 ) ? 0 : ( 0xffffffffu << ( 32 - len ) );
        // Out_as_Binary(record.route_prefix);
        // Out_as_Binary(mask);
        // cerr << "--------------------------------" << endl;
        // assert(record.route_prefix == (record.route_prefix & mask));
        if ( ( record.route_prefix & mask ) != ( dst & mask ) ) {
          continue;
        }
        if ( record.prefix_length >= cur_len ) {
          matching_idx = i;
          cur_len = record.prefix_length;
        }
      }

      if ( matching_idx == -1 ) {
        continue;
      }
      datagram.header.ttl -= 1;
      datagram.header.compute_checksum();

      std::optional<Address> next_hop = _routing_table[matching_idx].next_hop;
      size_t interface_num = _routing_table[matching_idx].interface_num;

      _interfaces[interface_num]->send_datagram( move( datagram ),
                                                 next_hop.value_or( Address::from_ipv4_numeric( dst ) ) );
    }
  }
}