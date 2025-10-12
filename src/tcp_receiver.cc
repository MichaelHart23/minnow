#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reassembler_.set_error();
    return;
  }
  if ( message.SYN && !zero_point_set ) {
    zero_point = message.seqno;
    zero_point_set = true;
  }
  if ( !zero_point_set )
    return;
  uint64_t checkpoint = reassembler_.writer().bytes_pushed() + 1;
  uint64_t asn = message.seqno.unwrap( zero_point, checkpoint );
  uint64_t stream_index = message.SYN ? 0 : asn - 1;
  reassembler_.insert( stream_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage msg;
  if ( zero_point_set ) {
    uint64_t asn_ack = reassembler_.writer().bytes_pushed() + 1;
    if ( reassembler_.writer().is_closed() ) {
      asn_ack++; // FIN占一个序号
    }
    msg.ackno = zero_point + static_cast<uint32_t>( asn_ack );
  }
  uint64_t ac = reassembler_.writer().available_capacity();
  msg.window_size = ac >= ( 1u << 16 ) ? UINT16_MAX : static_cast<uint16_t>( ac );
  msg.RST = reassembler_.writer().has_error();
  return msg;
}
