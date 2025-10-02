#include "reassembler.hh"
#include "debug.hh"
#include <iterator>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring ) {
  uint64_t ac = output_.writer().available_capacity();
  window_head += ac - last_ac;
  insert_data(first_index, data, is_last_substring);
  uint64_t vector_size = pending_data.size();
  while(!intervals.empty() && intervals.begin()->first <= index) {
    auto it = intervals.begin();
    uint64_t interval_size = it->second - it->first + 1;
    std::string s(interval_size, 0);
    for(uint64_t i = 0; i < interval_size; i++) {
      s[i] = pending_data[(it->first + i) % vector_size];
    }
    insert_data(it->first, s, (std::next(it) == intervals.end() && has_last));
    intervals.erase(it);
  }
  last_ac = ac;
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t res = 0;
  for(auto& it : intervals) {
    res += it.second - it.first + 1;
  }
  return res;
}

void Reassembler::insert_data( uint64_t first_index, std::string data, bool is_last_substring ) {
  if(output_.writer().available_capacity() == 0) return;
  if(first_index < index) {
    if(first_index + data.size() <= index) return;
    uint64_t overlap = index - first_index;
    uint64_t remain_size = data.size() - overlap;
    index += output_.writer().push(data.substr(overlap, remain_size));
    if(is_last_substring) {
      output_.writer().close();
    }
  }
  else if(first_index == index) {
    bool is_within_capacity = data.size() <=  output_.writer().available_capacity();
    index += output_.writer().push(data);
    if(is_last_substring && is_within_capacity) {
      output_.writer().close();
    }
  }
  else {
    if(first_index > index + output_.writer().available_capacity()) return;
    uint64_t gap = first_index - index;
    uint64_t empty_space = output_.writer().available_capacity();
    if(gap + data.size() > empty_space) {
      uint64_t within_size = empty_space - gap;
      reserve_data(first_index, data.substr(0, within_size), false);
    }
    else {
      reserve_data(first_index, data, is_last_substring);
    }
  }
}

void Reassembler::reserve_data(uint64_t first_index, std::string data, bool is_last_substring) {
  uint64_t vector_size = pending_data.size();
  uint64_t last_index = first_index + data.size() - 1;
  for(uint64_t i = first_index; i <= last_index; i++) {
    pending_data[i % vector_size] = data[i - first_index];
  }
  intervals.insert(std::make_pair(first_index, last_index));
  has_last = is_last_substring | has_last;
  merge_intervals();
}

void Reassembler::merge_intervals() {
  for(auto it = intervals.begin(); it != intervals.end(); ) {
    auto next = std::next(it);
    if(next == intervals.end()) break;
    if(it->first == next->first) {
      it->second = std::max(it->second, next->second);
      intervals.erase(next);
    }
    else if(it->first <= next->first && it->second >= next->second) {
      intervals.erase(next);
    }
    else if(it->second >= next->first) {
      it->second = next->second;
      intervals.erase(next);
    }
    else it++;
  }
}
