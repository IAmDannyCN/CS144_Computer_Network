#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <set>

class Ticker
{
public:
  Ticker( uint64_t initial_RTO_ms ) : is_running_( false ), RTO_ms_( initial_RTO_ms ), clock_( 0 ) {}

  void reset( uint64_t target_clock_ = 0 )
  {
    // std::cout << "ticker reset()" << std::endl;
    clock_ = target_clock_;
  }
  void set_RTO_ms( uint64_t new_RTO_ms ) { RTO_ms_ = new_RTO_ms; }
  void start()
  {
    // std::cout << "ticker start()" << std::endl;
    is_running_ = true;
    reset();
  }
  void stop()
  {
    // std::cout << "ticker stop()" << std::endl;
    is_running_ = false;
    reset();
  }
  void init( uint64_t initial_RTO_ms )
  {
    RTO_ms_ = initial_RTO_ms;
    reset();
  }
  void tick( uint64_t ms_since_last_tick )
  {
    clock_ += ms_since_last_tick;
    // std::cout << "    ticked: clock_ = " << clock_  << ", RTO_ms_ = " << RTO_ms_ << std::endl;
  }
  void exponential_backoff()
  {
    RTO_ms_ <<= 1;
    // std::cout << "    exponential backoff, RTO_ms = " << RTO_ms_ << std::endl;
  }
  bool is_running() { return is_running_; }
  bool is_expired() { return is_running() && ( clock_ >= RTO_ms_ ); }

private:
  bool is_running_;
  uint64_t RTO_ms_;
  uint64_t clock_;
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , initial_RTO_ms_( initial_RTO_ms )
    , ticker_( initial_RTO_ms )
    , SYN_sent_( false )
    , FIN_sent_( false )
    , outstanding_messages_()
    , n_outstanding_( 0 )
    , n_retransmission_( 0 )
    , window_size_( 1 )
    , next_seqno_( 0 )
    , acked_seqno_( 0 )
    , no_RTO_backoff_()
  {
    // std::cout << "initial_RTO_ms = " << initial_RTO_ms_ << std::endl;
  }

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  Ticker ticker_;
  bool SYN_sent_;
  bool FIN_sent_;

  std::deque<TCPSenderMessage> outstanding_messages_;

  int n_outstanding_;
  int n_retransmission_;

  uint16_t window_size_;
  uint64_t next_seqno_;
  uint64_t acked_seqno_;

  std::set<uint64_t> no_RTO_backoff_;
};
