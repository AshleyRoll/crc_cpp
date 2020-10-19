![C++20 Test](https://github.com/AshleyRoll/crc_cpp/workflows/C++20%20Test/badge.svg)
![C++17 Test](https://github.com/AshleyRoll/crc_cpp/workflows/C++17%20Test/badge.svg)


# crc_cpp

A Header only, `constexpr` / compile time small-table based CRC library.

Currently only supporting:
 - C++20 and above
 - C++17 using a fall back static table generator pattern

Unfortunately older C++ standards will not allow the complex statements in
`constexpr` functions.

This library implements most of the common CRC checks from 8 to 64 bits.

The calculation is performed using a compile time generated lookup table with
only 16 entries (by default) to minimise memory footprint - this is designed
for embedded systems. We trade a small speed penalty to reduce the space.

# Usage

Simply create an instance of the correct algorithm type, and continually
`update()` it with bytes. Call `final()` to get the CRC result.

If you wish to reuse the crc object, you can call `reset()` to initialise it
again. The object

```cpp

#include "crc_cpp.h"

[[nodiscard]] auto compute_crc16ccit(std::vector<uint8_t> message)
{
    crc_cpp::crc16_ccit crc;

    for(auto c : message)
    {
        crc.update(c);
    }

    return crc.final();
}

```

For each different CRC you use in the program, you will get a new table
generated as they all will contain different values.

## Trading size and speed

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


## Define your own

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
MSG to LSB (rotate left). If it is `true` the reverse happens and bits are shifted through
LSB to MSB (rotate right).

# Limitations

Support is only provided for CRC algorithms with a register size of 8, 16, 32
or 64 bits.


# Future

- Try to back-port to older C++ standards (help appreciated)

