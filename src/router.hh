#pragma once

#include <map>
#include <memory>
#include <optional>

#include "exception.hh"
#include "network_interface.hh"

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( notnull( "add_interface", std::move( interface ) ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return _interfaces.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  class RoutingRecord
  {
  public:
    RoutingRecord( uint32_t _route_prefix,
                   uint32_t _prefix_length,
                   std::optional<Address> _next_hop,
                   size_t _interface_num )
      : route_prefix( _route_prefix )
      , prefix_length( _prefix_length )
      , next_hop( _next_hop )
      , interface_num( _interface_num )
    {}

    uint32_t route_prefix;
    uint32_t prefix_length;
    std::optional<Address> next_hop;
    size_t interface_num;
  };
  std::vector<RoutingRecord> _routing_table {};

  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};
};
