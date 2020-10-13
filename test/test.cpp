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

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include "crc_cpp.h"

using namespace crc_cpp;


template <typename TCrc>
bool test_crc(std::string name, std::vector<uint8_t> message, typename TCrc::AccumulatorType expected)
{
    TCrc crc;


    crc.init();
    for(auto c : message)
    {
        crc.update(c);
    }

    auto result = crc.final();

    if(result != expected) {
        std::cout << "FAIL " << name << "\n";
        std::cout << "\tExpected: 0x" << std::hex << std::setfill('0')
                  << std::setw(2 * sizeof(expected)) << uint64_t(expected)
                  << " Got: 0x" << uint64_t(result) << "\n";
    } else {
        std::cout << "PASS " << name << "\n";
    }

    return result == expected;
}


int main()
{
    std::vector<uint8_t> message{'1', '2', '3', '4', '5', '6', '7', '8', '9' };

    bool status = true;

    status = status &&   test_crc<crc64_ecma>("crc64_scma",   message, 0x6C40DF5F0B497347U);
    status = status &&        test_crc<crc32>("crc32",        message, 0xCBF43926);
    status = status &&  test_crc<crc32_posix>("crc32_posix",  message, 0x765E7680);
    status = status &&   test_crc<crc32_xfer>("crc32_xfer",   message, 0xBD0BE338);
    status = status &&   test_crc<crc16_ccit>("crc16_ccit",   message, 0x29B1);
    status = status && test_crc<crc16_kermit>("crc16_kermit", message, 0x2189);
    status = status &&         test_crc<crc8>("crc8",         message, 0xF4);
    status = status &&    test_crc<crc8_rohc>("crc8_rohc",    message, 0xD0);

    if(status) {
        std::cout << "\nSuccess, all passed\n";
    } else {
        std::cout << "\nTests Failed!\n";
    }

    return status;
}

