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



#include "include/crc.h"

template <typename TCrc>
constexpr bool test_crc(const uint8_t message[], int message_len, typename TCrc::AccumulatorType expected)
{
    TCrc crc;

    crc.init();
    for(int i = 0; i < message_len; i++)
    {
        crc.update(message[i]);
    }

    return crc.final() == expected;
}


int main()
{
    uint8_t message[]{'1', '2', '3', '4', '5', '6', '7', '8', '9' };
    int const len = sizeof(message);

    return true
        && test_crc<crc64_ecma>(message, len, 0x6C40DF5F0B497347U)
        && test_crc<crc32>(message, len, 0xCBF43926)
        && test_crc<crc32_posix>(message, len, 0x765E7680)
        && test_crc<crc32_xfer>(message, len, 0xBD0BE338)
        && test_crc<crc16_ccit>(message, len, 0x29B1)
        && test_crc<crc16_kermit>(message, len, 0x2189)
        && test_crc<crc8>(message, len, 0xf4)
        && test_crc<crc8_rohc>(message, len, 0xD0)
        ;
}

