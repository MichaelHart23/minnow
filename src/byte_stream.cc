#include "byte_stream.hh"
#include "debug.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , buffer( capacity, 'a' )
  , pos2read( 0 )
  , pos2write( 0 )
  , is_full( false )
  , is_empty( true )
  , total_pushed( 0 )
  , total_poped( 0 )
  , writer_end( false )
  , error_(false)
{}
// Push data to stream, but only as much as available capacity allows.
uint64_t Writer::push( string data )
{
  if(is_closed()) return 0;
  uint64_t size_to_be_written = available_capacity();

  if ( size_to_be_written > data.size() ) {
    size_to_be_written = data.size();
  } else
    is_full = true; // 剩余容量会全被占满，提前设置is_full，否则available_capacity()不能正确工作
  // 若是要写入的数据大于剩余容量，做截断处理

  for ( uint64_t i = 0; i < size_to_be_written; i++ ) {
    buffer[pos2write] = data[i];
    pos2write = ( pos2write + 1 ) % capacity_;
  }
  total_pushed += size_to_be_written;
  if ( data.size() != 0 )
    is_empty = false;
  return size_to_be_written;
}

// Signal that the stream has reached its ending. Nothing more will be written.
void Writer::close()
{
  writer_end = true;
}

// Has the stream been closed?
bool Writer::is_closed() const
{
  return writer_end;
}

// How many bytes can be pushed to the stream right now?
uint64_t Writer::available_capacity() const
{
  if ( pos2read < pos2write ) {
    return capacity_ - ( pos2write - pos2read );
  } else if ( pos2read > pos2write ) {
    return pos2read - pos2write;
  } else {
    if ( is_full )
      return 0;
    if ( is_empty )
      return capacity_;
  }
  return UINT64_MAX; // 不会被执行
}

// Total number of bytes cumulatively pushed to the stream
uint64_t Writer::bytes_pushed() const
{
  return total_pushed;
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
  if ( is_empty )
    return "";
  std::string_view sv {};
  if ( pos2write <= pos2read )
    sv = std::string_view( buffer.data() + pos2read, capacity_ - pos2read );
  else
    sv = std::string_view( buffer.data() + pos2read, pos2write - pos2read );
  return sv;
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  uint64_t size_to_be_poped = bytes_buffered();
  if ( size_to_be_poped > len )
    size_to_be_poped = len;
  else
    is_empty = true; // 所有数据被全部pop，提前设置is_empty，否则bytes_buffered()不会正常运行
  pos2read = ( pos2read + size_to_be_poped ) % capacity_;
  total_poped += size_to_be_poped;
  if ( len > 0 )
    is_full = false;
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  if ( writer_end && is_empty ) { // 不会再写且读完了
    return true;
  }
  return false;
}
// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
  if ( pos2read < pos2write ) {
    return pos2write - pos2read;
  } else if ( pos2read > pos2write ) {
    return capacity_ - ( pos2read - pos2write );
  } else {
    if ( is_full )
      return capacity_;
    if ( is_empty )
      return 0;
  }
  return UINT64_MAX; // 不会被执行
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
  return total_poped;
}

