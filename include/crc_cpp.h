/*
 * MIT License
 *
 * Copyright (c) 2020 Ashley Roll
 * https://github.com/AshleyRoll/crc_cpp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */



#include <cstdint>
#include <type_traits>
#include <array>

namespace crc_cpp
{

namespace util
{
    template<typename T> constexpr T reverse_bits(T value);

    template<> constexpr uint8_t reverse_bits(uint8_t value)
    {
        value = ((value & 0xAA) >> 1) | ((value & 0x55) << 1);
        value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2);
        value = ((value & 0xF0) >> 4) | ((value & 0x0F) << 4);
        return value;
    }

    template<> constexpr uint16_t reverse_bits(uint16_t value)
    {
        value = ((value & 0xAAAA) >> 1) | ((value & 0x5555) << 1);
        value = ((value & 0xCCCC) >> 2) | ((value & 0x3333) << 2);
        value = ((value & 0xF0F0) >> 4) | ((value & 0x0F0F) << 4);
        value = (value >> 8) | (value << 8);
        return value;
    }

    template<> constexpr uint32_t reverse_bits(uint32_t value)
    {
        value = ((value & 0xAAAAAAAA) >> 1) | ((value & 0x55555555) << 1);
        value = ((value & 0xCCCCCCCC) >> 2) | ((value & 0x33333333) << 2);
        value = ((value & 0xF0F0F0F0) >> 4) | ((value & 0x0F0F0F0F) << 4);
        value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
        value = (value >> 16) | (value << 16);
        return value;
    }

    template<> constexpr uint64_t reverse_bits(uint64_t value)
    {
        value = ((value & 0xAAAAAAAAAAAAAAAA) >>  1) | ((value & 0x5555555555555555) <<  1);
        value = ((value & 0xCCCCCCCCCCCCCCCC) >>  2) | ((value & 0x3333333333333333) <<  2);
        value = ((value & 0xF0F0F0F0F0F0F0F0) >>  4) | ((value & 0x0F0F0F0F0F0F0F0F) <<  4);
        value = ((value & 0xFF00FF00FF00FF00) >>  8) | ((value & 0x00FF00FF00FF00FF) <<  8);
        value = ((value & 0xFFFF0000FFFF0000) >> 16) | ((value & 0x0000FFFF0000FFFF) << 16);
        value = (value >> 32) | (value << 32);
        return value;
    }
}   // namespace util

namespace impl
{


    //
    // A generic CRC implementation using a lookup table sized for computing
    // a nibble (4 bits) at a time.
    //
    // The forward implementation rotates the accumulator register left
    // and clocks data in from the MSB to LSB
    //
    template <typename TAccumulator, TAccumulator POLY,
              typename = std::enable_if_t<std::is_unsigned<TAccumulator>::value>>
    class crc_nibble_table_forward
    {
    public:
        static constexpr std::size_t ACCUMULATOR_BITS = sizeof(TAccumulator) * 8;
        static constexpr std::size_t NIBBLE_BITS = 4;
        static constexpr uint8_t NIBBLE_MASK = 0x0Fu;
        static constexpr std::size_t ENTRIES = 1u << NIBBLE_BITS;

        static constexpr TAccumulator ACCUMULATOR_HIGH_BIT_MASK = TAccumulator(1u) << (ACCUMULATOR_BITS - 1);
        static constexpr TAccumulator POLYNOMIAL = POLY;
        using TableType = std::array<TAccumulator, ENTRIES>;

        static constexpr TAccumulator update(TAccumulator crc, uint8_t value)
        {
            crc = update_nibble(crc, value >> NIBBLE_BITS);     // high nibble
            crc = update_nibble(crc, value & NIBBLE_MASK);      // low nibble
            return crc;
        }

    private:
        static constexpr TAccumulator update_nibble(TAccumulator crc, uint8_t value)
        {
            // ensure we have only 4 bits
            value &= NIBBLE_MASK;

            // Extract the most significant nibble of the crc and xor with the value nibble
            uint8_t t = (crc >> (ACCUMULATOR_BITS - NIBBLE_BITS)) ^ value;

            // shit crc left the size of the nibble
            crc <<= NIBBLE_BITS;

            // xor in the table data
            crc ^= m_Table[t];

            return crc;
        }

        static constexpr TAccumulator GenerateEntry(uint8_t index)
        {
            // initialise with the register in the upper bits
            TAccumulator entry = TAccumulator(index) << (ACCUMULATOR_BITS - NIBBLE_BITS);

            for(std::size_t i = 0; i < NIBBLE_BITS; i++)
            {
                // We are processing MSBs / rotating left
                if(entry & ACCUMULATOR_HIGH_BIT_MASK) {
                    entry = (entry << 1) ^ POLYNOMIAL;
                } else {
                    entry = (entry << 1);
                }
            }
            return entry;
        }

        static constexpr TableType Generate()
        {
            TableType table;

            for(std::size_t nibble = 0; nibble < ENTRIES; ++nibble)
            {
                table[nibble] = GenerateEntry(nibble);
            }

            return table;
        }

        static constexpr TableType m_Table = Generate();
    };

    //
    // A generic CRC implementation using a lookup table sized for computing
    // a nibble (4 bits) at a time.
    //
    // The reverse implementation rotates the accumulator register right
    // and clocks data in from the LSB to MSB
    //
    template <typename TAccumulator, TAccumulator POLY,
              typename = std::enable_if_t<std::is_unsigned<TAccumulator>::value>>
    class crc_nibble_table_reverse
    {
    public:
        static constexpr std::size_t ACCUMULATOR_BITS = sizeof(TAccumulator) * 8;
        static constexpr std::size_t NIBBLE_BITS = 4;
        static constexpr uint8_t NIBBLE_MASK = 0x0Fu;
        static constexpr std::size_t ENTRIES = 1u << NIBBLE_BITS;
        static constexpr TAccumulator POLYNOMIAL = POLY;
        using TableType = std::array<TAccumulator, ENTRIES>;

        static constexpr TAccumulator update(TAccumulator crc, uint8_t value)
        {
            crc = update_nibble(crc, value & NIBBLE_MASK);      // low nibble
            crc = update_nibble(crc, value >> NIBBLE_BITS);     // high nibble
            return crc;
        }

    private:
        static constexpr TAccumulator update_nibble(TAccumulator crc, uint8_t value)
        {
            // ensure we have only 4 bits
            value &= NIBBLE_MASK;

            // Extract the least significant nibble of the crc and xor with the value nibble
            uint8_t t = (crc & NIBBLE_MASK) ^ value;

            // shit crc right the size of a nibble
            crc >>= NIBBLE_BITS;

            // xor in the table data
            crc ^= m_Table[t];

            return crc;
        }

        static constexpr TAccumulator GenerateEntry(uint8_t index)
        {
            // initialise with the register in the lower bits
            TAccumulator entry = TAccumulator(index);

            for(std::size_t i = 0; i < NIBBLE_BITS; i++)
            {
                // we are processing LSBs/rotating right
                if(entry & 0x1u) {
                    entry = (entry >> 1) ^ util::reverse_bits(POLYNOMIAL);
                } else {
                    entry = (entry >> 1);
                }
            }
            return entry;
        }

        static constexpr TableType Generate()
        {
            TableType table;

            for(std::size_t nibble = 0; nibble < ENTRIES; ++nibble)
            {
                table[nibble] = GenerateEntry(nibble);
            }

            return table;
        }

        static constexpr TableType m_Table = Generate();
    };

    template <
        typename TAccumulator,
        const TAccumulator POLY,
        const TAccumulator INITIAL,
        const TAccumulator XOR_OUT,
        const bool REVERSE>
    class crc
    {
        public:
            using AccumulatorType = TAccumulator;
            using TableType = typename
                std::conditional<REVERSE,
                    crc_nibble_table_reverse<TAccumulator, POLY>,
                    crc_nibble_table_forward<TAccumulator, POLY>>::type;

            void init() { m_Crc = INITIAL; }
            TAccumulator final() { return m_Crc ^ XOR_OUT; }

            void update(uint8_t value) { m_Crc = TableType::update(m_Crc, value); }

        private:
            TAccumulator m_Crc = INITIAL;
    };
}   // namespace impl

using crc8 =            impl::crc<uint8_t, 0x07, 0x00, 0x00, false>;
using crc8_rohc =       impl::crc<uint8_t, 0x07, 0xFF, 0x00, true>;

using crc16_ccit =      impl::crc<uint16_t, 0x1021, 0xFFFF, 0x0000, false>;
using crc16_kermit =    impl::crc<uint16_t, 0x1021, 0x0000, 0x0000, true>;

using crc32 =           impl::crc<uint32_t, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true>;
using crc32_posix =     impl::crc<uint32_t, 0x04C11DB7, 0x00000000, 0xFFFFFFFF, false>;
using crc32_xfer =      impl::crc<uint32_t, 0x000000AF, 0x00000000, 0x00000000, false>;

using crc64_ecma =      impl::crc<uint64_t, 0x42f0e1eba9ea3693, 0x0000000000000000, 0x0000000000000000, false>;


}   // namespace crc_cpp
