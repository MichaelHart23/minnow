#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// How many sequence numbers are outstanding?
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  uint64_t res = 0;
  for(auto &it : q) {
    res += it.sequence_length();
  }
  return res;
}

// How many consecutive retransmissions have happened?
uint64_t TCPSender::consecutive_retransmissions() const
{
  return cons_retrans;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if(push_over) return;
  std::string data;

  if(abs_seqno(ackno) == isn_.unwrap(isn_, 0ul) && abs_seqno(seqno) == isn_.unwrap(isn_, 0ul)) {//尚未连接
    read(const_cast<Reader&>(input_.reader()), window_size - 1, data); //由于SYN占一个位置，最多只能读window size - 1个字节
    uint64_t str_len = data.size();
    //Don't add FIN if this would make the segment exceed the receiver's window
    TCPSenderMessage msg(isn_, true, std::move(data), 
                         reader().is_finished() && str_len + 1 < window_size, reader().has_error());
    transmit(msg);
    if(q.empty()) timer.start(RTO);
    q.push_back(msg);
    seqno = seqno.wrap(abs_seqno(seqno) + msg.sequence_length(), isn_); //seqno加上发出的message的长度
    if(msg.FIN) push_over = true;
    return;
  }
  else if(window_size == 0) {//已连接，但没有空间了，发送长度为1的串做试探
    if(is_probing) return;
    is_probing = true;
    read(const_cast<Reader&>(input_.reader()), 1, data);
    TCPSenderMessage msg(seqno, ackno==isn_, std::move(data), reader().is_finished(), reader().has_error());
    if(msg.sequence_length() == 0) return;
    else if(msg.sequence_length() == 2) { msg.FIN = false; }
    transmit(msg);
    seqno = seqno.wrap(abs_seqno(seqno) + 1ul, isn_); //seqno += 1
    if(q.empty()) timer.start(RTO);
    q.push_back(msg);
    if(msg.FIN) push_over = true;
    return;
  }
  else { //正常
    while(true) {
      uint64_t diff = seqno.unwrap(isn_, reader().bytes_popped()) - ackno.unwrap(isn_, reader().bytes_popped());
      if(diff >= window_size) return; //已发送的segment已经超出窗口了
      uint16_t len = window_size - diff;
      len = len >= TCPConfig::MAX_PAYLOAD_SIZE ? TCPConfig::MAX_PAYLOAD_SIZE: len;
      read(const_cast<Reader&>(input_.reader()), len, data);
      uint64_t str_len = data.size();
      //Don't add FIN if this would make the segment exceed the receiver's window
      TCPSenderMessage msg(seqno, false, std::move(data), 
                           reader().is_finished() && diff + str_len < window_size, reader().has_error());
      if(msg.sequence_length() == 0) return; //读不到数据，也没有FIN
      transmit(msg);
      if(q.empty()) timer.start(RTO);
      q.push_back(msg);
      seqno = seqno.wrap(abs_seqno(seqno) + msg.sequence_length(), isn_);//seqno加上发出的message的长度
      if(msg.FIN) { //push的数据中有FIN，以后就不push了
        push_over = true;
        return;
      }
    }
    return;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return TCPSenderMessage( seqno, false, "", false, reader().has_error() );
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if(msg.RST) {
    input_.reader().set_error();
    return;
  }
  window_size = msg.window_size; //只要不出错，就更新window size
  if(msg.ackno.has_value()) {
    if(abs_seqno(ackno) >= abs_seqno(msg.ackno.value()) //收到的ackno大于之前持有的
       || abs_seqno(seqno) < abs_seqno(msg.ackno.value())) //收到的ackno不大于seqno
      return;
    ackno = msg.ackno.value();
  }
  is_probing = false;
  RTO = initial_RTO_ms_;
  timer.stop();
  cons_retrans = 0;
  uint64_t asn_ackno = ackno.unwrap(isn_, reader().bytes_popped());
  for(auto it = q.begin(); it != q.end(); ) {
    if(it->seqno.unwrap(isn_, reader().bytes_popped()) + it->sequence_length() > asn_ackno) 
      break;
    it = q.erase(it);
  }
  if(!q.empty()) timer.start(RTO);
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if(timer.accumulate(ms_since_last_tick)) {
    transmit(q.front());
    if(window_size != 0) {
      RTO *= 2;
      cons_retrans += 1;
    }
    timer.start(RTO);
  }
}

uint64_t TCPSender::abs_seqno(const Wrap32& num) {
  return num.unwrap(isn_, reader().bytes_popped() + 1);
}