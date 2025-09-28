#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class Reader;
class Writer;

class ByteStream
{
public:
  explicit ByteStream( uint64_t capacity );

  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  // 这四个函数是关键：他们能让ByteStream对象被当作Writer或Reader来对待，进而能调用此两个类的成员函数了
  // 且读写两端对应的是同一块buffer。这样一个ByteStream对象就能实现这个管道功能

  // 另外定义两个类Reader和Writer的目的是把读写操作隔离分开，他们继承ByteStream是为了共享数据
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;

  void set_error() { error_ = true; };       // Signal that the stream suffered an error.
  bool has_error() const { return error_; }; // Has the stream had an error?

protected:
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.
  uint64_t capacity_;
  std::string buffer;
  uint64_t pos2read; // position to read，数据区的第一个位置, 初始化为0，和之后读完的情况统一
  uint64_t pos2write; // position to write，无数据区的第一个位置
  // 由于当写满了或读完了时，都是 pos2read == pos2write，故增加以下两个变量来区分
  bool is_full;
  bool is_empty;

  uint64_t total_pushed;
  uint64_t total_poped;

  bool writer_end; // no more writing
  bool error_ {};
};

class Writer : public ByteStream
{
public:
  void push( std::string data ); // Push data to stream, but only as much as available capacity allows.
  void close();                  // Signal that the stream has reached its ending. Nothing more will be written.

  bool is_closed() const;              // Has the stream been closed?
  uint64_t available_capacity() const; // How many bytes can be pushed to the stream right now?
  uint64_t bytes_pushed() const;       // Total number of bytes cumulatively pushed to the stream
};

class Reader : public ByteStream
{
public:
  std::string_view peek() const; // Peek at the next bytes in the buffer -- ideally as many as possible.
  void pop( uint64_t len );      // Remove `len` bytes from the buffer.

  bool is_finished() const;        // Is the stream finished (closed and fully popped)?
  uint64_t bytes_buffered() const; // Number of bytes currently buffered (pushed and not popped)
  uint64_t bytes_popped() const;   // Total number of bytes cumulatively popped from stream
};

/*
 * read: A (provided) helper function thats peeks and pops up to `max_len` bytes
 * from a ByteStream Reader into a string;
 */
void read( Reader& reader, uint64_t max_len, std::string& out );
