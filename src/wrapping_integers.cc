#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  // (void)n;
  // (void)zero_point;
  // return Wrap32 { 0 };
  return ( zero_point + (uint32_t)n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  // (void)zero_point;
  // (void)checkpoint;
  // return {};
  uint32_t seqno_32 = raw_value_ - zero_point.raw_value_;
  uint64_t guess1 = ( checkpoint & 0xFFFFFFFF00000000ull ) | ( seqno_32 );
  uint64_t guess2 = guess1 - ( 1ull << 32 );
  uint64_t guess3 = guess1 + ( 1ull << 32 );
  uint64_t gap1 = ( guess1 > checkpoint ) ? ( guess1 - checkpoint ) : ( checkpoint - guess1 );
  uint64_t gap2 = ( guess2 > checkpoint ) ? ( guess2 - checkpoint ) : ( checkpoint - guess2 );
  uint64_t gap3 = ( guess3 > checkpoint ) ? ( guess3 - checkpoint ) : ( checkpoint - guess3 );

  if ( gap1 <= gap2 && gap1 <= gap3 ) {
    return guess1;
  } else if ( gap2 <= gap1 && gap2 <= gap3 ) {
    return guess2;
  } else {
    return guess3;
  }
}
