#include "byte_stream.hh"
#include "debug.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , buffer( capacity, 'a' )
{}
// Push data to stream, but only as much as available capacity allows.
uint64_t Writer::push( string data )
{
  if ( is_closed() ) return 0;
  uint64_t size_to_be_written = available_capacity() >= data.size() ? data.size() : available_capacity();// 若是要写入的数据大于剩余容量，做截断处理
  for ( uint64_t i = 0; i < size_to_be_written; i++ ) {
    buffer[num_write++ % capacity_] = data[i];
  }
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
  return capacity_ - (num_write - num_read);
}

// Total number of bytes cumulatively pushed to the stream
uint64_t Writer::bytes_pushed() const
{
  return num_write;
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
  if(num_write == num_read) 
    return "";
  std::string_view sv {};
  uint64_t pos2write = num_write % capacity_, pos2read = num_read % capacity_;
  if ( pos2write <= pos2read )
    sv = std::string_view( buffer.data() + pos2read, capacity_ - pos2read );
  else
    sv = std::string_view( buffer.data() + pos2read, pos2write - pos2read );
  return sv;
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  uint64_t size_to_be_poped = bytes_buffered() >= len ? len : bytes_buffered();
  num_read += size_to_be_poped;
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  return writer_end && num_write == num_read;  // 不会再写且读完了
}
// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
    return num_write - num_read;
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
  return num_read;
}
