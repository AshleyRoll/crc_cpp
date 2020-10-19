#ifndef CRC_CPP_IMPL_TABLE_GEN_H_INCLUDED
#define CRC_CPP_IMPL_TABLE_GEN_H_INCLUDED
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

#include "reverse_bits.h"

namespace crc_cpp
{

namespace impl
{
    //
    // Define the basic traits we use in our CRC accumulators and Table Lookup
    //
    template <typename TAccumulator, const table_size TABLE_SIZE>
    struct crc_traits {};

    template <typename TAccumulator>
    struct crc_traits<TAccumulator, table_size::tiny>
    {
        static constexpr std::size_t ACCUMULATOR_BITS = sizeof(TAccumulator) * 8;
        static constexpr std::size_t CHUNK_BITS = 2;
        static constexpr uint8_t CHUNK_MASK = 0x03u;
        static constexpr std::size_t TABLE_ENTRIES = 1u << CHUNK_BITS;

        using table_type = std::array<TAccumulator, TABLE_ENTRIES>;
    };

    template <typename TAccumulator>
    struct crc_traits<TAccumulator, table_size::small>
    {
        static constexpr std::size_t ACCUMULATOR_BITS = sizeof(TAccumulator) * 8;
        static constexpr std::size_t CHUNK_BITS = 4;
        static constexpr uint8_t CHUNK_MASK = 0x0Fu;
        static constexpr std::size_t TABLE_ENTRIES = 1u << CHUNK_BITS;

        using table_type = std::array<TAccumulator, TABLE_ENTRIES>;
    };

    template <typename TAccumulator>
    struct crc_traits<TAccumulator, table_size::large>
    {
        static constexpr std::size_t ACCUMULATOR_BITS = sizeof(TAccumulator) * 8;
        static constexpr std::size_t CHUNK_BITS = 8;
        static constexpr uint8_t CHUNK_MASK = 0xFFu;
        static constexpr std::size_t TABLE_ENTRIES = 1u << CHUNK_BITS;

        using table_type = std::array<TAccumulator, TABLE_ENTRIES>;
    };

    //
    // CRC rotation policies.
    //
    // Forward rotation means that we clock in data MSB->LSB and rotate the Accumulator register left
    // Reverse rotation means that we clock in data LSB->MSB and rotate the Accumulator register right
    //

    template <typename TAccumulator, const table_size TABLE_SIZE>
    struct crc_forward_policy
    {
        using traits = crc_traits<TAccumulator, TABLE_SIZE>;


        [[nodiscard]] static constexpr TAccumulator update_impl_tiny(
                TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            crc = update_chunk(crc, (value >> (traits::CHUNK_BITS*3)) & traits::CHUNK_MASK, table);     // high chunk
            crc = update_chunk(crc, (value >> (traits::CHUNK_BITS*2)) & traits::CHUNK_MASK, table);
            crc = update_chunk(crc, (value >> (traits::CHUNK_BITS*1)) & traits::CHUNK_MASK, table);
            crc = update_chunk(crc, value & traits::CHUNK_MASK, table);                                 // low chunk
            return crc;
        }

        [[nodiscard]] static constexpr TAccumulator update_impl_small(
                TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            crc = update_chunk(crc, value >> traits::CHUNK_BITS, table);     // high nibble
            crc = update_chunk(crc, value & traits::CHUNK_MASK, table);      // low nibble
            return crc;
        }

        [[nodiscard]] static constexpr TAccumulator update_impl_large(
                TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            crc = update_chunk(crc, value, table); // full byte
            return crc;
        }

        [[nodiscard]] static constexpr TAccumulator update_chunk(
                TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            // ensure we have only 4 bits
            value &= traits::CHUNK_MASK;

            // Extract the most significant nibble of the crc and xor with the value nibble
            uint8_t t = (crc >> (traits::ACCUMULATOR_BITS - traits::CHUNK_BITS)) ^ value;

            // special case for when chunk size is same as accumulator size.
            if constexpr(traits::ACCUMULATOR_BITS > traits::CHUNK_BITS) {
                // shit crc left the size of the nibble
                crc <<= traits::CHUNK_BITS;

                // xor in the table data
                crc ^= table[t];
            } else {
                crc = table[t];
            }

            return crc;
        }

        [[nodiscard]] static constexpr TAccumulator generate_entry(TAccumulator const polynomial, uint8_t const index)
        {
            // initialise with the register in the upper bits
            TAccumulator entry = TAccumulator(index) << (traits::ACCUMULATOR_BITS - traits::CHUNK_BITS);

            for(std::size_t i = 0; i < traits::CHUNK_BITS; i++)
            {
                // We are processing MSBs / rotating left so we need to check the high bit
                if(entry & (TAccumulator(1u) << (traits::ACCUMULATOR_BITS - 1))) {
                    entry = (entry << 1) ^ polynomial;
                } else {
                    entry = (entry << 1);
                }
            }

            return entry;
        }

        [[nodiscard]] static constexpr TAccumulator make_initial_value(TAccumulator init)
        {
            return init;
        }
    };

    template <typename TAccumulator, const table_size TABLE_SIZE>
    struct crc_reverse_policy
    {
        using traits = crc_traits<TAccumulator, TABLE_SIZE>;


        [[nodiscard]] static constexpr TAccumulator update_impl_tiny(
                TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            crc = update_chunk(crc, value & traits::CHUNK_MASK, table);                                 // low chunk
            crc = update_chunk(crc, (value >> (traits::CHUNK_BITS*1)) & traits::CHUNK_MASK, table);
            crc = update_chunk(crc, (value >> (traits::CHUNK_BITS*2)) & traits::CHUNK_MASK, table);
            crc = update_chunk(crc, (value >> (traits::CHUNK_BITS*3)) & traits::CHUNK_MASK, table);     // high chunk
            return crc;
        }

        [[nodiscard]] static constexpr TAccumulator update_impl_small(
                TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            crc = update_chunk(crc, value & traits::CHUNK_MASK, table);      // low nibble
            crc = update_chunk(crc, value >> traits::CHUNK_BITS, table);     // high nibble
            return crc;
        }

        [[nodiscard]] static constexpr TAccumulator update_impl_large(
                TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            crc = update_chunk(crc, value, table); // full byte
            return crc;
        }

        [[nodiscard]] static constexpr TAccumulator update_chunk(
                TAccumulator crc, uint8_t value, typename traits::table_type const &table)
        {
            // ensure we have only 4 bits
            value &= traits::CHUNK_MASK;

            // Extract the least significant nibble of the crc and xor with the value nibble
            uint8_t t = (crc & traits::CHUNK_MASK) ^ value;

            // special case for when chunk size is same as accumulator size.
            if constexpr(traits::ACCUMULATOR_BITS > traits::CHUNK_BITS) {
                // shit crc right the size of a nibble
                crc >>= traits::CHUNK_BITS;

                // xor in the table data
                crc ^= table[t];
            } else {
                crc = table[t];
            }

            return crc;
        }

        [[nodiscard]] static constexpr TAccumulator generate_entry(TAccumulator const polynomial, uint8_t const index)
        {
            // initialise with the register in the lower bits
            TAccumulator entry = TAccumulator(index);

            for(std::size_t i = 0; i < traits::CHUNK_BITS; i++)
            {
                // we are processing LSBs/rotating right
                if(entry & 0x1u) {
                    entry = (entry >> 1) ^ util::reverse_bits(polynomial);
                } else {
                    entry = (entry >> 1);
                }
            }

            return entry;
        }

        [[nodiscard]] static constexpr TAccumulator make_initial_value(TAccumulator init)
        {
            return util::reverse_bits(init);
        }
    };


    //
    // A generic CRC lookup table sized for computing a nibble (4 bits) at a time.
    //
    // This is can be a large reduction of table space storage for embedded devices.
    //
    template <typename TAccumulator,
              TAccumulator const POLYNOMIAL,
              bool const REVERSE,
              table_size const TABLE_SIZE,
              typename = std::enable_if_t<std::is_unsigned<TAccumulator>::value>>
    class crc_chunk_table
    {
    public:
        using policy = typename std::conditional<REVERSE,
                crc_reverse_policy<TAccumulator, TABLE_SIZE>,
                crc_forward_policy<TAccumulator, TABLE_SIZE>>::type;

        using traits = typename policy::traits;


        // update the given crc accumulator with the value
        [[nodiscard]] static constexpr TAccumulator update(TAccumulator crc, uint8_t value)
        {
            if constexpr(TABLE_SIZE == table_size::tiny) {
                return policy::update_impl_tiny(crc, value, m_Table);
            } else if constexpr(TABLE_SIZE == table_size::small) {
                return policy::update_impl_small(crc, value, m_Table);
            } else if constexpr(TABLE_SIZE == table_size::large) {
                return policy::update_impl_large(crc, value, m_Table);
            } else {
                static_assert("Unknown table_size");
            }
        }

        // the crc accumulator initial value may need to be modified by the policy
        // to account for rotation direction
        [[nodiscard]] static constexpr TAccumulator make_initial_value(TAccumulator init)
        {
            return policy::make_initial_value(init);
        }

    private:

// If we are C++20 or above, we can leverage cleaner constexpr initialisation
// otherwise we will attempt to use a static table builder metaprogramming pattern
// NOTE: Only C++17 will work, other constexpr code prevents C++14 and below working.
#if __cplusplus >= 202002L
        [[nodiscard]] static constexpr typename traits::table_type Generate()
        {
            typename traits::table_type table;

            for(std::size_t nibble = 0; nibble < traits::TABLE_ENTRIES; ++nibble)
            {
                table[nibble] = policy::generate_entry(POLYNOMIAL, nibble);
            }

            return table;
        }

        static constexpr typename traits::table_type m_Table = Generate();
#else
        // table builder for C++17

        // recursive case
        template<uint16_t INDEX = 0, TAccumulator ...D>
        struct table_builder : table_builder<INDEX+1, D..., policy::generate_entry(POLYNOMIAL, INDEX)> {};

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
    // Define the CRC algorithm parameters
    //
    template <
        typename TAccumulator,
        const TAccumulator POLYNOMIAL,
        const TAccumulator INITIAL,
        const TAccumulator XOR_OUT,
        const bool REVERSE>
    struct crc_algorithm
    {
        using accumulator_type = TAccumulator;
        static constexpr TAccumulator polynomial = POLYNOMIAL;
        static constexpr TAccumulator initial_value = INITIAL;
        static constexpr TAccumulator xor_out_value = XOR_OUT;
        static constexpr bool reverse = REVERSE;
    };

    //
    // The generic CRC accumulator that is table driven
    //
    template <typename TAlgorithm, const table_size TABLE_SIZE>
    class crc
    {
        public:
            using algorithm = TAlgorithm;
            using accumulator_type = typename algorithm::accumulator_type;

            //
            // Update the accumulator with a new byte
            //
            void update(uint8_t value) { m_Crc = table_impl::update(m_Crc, value); }

            //
            // Extract the final value of the accumulator.
            //
            [[nodiscard]] accumulator_type final() { return m_Crc ^ algorithm::xor_out_value; }

            //
            // Reset the state of the accumulator back to the INITIAL value.
            //
            void reset() { m_Crc = table_impl::make_initial_value(algorithm::initial_value); }


        private:
            using table_impl = crc_chunk_table<accumulator_type, algorithm::polynomial, algorithm::reverse, TABLE_SIZE>;

            accumulator_type m_Crc = table_impl::make_initial_value(algorithm::initial_value);
    };

}   // namespace impl

}   // namespace crc_cpp


#endif //CRC_CPP_IMPL_TABLE_GEN_H_INCLUDED
