#ifndef CRC_CPP_H_INCLUDED
#define CRC_CPP_H_INCLUDED
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
#include <array>


//
// C++20 feature toggle
//
#if __cplusplus >= 202002L
#define CRC_CPP_STD20_MODE 1
#endif


namespace crc_cpp
{
    // Select the table size to use. This trades speed for size.
    enum class table_size
    {
        tiny,   // 4 Entries, 2 bits per chunk
        small,  // 16 Entries, 4 bits per chunk
        large   // 256 Entries, 8 bits per chunk
    };

namespace util
{
    //
    // Efficient bit reversal routines
    //
    template<typename T> constexpr T reverse_bits(T value);

    template<> [[nodiscard]] constexpr uint8_t reverse_bits(uint8_t value)
    {
        value = ((value & 0xAA) >> 1) | ((value & 0x55) << 1);
        value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2);
        value = ((value & 0xF0) >> 4) | ((value & 0x0F) << 4);
        return value;
    }

    template<> [[nodiscard]] constexpr uint16_t reverse_bits(uint16_t value)
    {
        value = ((value & 0xAAAA) >> 1) | ((value & 0x5555) << 1);
        value = ((value & 0xCCCC) >> 2) | ((value & 0x3333) << 2);
        value = ((value & 0xF0F0) >> 4) | ((value & 0x0F0F) << 4);
        value = (value >> 8) | (value << 8);
        return value;
    }

    template<> [[nodiscard]] constexpr uint32_t reverse_bits(uint32_t value)
    {
        value = ((value & 0xAAAAAAAA) >> 1) | ((value & 0x55555555) << 1);
        value = ((value & 0xCCCCCCCC) >> 2) | ((value & 0x33333333) << 2);
        value = ((value & 0xF0F0F0F0) >> 4) | ((value & 0x0F0F0F0F) << 4);
        value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
        value = (value >> 16) | (value << 16);
        return value;
    }

    template<> [[nodiscard]] constexpr uint64_t reverse_bits(uint64_t value)
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
#ifdef CRC_CPP_STD20_MODE
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
            constexpr void update(uint8_t value) { m_Crc = table_impl::update(m_Crc, value); }

            //
            // Extract the final value of the accumulator.
            //
            [[nodiscard]] constexpr accumulator_type final() { return m_Crc ^ algorithm::xor_out_value; }

            //
            // Reset the state of the accumulator back to the INITIAL value.
            //
            constexpr void reset() { m_Crc = table_impl::make_initial_value(algorithm::initial_value); }


        private:
            using table_impl = crc_chunk_table<accumulator_type, algorithm::polynomial, algorithm::reverse, TABLE_SIZE>;

            accumulator_type m_Crc = table_impl::make_initial_value(algorithm::initial_value);
    };

}   // namespace impl

namespace alg
{
    //------------------------------------------------------------------------
    //
    // Define the set of CRC algorithms
    //
    // The algorihms define the parameters for the calculation
    //
    //------------------------------------------------------------------------

    //                                          size,    poly, init, xor,  reverse
    using crc8 =            impl::crc_algorithm<uint8_t, 0x07, 0x00, 0x00, false>;
    using crc8_cdma2000 =   impl::crc_algorithm<uint8_t, 0x9B, 0xFF, 0x00, false>;
    using crc8_darc =       impl::crc_algorithm<uint8_t, 0x39, 0x00, 0x00, true>;
    using crc8_dvbs2 =      impl::crc_algorithm<uint8_t, 0xD5, 0x00, 0x00, false>;
    using crc8_ebu =        impl::crc_algorithm<uint8_t, 0x1D, 0xFF, 0x00, true>;
    using crc8_icode =      impl::crc_algorithm<uint8_t, 0x1D, 0xFD, 0x00, false>;
    using crc8_itu =        impl::crc_algorithm<uint8_t, 0x07, 0x00, 0x55, false>;
    using crc8_maxim =      impl::crc_algorithm<uint8_t, 0x31, 0x00, 0x00, true>;
    using crc8_rohc =       impl::crc_algorithm<uint8_t, 0x07, 0xFF, 0x00, true>;
    using crc8_wcdma =      impl::crc_algorithm<uint8_t, 0x9B, 0x00, 0x00, true>;

    //                                          size,     poly,   init,   xor,    reverse
    using crc16_ccit =      impl::crc_algorithm<uint16_t, 0x1021, 0xFFFF, 0x0000, false>;
    using crc16_arc =       impl::crc_algorithm<uint16_t, 0x8005, 0x0000, 0x0000, true>;
    using crc16_augccit =   impl::crc_algorithm<uint16_t, 0x1021, 0x1D0F, 0x0000, false>;
    using crc16_buypass =   impl::crc_algorithm<uint16_t, 0x8005, 0x0000, 0x0000, false>;
    using crc16_cdma2000 =  impl::crc_algorithm<uint16_t, 0xc867, 0xFFFF, 0x0000, false>;
    using crc16_dds110 =    impl::crc_algorithm<uint16_t, 0x8005, 0x800D, 0x0000, false>;
    using crc16_dectr =     impl::crc_algorithm<uint16_t, 0x0589, 0x0000, 0x0001, false>;
    using crc16_dectx =     impl::crc_algorithm<uint16_t, 0x0589, 0x0000, 0x0000, false>;
    using crc16_dnp =       impl::crc_algorithm<uint16_t, 0x3D65, 0x0000, 0xFFFF, true>;
    using crc16_en13757 =   impl::crc_algorithm<uint16_t, 0x3D65, 0x0000, 0xFFFF, false>;
    using crc16_genibus =   impl::crc_algorithm<uint16_t, 0x1021, 0xFFFF, 0xFFFF, false>;
    using crc16_maxim =     impl::crc_algorithm<uint16_t, 0x8005, 0x0000, 0xFFFF, true>;
    using crc16_mcrf4xx =   impl::crc_algorithm<uint16_t, 0x1021, 0xFFFF, 0x0000, true>;
    using crc16_riello =    impl::crc_algorithm<uint16_t, 0x1021, 0xB2AA, 0x0000, true>;
    using crc16_t10dif =    impl::crc_algorithm<uint16_t, 0x8BB7, 0x0000, 0x0000, false>;
    using crc16_teledisk =  impl::crc_algorithm<uint16_t, 0xA097, 0x0000, 0x0000, false>;
    using crc16_tms37157 =  impl::crc_algorithm<uint16_t, 0x1021, 0x89EC, 0x0000, true>;
    using crc16_usb =       impl::crc_algorithm<uint16_t, 0x8005, 0xFFFF, 0xFFFF, true>;
    using crc16_a =         impl::crc_algorithm<uint16_t, 0x1021, 0xC6C6, 0x0000, true>;
    using crc16_kermit =    impl::crc_algorithm<uint16_t, 0x1021, 0x0000, 0x0000, true>;
    using crc16_modbus =    impl::crc_algorithm<uint16_t, 0x8005, 0xFFFF, 0x0000, true>;
    using crc16_x25 =       impl::crc_algorithm<uint16_t, 0x1021, 0xFFFF, 0xFFFF, true>;
    using crc16_xmodem =    impl::crc_algorithm<uint16_t, 0x1021, 0x0000, 0x0000, false>;



    //                                          size,     poly,       init,       xor,        reverse
    using crc32 =           impl::crc_algorithm<uint32_t, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true>;
    using crc32_bzip2 =     impl::crc_algorithm<uint32_t, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, false>;
    using crc32_c =         impl::crc_algorithm<uint32_t, 0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, true>;
    using crc32_d =         impl::crc_algorithm<uint32_t, 0xA833982B, 0xFFFFFFFF, 0xFFFFFFFF, true>;
    using crc32_mpeg2 =     impl::crc_algorithm<uint32_t, 0x04C11DB7, 0xFFFFFFFF, 0x00000000, false>;
    using crc32_posix =     impl::crc_algorithm<uint32_t, 0x04C11DB7, 0x00000000, 0xFFFFFFFF, false>;
    using crc32_q =         impl::crc_algorithm<uint32_t, 0x814141AB, 0x00000000, 0x00000000, false>;
    using crc32_jamcrc =    impl::crc_algorithm<uint32_t, 0x04C11DB7, 0xFFFFFFFF, 0x00000000, true>;
    using crc32_xfer =      impl::crc_algorithm<uint32_t, 0x000000AF, 0x00000000, 0x00000000, false>;

    //                                          size,     poly,               init,               xor,                reverse
    using crc64_ecma =      impl::crc_algorithm<uint64_t, 0x42f0e1eba9ea3693, 0x0000000000000000, 0x0000000000000000, false>;

}   // namespace alg



namespace family
{
    //------------------------------------------------------------------------
    //
    // Define Algorith Families
    //
    // Each algorithm can be implemented using a table of varying sizes
    // which allows the user to trade off space and speed.
    //
    // We are creating a template that is parameterised with the size for
    // each algorithm here.
    //
    //------------------------------------------------------------------------

    template<const table_size TS> class crc8            : public impl::crc<alg::crc8, TS>{};
    template<const table_size TS> class crc8_cdma2000   : public impl::crc<alg::crc8_cdma2000, TS>{};
    template<const table_size TS> class crc8_darc       : public impl::crc<alg::crc8_darc, TS>{};
    template<const table_size TS> class crc8_dvbs2      : public impl::crc<alg::crc8_dvbs2, TS>{};
    template<const table_size TS> class crc8_ebu        : public impl::crc<alg::crc8_ebu, TS>{};
    template<const table_size TS> class crc8_icode      : public impl::crc<alg::crc8_icode, TS>{};
    template<const table_size TS> class crc8_itu        : public impl::crc<alg::crc8_itu, TS>{};
    template<const table_size TS> class crc8_maxim      : public impl::crc<alg::crc8_maxim, TS>{};
    template<const table_size TS> class crc8_rohc       : public impl::crc<alg::crc8_rohc, TS>{};
    template<const table_size TS> class crc8_wcdma      : public impl::crc<alg::crc8_wcdma, TS>{};

    template<const table_size TS> class crc16_ccit      : public impl::crc<alg::crc16_ccit, TS>{};
    template<const table_size TS> class crc16_arc       : public impl::crc<alg::crc16_arc, TS>{};
    template<const table_size TS> class crc16_augccit   : public impl::crc<alg::crc16_augccit, TS>{};
    template<const table_size TS> class crc16_buypass   : public impl::crc<alg::crc16_buypass, TS>{};
    template<const table_size TS> class crc16_cdma2000  : public impl::crc<alg::crc16_cdma2000, TS>{};
    template<const table_size TS> class crc16_dds110    : public impl::crc<alg::crc16_dds110, TS>{};
    template<const table_size TS> class crc16_dectr     : public impl::crc<alg::crc16_dectr, TS>{};
    template<const table_size TS> class crc16_dectx     : public impl::crc<alg::crc16_dectx, TS>{};
    template<const table_size TS> class crc16_dnp       : public impl::crc<alg::crc16_dnp, TS>{};
    template<const table_size TS> class crc16_en13757   : public impl::crc<alg::crc16_en13757, TS>{};
    template<const table_size TS> class crc16_genibus   : public impl::crc<alg::crc16_genibus, TS>{};
    template<const table_size TS> class crc16_maxim     : public impl::crc<alg::crc16_maxim, TS>{};
    template<const table_size TS> class crc16_mcrf4xx   : public impl::crc<alg::crc16_mcrf4xx, TS>{};
    template<const table_size TS> class crc16_riello    : public impl::crc<alg::crc16_riello, TS>{};
    template<const table_size TS> class crc16_t10dif    : public impl::crc<alg::crc16_t10dif, TS>{};
    template<const table_size TS> class crc16_teledisk  : public impl::crc<alg::crc16_teledisk, TS>{};
    template<const table_size TS> class crc16_tms37157  : public impl::crc<alg::crc16_tms37157, TS>{};
    template<const table_size TS> class crc16_usb       : public impl::crc<alg::crc16_usb, TS>{};
    template<const table_size TS> class crc16_a         : public impl::crc<alg::crc16_a, TS>{};
    template<const table_size TS> class crc16_kermit    : public impl::crc<alg::crc16_kermit, TS>{};
    template<const table_size TS> class crc16_modbus    : public impl::crc<alg::crc16_modbus, TS>{};
    template<const table_size TS> class crc16_x25       : public impl::crc<alg::crc16_x25, TS>{};
    template<const table_size TS> class crc16_xmodem    : public impl::crc<alg::crc16_xmodem, TS>{};

    template<const table_size TS> class crc32           : public impl::crc<alg::crc32, TS>{};
    template<const table_size TS> class crc32_bzip2     : public impl::crc<alg::crc32_bzip2, TS>{};
    template<const table_size TS> class crc32_c         : public impl::crc<alg::crc32_c, TS>{};
    template<const table_size TS> class crc32_d         : public impl::crc<alg::crc32_d, TS>{};
    template<const table_size TS> class crc32_mpeg2     : public impl::crc<alg::crc32_mpeg2, TS>{};
    template<const table_size TS> class crc32_posix     : public impl::crc<alg::crc32_posix, TS>{};
    template<const table_size TS> class crc32_q         : public impl::crc<alg::crc32_q, TS>{};
    template<const table_size TS> class crc32_jamcrc    : public impl::crc<alg::crc32_jamcrc, TS>{};
    template<const table_size TS> class crc32_xfer      : public impl::crc<alg::crc32_xfer, TS>{};

    template<const table_size TS> class crc64_ecma      : public impl::crc<alg::crc64_ecma, TS>{};

} // namespace family

namespace small {
    //------------------------------------------------------------------------
    //
    // Define the set of default CRC implementations using small table size
    //
    // These are the defined in the top level crc_cpp namespace
    //
    //------------------------------------------------------------------------

    using crc8 =           family::crc8            <table_size::small>;
    using crc8_cdma2000 =  family::crc8_cdma2000   <table_size::small>;
    using crc8_darc =      family::crc8_darc       <table_size::small>;
    using crc8_dvbs2 =     family::crc8_dvbs2      <table_size::small>;
    using crc8_ebu =       family::crc8_ebu        <table_size::small>;
    using crc8_icode =     family::crc8_icode      <table_size::small>;
    using crc8_itu =       family::crc8_itu        <table_size::small>;
    using crc8_maxim =     family::crc8_maxim      <table_size::small>;
    using crc8_rohc =      family::crc8_rohc       <table_size::small>;
    using crc8_wcdma =     family::crc8_wcdma      <table_size::small>;

    using crc16_ccit =     family::crc16_ccit      <table_size::small>;
    using crc16_arc =      family::crc16_arc       <table_size::small>;
    using crc16_augccit =  family::crc16_augccit   <table_size::small>;
    using crc16_buypass =  family::crc16_buypass   <table_size::small>;
    using crc16_cdma2000 = family::crc16_cdma2000  <table_size::small>;
    using crc16_dds110 =   family::crc16_dds110    <table_size::small>;
    using crc16_dectr =    family::crc16_dectr     <table_size::small>;
    using crc16_dectx =    family::crc16_dectx     <table_size::small>;
    using crc16_dnp =      family::crc16_dnp       <table_size::small>;
    using crc16_en13757 =  family::crc16_en13757   <table_size::small>;
    using crc16_genibus =  family::crc16_genibus   <table_size::small>;
    using crc16_maxim =    family::crc16_maxim     <table_size::small>;
    using crc16_mcrf4xx =  family::crc16_mcrf4xx   <table_size::small>;
    using crc16_riello =   family::crc16_riello    <table_size::small>;
    using crc16_t10dif =   family::crc16_t10dif    <table_size::small>;
    using crc16_teledisk = family::crc16_teledisk  <table_size::small>;
    using crc16_tms37157 = family::crc16_tms37157  <table_size::small>;
    using crc16_usb =      family::crc16_usb       <table_size::small>;
    using crc16_a =        family::crc16_a         <table_size::small>;
    using crc16_kermit =   family::crc16_kermit    <table_size::small>;
    using crc16_modbus =   family::crc16_modbus    <table_size::small>;
    using crc16_x25 =      family::crc16_x25       <table_size::small>;
    using crc16_xmodem =   family::crc16_xmodem    <table_size::small>;

    using crc32 =          family::crc32           <table_size::small>;
    using crc32_bzip2 =    family::crc32_bzip2     <table_size::small>;
    using crc32_c =        family::crc32_c         <table_size::small>;
    using crc32_d =        family::crc32_d         <table_size::small>;
    using crc32_mpeg2 =    family::crc32_mpeg2     <table_size::small>;
    using crc32_posix =    family::crc32_posix     <table_size::small>;
    using crc32_q =        family::crc32_q         <table_size::small>;
    using crc32_jamcrc =   family::crc32_jamcrc    <table_size::small>;
    using crc32_xfer =     family::crc32_xfer      <table_size::small>;

    using crc64_ecma =     family::crc64_ecma      <table_size::small>;
} // namespace small

namespace large
{
    //------------------------------------------------------------------------
    //
    // Define the set of large table CRC implementations
    //
    //------------------------------------------------------------------------
    using crc8 =           family::crc8            <table_size::large>;
    using crc8_cdma2000 =  family::crc8_cdma2000   <table_size::large>;
    using crc8_darc =      family::crc8_darc       <table_size::large>;
    using crc8_dvbs2 =     family::crc8_dvbs2      <table_size::large>;
    using crc8_ebu =       family::crc8_ebu        <table_size::large>;
    using crc8_icode =     family::crc8_icode      <table_size::large>;
    using crc8_itu =       family::crc8_itu        <table_size::large>;
    using crc8_maxim =     family::crc8_maxim      <table_size::large>;
    using crc8_rohc =      family::crc8_rohc       <table_size::large>;
    using crc8_wcdma =     family::crc8_wcdma      <table_size::large>;

    using crc16_ccit =     family::crc16_ccit      <table_size::large>;
    using crc16_arc =      family::crc16_arc       <table_size::large>;
    using crc16_augccit =  family::crc16_augccit   <table_size::large>;
    using crc16_buypass =  family::crc16_buypass   <table_size::large>;
    using crc16_cdma2000 = family::crc16_cdma2000  <table_size::large>;
    using crc16_dds110 =   family::crc16_dds110    <table_size::large>;
    using crc16_dectr =    family::crc16_dectr     <table_size::large>;
    using crc16_dectx =    family::crc16_dectx     <table_size::large>;
    using crc16_dnp =      family::crc16_dnp       <table_size::large>;
    using crc16_en13757 =  family::crc16_en13757   <table_size::large>;
    using crc16_genibus =  family::crc16_genibus   <table_size::large>;
    using crc16_maxim =    family::crc16_maxim     <table_size::large>;
    using crc16_mcrf4xx =  family::crc16_mcrf4xx   <table_size::large>;
    using crc16_riello =   family::crc16_riello    <table_size::large>;
    using crc16_t10dif =   family::crc16_t10dif    <table_size::large>;
    using crc16_teledisk = family::crc16_teledisk  <table_size::large>;
    using crc16_tms37157 = family::crc16_tms37157  <table_size::large>;
    using crc16_usb =      family::crc16_usb       <table_size::large>;
    using crc16_a =        family::crc16_a         <table_size::large>;
    using crc16_kermit =   family::crc16_kermit    <table_size::large>;
    using crc16_modbus =   family::crc16_modbus    <table_size::large>;
    using crc16_x25 =      family::crc16_x25       <table_size::large>;
    using crc16_xmodem =   family::crc16_xmodem    <table_size::large>;

    using crc32 =          family::crc32           <table_size::large>;
    using crc32_bzip2 =    family::crc32_bzip2     <table_size::large>;
    using crc32_c =        family::crc32_c         <table_size::large>;
    using crc32_d =        family::crc32_d         <table_size::large>;
    using crc32_mpeg2 =    family::crc32_mpeg2     <table_size::large>;
    using crc32_posix =    family::crc32_posix     <table_size::large>;
    using crc32_q =        family::crc32_q         <table_size::large>; using crc32_jamcrc =   family::crc32_jamcrc    <table_size::large>; using crc32_xfer =     family::crc32_xfer      <table_size::large>; using crc64_ecma =     family::crc64_ecma      <table_size::large>;

}   // namespace large

namespace tiny
{
    //------------------------------------------------------------------------
    //
    // Define the set of tiny table CRC implementations
    //
    //------------------------------------------------------------------------
    using crc8 =           family::crc8            <table_size::tiny>;
    using crc8_cdma2000 =  family::crc8_cdma2000   <table_size::tiny>;
    using crc8_darc =      family::crc8_darc       <table_size::tiny>;
    using crc8_dvbs2 =     family::crc8_dvbs2      <table_size::tiny>;
    using crc8_ebu =       family::crc8_ebu        <table_size::tiny>;
    using crc8_icode =     family::crc8_icode      <table_size::tiny>;
    using crc8_itu =       family::crc8_itu        <table_size::tiny>;
    using crc8_maxim =     family::crc8_maxim      <table_size::tiny>;
    using crc8_rohc =      family::crc8_rohc       <table_size::tiny>;
    using crc8_wcdma =     family::crc8_wcdma      <table_size::tiny>;

    using crc16_ccit =     family::crc16_ccit      <table_size::tiny>;
    using crc16_arc =      family::crc16_arc       <table_size::tiny>;
    using crc16_augccit =  family::crc16_augccit   <table_size::tiny>;
    using crc16_buypass =  family::crc16_buypass   <table_size::tiny>;
    using crc16_cdma2000 = family::crc16_cdma2000  <table_size::tiny>;
    using crc16_dds110 =   family::crc16_dds110    <table_size::tiny>;
    using crc16_dectr =    family::crc16_dectr     <table_size::tiny>;
    using crc16_dectx =    family::crc16_dectx     <table_size::tiny>;
    using crc16_dnp =      family::crc16_dnp       <table_size::tiny>;
    using crc16_en13757 =  family::crc16_en13757   <table_size::tiny>;
    using crc16_genibus =  family::crc16_genibus   <table_size::tiny>;
    using crc16_maxim =    family::crc16_maxim     <table_size::tiny>;
    using crc16_mcrf4xx =  family::crc16_mcrf4xx   <table_size::tiny>;
    using crc16_riello =   family::crc16_riello    <table_size::tiny>;
    using crc16_t10dif =   family::crc16_t10dif    <table_size::tiny>;
    using crc16_teledisk = family::crc16_teledisk  <table_size::tiny>;
    using crc16_tms37157 = family::crc16_tms37157  <table_size::tiny>;
    using crc16_usb =      family::crc16_usb       <table_size::tiny>;
    using crc16_a =        family::crc16_a         <table_size::tiny>;
    using crc16_kermit =   family::crc16_kermit    <table_size::tiny>;
    using crc16_modbus =   family::crc16_modbus    <table_size::tiny>;
    using crc16_x25 =      family::crc16_x25       <table_size::tiny>;
    using crc16_xmodem =   family::crc16_xmodem    <table_size::tiny>;

    using crc32 =          family::crc32           <table_size::tiny>;
    using crc32_bzip2 =    family::crc32_bzip2     <table_size::tiny>;
    using crc32_c =        family::crc32_c         <table_size::tiny>;
    using crc32_d =        family::crc32_d         <table_size::tiny>;
    using crc32_mpeg2 =    family::crc32_mpeg2     <table_size::tiny>;
    using crc32_posix =    family::crc32_posix     <table_size::tiny>;
    using crc32_q =        family::crc32_q         <table_size::tiny>;
    using crc32_jamcrc =   family::crc32_jamcrc    <table_size::tiny>;
    using crc32_xfer =     family::crc32_xfer      <table_size::tiny>;

    using crc64_ecma =     family::crc64_ecma      <table_size::tiny>;

}   // namespace tiny

// select the default size
using namespace small;

}   // namespace crc_cpp

#undef CRC_CPP_STD20_MODE
#undef CRC_CPP_API_CONSTEXPR


#endif // CRC_CPP_H_INCLUDED
