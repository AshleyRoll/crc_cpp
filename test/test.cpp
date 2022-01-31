#include "crc_cpp.h"

#include <array>
#include <catch2/catch.hpp>
#include <string>
#include <vector>

using namespace crc_cpp;

template<typename T> bool is_expected(std::string const& name, T const result, T const expected)
{
    if (result != expected)
    {
        return false;
    }

    return true;
}

template<typename T> bool test_reverse_bits(std::string const& name)
{
    bool status = true;

    int const bits = sizeof(T) * 8;

    for (int i = 0; i < bits; i++)
    {
        // careful with types to ensure we don't end up with signed ints
        T const test = T(0x01u) << i;
        T const expected = T(0x01u) << ((bits - 1) - i);

        T const result = crc_cpp::util::reverse_bits<T>(test);

        status &= is_expected(name, result, expected);
    }

    return status;
}

template<typename TCrc> bool test_crc(std::string const& name, std::vector<uint8_t> const& message, typename TCrc::accumulator_type expected)
{
    TCrc crc;  // initialised by construction

    // Assert that the crc register is only the size of accululator.
    static_assert(sizeof(TCrc) == sizeof(typename TCrc::accumulator_type), "Unexpected CRC object size");

    for (auto const& c : message)
    {
        crc.update(c);
    }

    auto const result = crc.final();

    return is_expected(name, result, expected);
}

//
// Helper to test a family of the same algorithm implemented with different table sizes
//
template<template<const table_size> class TCrc>
bool test_crc(std::string const& name, std::vector<uint8_t> const& message,
              // hack to get to the accumulator type by fully quallifying one of the instances
              typename TCrc<table_size::small>::accumulator_type expected)
{
    bool result = true;

    // expand into the supported table sizes for the family
    result &= test_crc<TCrc<table_size::tiny>>(name + " (tiny)", message, expected);
    result &= test_crc<TCrc<table_size::small>>(name + " (small)", message, expected);
    result &= test_crc<TCrc<table_size::large>>(name + " (large)", message, expected);

    return result;
}

//------------------------------------------------------------------------
// Constexpr test
//
// Verify that the CRC can be calculated in a constexpr (compile-time) context
template<std::size_t N> constexpr bool constexpr_check_message(std::array<uint8_t, N> data, uint8_t const expected)
{
    crc_cpp::crc8 crc;
    for (auto const& b : data)
    {
        crc.update(b);
    }
    return expected == crc.final();
}

constexpr std::array<uint8_t, 9> constexpr_message{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
[[maybe_unused]] constexpr bool constexpr_test_result = constexpr_check_message(constexpr_message, 0xF4);

static_assert(constexpr_test_result, "Failed to compute crc at compile time");

TEST_CASE("BitReversing", "TestHelperFunction")
{
    REQUIRE(test_reverse_bits<uint8_t>("uint8_t"));
    REQUIRE(test_reverse_bits<uint32_t>("uint32_t"));
    REQUIRE(test_reverse_bits<uint64_t>("uint64_t"));
}

TEST_CASE("Algorithm", "TestCRC")
{
    const std::vector<uint8_t> message{ '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    REQUIRE(test_crc<family::crc8>("crc8", message, 0xF4));
    REQUIRE(test_crc<family::crc8_cdma2000>("crc8_cdma2000", message, 0xDA));
    REQUIRE(test_crc<family::crc8_darc>("crc8_darc", message, 0x15));
    REQUIRE(test_crc<family::crc8_dvbs2>("crc8_dvbs2", message, 0xBC));
    REQUIRE(test_crc<family::crc8_ebu>("crc8_ebu", message, 0x97));
    REQUIRE(test_crc<family::crc8_icode>("crc8_icode", message, 0x7E));
    REQUIRE(test_crc<family::crc8_itu>("crc8_itu", message, 0xA1));
    REQUIRE(test_crc<family::crc8_maxim>("crc8_maxim", message, 0xA1));
    REQUIRE(test_crc<family::crc8_rohc>("crc8_rohc", message, 0xD0));
    REQUIRE(test_crc<family::crc8_wcdma>("crc8_wcdma", message, 0x25));
    REQUIRE(test_crc<family::crc16_ccit>("crc16_ccit", message, 0x29B1));
    REQUIRE(test_crc<family::crc16_arc>("crc16_arc", message, 0xBB3D));
    REQUIRE(test_crc<family::crc16_augccit>("crc16_augccit", message, 0xE5CC));
    REQUIRE(test_crc<family::crc16_buypass>("crc16_buypass", message, 0xFEE8));
    REQUIRE(test_crc<family::crc16_cdma2000>("crc16_cdma2000", message, 0x4C06));
    REQUIRE(test_crc<family::crc16_dds110>("crc16_dds110", message, 0x9ECF));
    REQUIRE(test_crc<family::crc16_dectr>("crc16_dectr", message, 0x007E));
    REQUIRE(test_crc<family::crc16_dectx>("crc16_dectx", message, 0x007F));
    REQUIRE(test_crc<family::crc16_dnp>("crc16_dnp", message, 0xEA82));
    REQUIRE(test_crc<family::crc16_en13757>("crc16_en13757", message, 0xC2B7));
    REQUIRE(test_crc<family::crc16_genibus>("crc16_genibus", message, 0xD64E));
    REQUIRE(test_crc<family::crc16_maxim>("crc16_maxim", message, 0x44C2));
    REQUIRE(test_crc<family::crc16_mcrf4xx>("crc16_mcrf4xx", message, 0x6F91));
    REQUIRE(test_crc<family::crc16_riello>("crc16_riello", message, 0x63D0));
    REQUIRE(test_crc<family::crc16_t10dif>("crc16_t10dif", message, 0xD0DB));
    REQUIRE(test_crc<family::crc16_teledisk>("crc16_teledisk", message, 0x0FB3));
    REQUIRE(test_crc<family::crc16_tms37157>("crc16_tms37157", message, 0x26B1));
    REQUIRE(test_crc<family::crc16_usb>("crc16_usb", message, 0xB4C8));
    REQUIRE(test_crc<family::crc16_a>("crc16_a", message, 0xBF05));
    REQUIRE(test_crc<family::crc16_kermit>("crc16_kermit", message, 0x2189));
    REQUIRE(test_crc<family::crc16_modbus>("crc16_modbus", message, 0x4B37));
    REQUIRE(test_crc<family::crc16_x25>("crc16_x25", message, 0x906E));
    REQUIRE(test_crc<family::crc16_xmodem>("crc16_xmodem", message, 0x31C3));
    REQUIRE(test_crc<family::crc32>("crc32", message, 0xCBF43926));
    REQUIRE(test_crc<family::crc32_bzip2>("crc32_bzip2", message, 0xFC891918));
    REQUIRE(test_crc<family::crc32_c>("crc32_c", message, 0xE3069283));
    REQUIRE(test_crc<family::crc32_d>("crc32_d", message, 0x87315576));
    REQUIRE(test_crc<family::crc32_mpeg2>("crc32_mpeg2", message, 0x0376E6E7));
    REQUIRE(test_crc<family::crc32_posix>("crc32_posix", message, 0x765E7680));
    REQUIRE(test_crc<family::crc32_q>("crc32_q", message, 0x3010BF7F));
    REQUIRE(test_crc<family::crc32_jamcrc>("crc32_jamcrc", message, 0x340BC6D9));
    REQUIRE(test_crc<family::crc32_xfer>("crc32_xfer", message, 0xBD0BE338));
    REQUIRE(test_crc<family::crc64_ecma>("crc64_ecma", message, 0x6C40DF5F0B497347U));
}