#pragma once

#include "byte_stream.hh"
#include <map>
#include <vector>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output )
    : output_( std::move( output ) )
    , index( 0 )
    , pending_data( output.writer().available_capacity() )
    , window_head( 0 )
    , last_ac( output.writer().available_capacity() )
    , has_last( false )
    , intervals()
  {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  // This function is for testing only; don't add extra state to support it.
  uint64_t count_bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;
  uint64_t index; // 指向下一个要buffer的byte(尚未buffered)，初始化为0
  std::vector<char> pending_data;
  uint64_t window_head;                        // 等于读取了多少数据
  uint64_t last_ac;                            // last available capacity, 用于更新window head
  bool has_last;                               // 当前reserve的数据中有无last_string
  std::multimap<uint64_t, uint64_t> intervals; // pendiing data的数据区间

  void insert_data( uint64_t first_index, std::string data, bool is_last_substring );
  void reserve_data( uint64_t first_index, std::string data, bool is_last_substring );
  void merge_intervals();
};
