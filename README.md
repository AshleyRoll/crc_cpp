
[![C++ CMake CI](https://github.com/AshleyRoll/crc_cpp/actions/workflows/build_cmake.yml/badge.svg)](https://github.com/AshleyRoll/crc_cpp/actions/workflows/build_cmake.yml)
[![CodeQL](https://github.com/AshleyRoll/crc_cpp/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/AshleyRoll/crc_cpp/actions/workflows/codeql-analysis.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/02c708fb7c554faabfccbaf04bfe5c14)](https://www.codacy.com/gh/AshleyRoll/crc_cpp/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=AshleyRoll/crc_cpp&amp;utm_campaign=Badge_Grade)

# `crc_cpp`: CRC Routines

A Header only, `constexpr` / compile time small-table based CRC library.

Currently only supporting:
-   C++20 and above
-   C++17 using a fall back static table generator pattern

Unfortunately older C++ standards will not allow the complex statements in
`constexpr` functions.

This library implements most of the common CRC checks from 8 to 64 bits.

The calculation is performed using a compile time generated lookup table with
only 16 entries (by default) to minimise memory footprint - this is designed
for embedded systems where RAM and program memory are at a premium.

If you are able to provide data to the CRC at compile time, the entire CRC is
able to be computed at compile time.

## Conan

This library is available through Conan.

[crc_cpp on Conan.io](https://conan.io/center/crc_cpp)


## Usage

Simply create an instance of the correct algorithm type, and continually
`update()` it with bytes. Call `final()` to get the CRC result.

If you wish to reuse the crc object, you can call `reset()` to initialise it
again.

```cpp

#include "crc_cpp.h"

[[nodiscard]] constexpr auto compute_crc16ccit(std::vector<char> const &message)
{
    crc_cpp::crc16_ccit crc;

    for(auto c : message) {
        crc.update(c);
    }

    return crc.final();
}

int main()
{
    std::vector<char> const message{'1', '2', '3', '4', '5', '6', '7', '8', '9'};

    return 0x29B1 == compute_crc16ccit(message);
}

```
[![Compiler Explorer](https://godbolt.org/favicon.ico) Try it on Compiler Explorer](https://godbolt.org/z/6Wc17zfze)

For each different CRC you use in the program, you will get a new table
generated as they all will contain different values.

### Trading size and speed

By default the implementation will select the `small` implementation, a
4-bit (16 entry table) and process each byte value as 2 nibbles.

However the crc_cpp library also supports a `tiny` and `large` implementation.

| Implementation      | Bits | Table Size  |
| ------------------- | ---- | ----------  |
| `crc_cpp::tiny::*`  | 2    | 4 entries   |
| `crc_cpp::small::*` | 4    | 16 entries  |
| `crc_cpp::large::*` | 8    | 256 entries |

Where the `*` is replaced with the name of the CRC algorithm. See
[inlcude/crc_cpp.h](include/crc_cpp.h) for the full list of supported
algorithms.

The bits column indicates how many bits of each byte is processed in a single
step. 8 bits means the entire bytes is used in a single step. 4 bits means that
we process a nibble (4 bits) at a time, and so on.

Each entry in the table is the size of the CRC register (8, 16, 32 or 64 bits).

### Define your own

If you have a CRC algorithm that isn't already baked in, feel free to define
your own:

```cpp

// Don't use this, seriously.. This is not a real polynomial :)
class mycrc : public crc_cpp::impl::crc<
              //                           size,     polynomial, initial,    final xor,  reverse?
              crc_cpp::impl::crc_algorithm<uint32_t, 0xDEADBEEF, 0x00C0DE00, 0x00000000, false>,
              // Select table size
              crc_cpp::table_size::small
              >{};
```

Then just use it like any other one.

If the reverse parameter is `false`, the bits are rotated through the register
MSB to LSB (rotate left). If it is `true` the reverse happens and bits are shifted through
LSB to MSB (rotate right).

## Supported CRC Algorithms

### 8 Bit

-   crc8
-   crc8_cdma2000
-   crc8_darc
-   crc8_dvbs2
-   crc8_ebu
-   crc8_icode
-   crc8_itu
-   crc8_maxim
-   crc8_rohc
-   crc8_wcdma

### 16 Bit

-   crc16_ccit
-   crc16_arc
-   crc16_augccit
-   crc16_buypass
-   crc16_cdma2000
-   crc16_dds110
-   crc16_dectr
-   crc16_dectx
-   crc16_dnp
-   crc16_en13757
-   crc16_genibus
-   crc16_maxim
-   crc16_mcrf4xx
-   crc16_riello
-   crc16_t10dif
-   crc16_teledisk
-   crc16_tms37157
-   crc16_usb
-   crc16_a
-   crc16_kermit
-   crc16_modbus
-   crc16_x25
-   crc16_xmodem

### 32 Bit

-   crc32
-   crc32_bzip2
-   crc32_c
-   crc32_d
-   crc32_mpeg2
-   crc32_posix
-   crc32_q
-   crc32_jamcrc
-   crc32_xfer

### 64 Bit

-   crc64_ecma

## Limitations

Support is only provided for CRC algorithms with a register size of 8, 16, 32
or 64 bits.

## Future

Things to work on in the future:
-   Try to back-port to older C++ standards (help appreciated)
