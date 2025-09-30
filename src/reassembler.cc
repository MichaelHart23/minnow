#include "reassembler.hh"
#include "debug.hh"
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring ) {
  bool is_buffered = insert_data(first_index, data, is_last_substring);
  if(is_last_substring && is_buffered) {
    output_.writer().close();
  }
  //防止为空，且
  while(!reserved_data.empty() && reserved_data.begin()->first <= index) {
    auto it = reserved_data.begin();
    is_buffered = insert_data(it->first, it->second.first, it->second.second);
    if(it->second.second && is_buffered) {
      output_.writer().close();
    }
    reserved_data.erase(it);
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  debug( "unimplemented count_bytes_pending() called" );
  return {};
}

bool Reassembler::insert_data(uint64_t first_index, std::string data, bool is_last_substring) {
  if(first_index > index) {
    uint64_t ac = output_.writer().available_capacity();
    if(first_index - index >= ac) return false; //没有任何部分在available_capacity内
    std::string to_be_reserved = data;
    if(first_index - index + data.size() > ac) {//看数据是否有超出available_capacity的部分
      uint64_t shift = first_index - index;
      to_be_reserved = data.substr(0, ac - shift);
    }
    reserved_data[first_index] = make_pair(to_be_reserved, is_last_substring);
    return false;
  }
  else if (first_index < index) {
    if(first_index + data.size() <= index) return false; //data全部以及被buffer过了
    uint64_t overlap = index - first_index;
    uint64_t insert_size = data.size() - overlap;
    output_.writer().push(data.substr(overlap, insert_size));
    index += insert_size;
    return false;
  } 
  else {
    //先更新index，否则在push后available_capacity就变了
    uint64_t insert_size = std::min(data.size(), output_.writer().available_capacity());//长度可能超出available_capacity
    index += insert_size;
    output_.writer().push(data);//若长度超出available_capacity，push函数内部会自己截断
    return true;
  }
  return false; //不会到这
}