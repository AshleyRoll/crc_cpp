# crc_cpp
A Header only, `constexpr` / compile time small-table based CRC library.

Currently only supporting C++20, but C++17 support will be coming soon.

This library implements most of the common CRC checks from 8 to 64 bits.

The calculation is performed using a compile time generated lookup table with
only 16 entries to minimise memory footprint - this is designed for embedded
systems. We trade a small speed penalty to reduce the space.

Rather than operate on bytes at a time like other table based CRC routines,
we use a nibble (4-bits) at a time.


