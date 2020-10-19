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

template<typename T>
bool test_reverse_bits(std::string name)
{
    bool status = true;

    int const bits = sizeof(T) * 8;

    for(int i = 0; i < bits; i++) {
        // careful with types to ensure we don't end up with signed ints
        T const test = T(0x01u) << i;
        T const expected = T(0x01u) << ((bits - 1) - i);

        T const result = crc_cpp::util::reverse_bits<T>(test);

        if(result != expected) {
            std::cout << "FAIL " << name << "\n";
            std::cout << "\tExpected: 0x"
                      << std::hex << std::setfill('0') << std::setw(2 * sizeof(expected))
                      << uint64_t(expected)
                      << " Got: 0x"
                      << std::hex << std::setfill('0') << std::setw(2 * sizeof(result))
                      << uint64_t(result) << "\n";

            status = false;
        }
    }

    if(status) {
        std::cout << "PASS " << name << "\n";
    }

    return status;
}


template<typename TCrc>
bool test_crc(std::string name, std::vector<uint8_t> message, typename TCrc::accumulator_type expected)
{
    TCrc crc;   // initialised by construction

    // Assert that the crc register is only the size of accululator.
    static_assert(sizeof(TCrc) == sizeof(typename TCrc::accumulator_type));

    for(auto c : message)
    {
        crc.update(c);
    }

    auto result = crc.final();

    if(result != expected) {
        std::cout << "FAIL " << name << "\n";
        std::cout << "\tExpected: 0x"
                  << std::hex << std::setfill('0') << std::setw(2 * sizeof(expected))
                  << uint64_t(expected)
                  << " Got: 0x"
                  << std::hex << std::setfill('0') << std::setw(2 * sizeof(result))
                  << uint64_t(result) << "\n";
    } else {
        std::cout << "PASS " << name << "\n";
    }

    return result == expected;
}

//
// Helper to test a family of the same algorithm implemented with different table sizes
//
template<template<const table_size> class TCrc>
bool test_crc(
        std::string name,
        std::vector<uint8_t> message,
        // hack to get to the accumulator type by fully quallifying one of the instances
        typename TCrc<table_size::small>::accumulator_type expected
        )
{
    bool result = true;

    // expand into the supported table sizes for the family
    result &=  test_crc<TCrc<table_size::tiny>>(name+" (tiny)", message, expected);
    result &= test_crc<TCrc<table_size::small>>(name+" (small)", message, expected);
    result &= test_crc<TCrc<table_size::large>>(name+" (large)", message, expected);

    return true;
}


int main()
{
    bool status = true;



    std::cout << "\nTesting Bit Reversal Helpers...\n";

    status &= test_reverse_bits<uint8_t>("uint8_t");
    status &= test_reverse_bits<uint32_t>("uint32_t");
    status &= test_reverse_bits<uint64_t>("uint64_t");



    std::cout << "\nTesting CRC Algorithms...\n";

    // Test vector and result from https://crccalc.com
    // except crc64_ecma which comes from https://etlcppp.com
    std::vector<uint8_t> message{'1', '2', '3', '4', '5', '6', '7', '8', '9' };


    // we are exeucting the tests per family (table size set, same algorithm).
    status &=           test_crc<family::crc8>("crc8",           message, 0xF4);
    status &=  test_crc<family::crc8_cdma2000>("crc8_cdma2000",  message, 0xDA);
    status &=      test_crc<family::crc8_darc>("crc8_darc",      message, 0x15);
    status &=     test_crc<family::crc8_dvbs2>("crc8_dvbs2",     message, 0xBC);
    status &=       test_crc<family::crc8_ebu>("crc8_ebu",       message, 0x97);
    status &=     test_crc<family::crc8_icode>("crc8_icode",     message, 0x7E);
    status &=       test_crc<family::crc8_itu>("crc8_itu",       message, 0xA1);
    status &=     test_crc<family::crc8_maxim>("crc8_maxim",     message, 0xA1);
    status &=      test_crc<family::crc8_rohc>("crc8_rohc",      message, 0xD0);
    status &=     test_crc<family::crc8_wcdma>("crc8_wcdma",     message, 0x25);

    status &=     test_crc<family::crc16_ccit>("crc16_ccit",     message, 0x29B1);
    status &=      test_crc<family::crc16_arc>("crc16_arc",      message, 0xBB3D);
    status &=  test_crc<family::crc16_augccit>("crc16_augccit",  message, 0xE5CC);
    status &=  test_crc<family::crc16_buypass>("crc16_buypass",  message, 0xFEE8);
    status &= test_crc<family::crc16_cdma2000>("crc16_cdma2000", message, 0x4C06);
    status &=   test_crc<family::crc16_dds110>("crc16_dds110",   message, 0x9ECF);
    status &=    test_crc<family::crc16_dectr>("crc16_dectr",    message, 0x007E);
    status &=    test_crc<family::crc16_dectx>("crc16_dectx",    message, 0x007F);
    status &=      test_crc<family::crc16_dnp>("crc16_dnp",      message, 0xEA82);
    status &=  test_crc<family::crc16_en13757>("crc16_en13757",  message, 0xC2B7);
    status &=  test_crc<family::crc16_genibus>("crc16_genibus",  message, 0xD64E);
    status &=    test_crc<family::crc16_maxim>("crc16_maxim",    message, 0x44C2);
    status &=  test_crc<family::crc16_mcrf4xx>("crc16_mcrf4xx",  message, 0x6F91);
    status &=   test_crc<family::crc16_riello>("crc16_riello",   message, 0x63D0);
    status &=   test_crc<family::crc16_t10dif>("crc16_t10dif",   message, 0xD0DB);
    status &= test_crc<family::crc16_teledisk>("crc16_teledisk", message, 0x0FB3);
    status &= test_crc<family::crc16_tms37157>("crc16_tms37157", message, 0x26B1);
    status &=      test_crc<family::crc16_usb>("crc16_usb",      message, 0xB4C8);
    status &=        test_crc<family::crc16_a>("crc16_a",        message, 0xBF05);
    status &=   test_crc<family::crc16_kermit>("crc16_kermit",   message, 0x2189);
    status &=   test_crc<family::crc16_modbus>("crc16_modbus",   message, 0x4B37);
    status &=      test_crc<family::crc16_x25>("crc16_x25",      message, 0x906E);
    status &=   test_crc<family::crc16_xmodem>("crc16_xmodem",   message, 0x31C3);

    status &=          test_crc<family::crc32>("crc32",          message, 0xCBF43926);
    status &=    test_crc<family::crc32_bzip2>("crc32_bzip2",    message, 0xFC891918);
    status &=        test_crc<family::crc32_c>("crc32_c",        message, 0xE3069283);
    status &=        test_crc<family::crc32_d>("crc32_d",        message, 0x87315576);
    status &=    test_crc<family::crc32_mpeg2>("crc32_mpeg2",    message, 0x0376E6E7);
    status &=    test_crc<family::crc32_posix>("crc32_posix",    message, 0x765E7680);
    status &=        test_crc<family::crc32_q>("crc32_q",        message, 0x3010BF7F);
    status &=   test_crc<family::crc32_jamcrc>("crc32_jamcrc",   message, 0x340BC6D9);
    status &=     test_crc<family::crc32_xfer>("crc32_xfer",     message, 0xBD0BE338);

    status &=    test_crc<family::crc64_ecma>("crc64_ecma",      message, 0x6C40DF5F0B497347U);




    if(status) {
        std::cout << "\nSuccess, all passed\n";
    } else {
        std::cout << "\nTests Failed!\n";
    }

    // return a zero status on success
    return status? 0:-1;
}

