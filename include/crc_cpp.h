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
    //
    // Efficient bit reversal routines
    //
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
    // Define the basic traits we use in our CRC accumulators and Table Lookup
    //
    template <typename TAccumulator>
    struct crc_traits
    {
        static constexpr std::size_t ACCUMULATOR_BITS = sizeof(TAccumulator) * 8;
        static constexpr std::size_t NIBBLE_BITS = 4;
        static constexpr uint8_t NIBBLE_MASK = 0x0Fu;
        static constexpr std::size_t TABLE_ENTRIES = 1u << NIBBLE_BITS;

        using table_type = std::array<TAccumulator, TABLE_ENTRIES>;
    };

    //
    // CRC rotation policies.
    //
    // Forward rotation means that we clock in data MSB->LSB and rotate the Accumulator register left
    // Reverse rotation means that we clock in data LSB->MSB and rotate the Accumulator register right
    //

    template <typename TAccumulator, TAccumulator POLY>
    struct crc_forward_policy
    {
        using traits = crc_traits<TAccumulator>;
        static constexpr TAccumulator POLYNOMIAL = POLY;

        static constexpr TAccumulator update(TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            crc = update_nibble(crc, value >> traits::NIBBLE_BITS, table);     // high nibble
            crc = update_nibble(crc, value & traits::NIBBLE_MASK, table);      // low nibble
            return crc;
        }

        static constexpr TAccumulator update_nibble(TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            // ensure we have only 4 bits
            value &= traits::NIBBLE_MASK;

            // Extract the most significant nibble of the crc and xor with the value nibble
            uint8_t t = (crc >> (traits::ACCUMULATOR_BITS - traits::NIBBLE_BITS)) ^ value;

            // shit crc left the size of the nibble
            crc <<= traits::NIBBLE_BITS;

            // xor in the table data
            crc ^= table[t];

            return crc;
        }

        static constexpr TAccumulator generate_entry(uint8_t index)
        {
            // initialise with the register in the upper bits
            TAccumulator entry = TAccumulator(index) << (traits::ACCUMULATOR_BITS - traits::NIBBLE_BITS);

            for(std::size_t i = 0; i < traits::NIBBLE_BITS; i++)
            {
                // We are processing MSBs / rotating left so we need to check the high bit
                if(entry & (TAccumulator(1u) << (traits::ACCUMULATOR_BITS - 1))) {
                    entry = (entry << 1) ^ POLYNOMIAL;
                } else {
                    entry = (entry << 1);
                }
            }

            return entry;
        }
    };

    template <typename TAccumulator, TAccumulator POLY>
    struct crc_reverse_policy
    {
        using traits = crc_traits<TAccumulator>;
        static constexpr TAccumulator POLYNOMIAL = POLY;

        static constexpr TAccumulator update(TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            crc = update_nibble(crc, value & traits::NIBBLE_MASK, table);      // low nibble
            crc = update_nibble(crc, value >> traits::NIBBLE_BITS, table);     // high nibble
            return crc;
        }

        static constexpr TAccumulator update_nibble(TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            // ensure we have only 4 bits
            value &= traits::NIBBLE_MASK;

            // Extract the least significant nibble of the crc and xor with the value nibble
            uint8_t t = (crc & traits::NIBBLE_MASK) ^ value;

            // shit crc right the size of a nibble
            crc >>= traits::NIBBLE_BITS;

            // xor in the table data
            crc ^= table[t];

            return crc;
        }

        static constexpr TAccumulator generate_entry(uint8_t index)
        {
            // initialise with the register in the lower bits
            TAccumulator entry = TAccumulator(index);

            for(std::size_t i = 0; i < traits::NIBBLE_BITS; i++)
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
    };


    //
    // A generic CRC lookup table sized for computing a nibble (4 bits) at a time.
    //
    // This is can be a large reduction of table space storage for embedded devices.
    //
    template <typename TAccumulator, const TAccumulator POLY, const bool REVERSE,
              typename = std::enable_if_t<std::is_unsigned<TAccumulator>::value>>
    class crc_nibble_table
    {
    public:
        using traits = crc_traits<TAccumulator>;
        using policy = typename
            std::conditional<REVERSE,
                crc_reverse_policy<TAccumulator, POLY>,
                crc_forward_policy<TAccumulator, POLY>>::type;


        static constexpr TAccumulator update(TAccumulator crc, uint8_t value)
        {
            return policy::update(crc, value, m_Table);
        }

    private:

// If we are C++20 or above, we can leverage cleaner constexpr initialisation
// otherwise we will attempt to use a static table builder metaprogramming pattern
// NOTE: Only C++17 will work, other constexpr code prevents C++14 and below working.
#if __cplusplus >= 202002L
        static constexpr typename traits::table_type Generate()
        {
            typename traits::table_type table;

            for(std::size_t nibble = 0; nibble < traits::TABLE_ENTRIES; ++nibble)
            {
                table[nibble] = policy::generate_entry(nibble);
            }

            return table;
        }

        static constexpr typename traits::table_type m_Table = Generate();
#else
        // table builder for C++17

        // recursive case
        template<uint8_t INDEX = 0, TAccumulator ...D>
        struct table_builder : table_builder<INDEX+1, D..., policy::generate_entry(INDEX)> {};

        // termination of recursion at table length
        template<TAccumulator ...D>
        struct table_builder<traits::TABLE_ENTRIES, D...>
        {
            static constexpr typename traits::table_type table = {D...};
        };

        static constexpr typename traits::table_type m_Table = table_builder<>::table;
#endif

    };

    //
    // The generic CRC accumulator that is table driven
    //
    template <
        typename TAccumulator,
        const TAccumulator POLY,
        const TAccumulator INITIAL,
        const TAccumulator XOR_OUT,
        const bool REVERSE>
    class crc
    {
        public:
            using accumulator_type = TAccumulator;
            static constexpr TAccumulator polynomial = POLY;
            static constexpr TAccumulator initial_value = INITIAL;
            static constexpr TAccumulator xor_out_value = XOR_OUT;
            static constexpr bool reverse = reverse;

            //
            // Update the accumulator with a new byte
            //
            void update(uint8_t value) { m_Crc = table_impl::update(m_Crc, value); }

            //
            // Extract the final value of the accumulator.
            //
            TAccumulator final() { return m_Crc ^ XOR_OUT; }

            //
            // Reset the state of the accumulator back to the INITIAL value.
            //
            void reset() { m_Crc = INITIAL; }


        private:
            using table_impl = crc_nibble_table<TAccumulator, POLY, REVERSE>;
            TAccumulator m_Crc = INITIAL;
    };
}   // namespace impl


//------------------------------------------------------------------------
//
// Define the set of CRC algorithms as types.
//
//------------------------------------------------------------------------



using crc8 =            impl::crc<uint8_t, 0x07, 0x00, 0x00, false>;
using crc8_rohc =       impl::crc<uint8_t, 0x07, 0xFF, 0x00, true>;

using crc16_ccit =      impl::crc<uint16_t, 0x1021, 0xFFFF, 0x0000, false>;
using crc16_kermit =    impl::crc<uint16_t, 0x1021, 0x0000, 0x0000, true>;

using crc32 =           impl::crc<uint32_t, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true>;
using crc32_posix =     impl::crc<uint32_t, 0x04C11DB7, 0x00000000, 0xFFFFFFFF, false>;
using crc32_xfer =      impl::crc<uint32_t, 0x000000AF, 0x00000000, 0x00000000, false>;

using crc64_ecma =      impl::crc<uint64_t, 0x42f0e1eba9ea3693, 0x0000000000000000, 0x0000000000000000, false>;

// TODO: Extend  with remaining types.


}   // namespace crc_cpp
