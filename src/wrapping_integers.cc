#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>(n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  const uint64_t wrap = 1ul << 32;
  uint64_t remainder = static_cast<uint64_t>(raw_value_ - zero_point.raw_value_); //absolute sequence numbers对2^32取余后的结果
  uint64_t n = checkpoint / wrap;
  uint64_t candidate = remainder + n * wrap;
  if(candidate + wrap - checkpoint < wrap/2) {
    candidate += wrap;
  }
  else if(candidate > wrap && checkpoint - (candidate - wrap) <= wrap/2) {
    candidate -= wrap;
  }

  return candidate;
}