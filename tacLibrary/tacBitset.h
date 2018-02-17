#pragma once

// This is basically std::bitset

template< u32 numBits >
struct BitSet
{
  b32 GetBit( u32 index ) const
  {
    u32 byteIndex = index / 8;
    u32 bitIndex = index % 8;
    const uint8_t& bitbyte = bits[ byteIndex ];
    uint8_t mask =  1 << bitIndex;

    b32 result = bitbyte & mask;
    return result;
  }
  void SetBit( u32 index, b32 set )
  {
    u32 byteIndex = index / 8;
    u32 bitIndex = index % 8;
    uint8_t& bitbyte = bits[ byteIndex ];
    uint8_t mask = 1 << bitIndex;

    if( set )
    {
      bitbyte |= mask;
    }
    else
    {
      bitbyte &= ~mask;
    }
  }
  uint8_t bits[ ( ( numBits + 7 ) / 8 ) * 8 ];
};
