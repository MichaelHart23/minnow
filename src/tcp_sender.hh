#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "retransmission_timer.hh"

#include <functional>
#include <deque>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ),
      RTO(initial_RTO_ms), ackno(isn), seqno(isn), window_size(1)
  {}

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
  uint64_t consecutive_retransmissions() const; // How many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t RTO;
  Wrap32 ackno;   //Receiver发来的确认号
  Wrap32 seqno;   //己方——Sender发送到哪了
  uint16_t window_size;
  uint64_t cons_retrans {}; //consecutive retransmissions
  uint64_t accumulated_time {};
  bool push_over {}; //FIN已被发送，之后不会再push了
  bool is_probing {}; //处于探测状态吗？处于则不再探测。当收到receiver的新回复,且ackno前进后，置为false

  std::deque<TCPSenderMessage> q {};
  RetransmissionTimer timer {};

  uint64_t abs_seqno(const Wrap32& num);
};
