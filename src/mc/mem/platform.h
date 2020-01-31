#pragma once

#include <estd/internal/platform.h>
#include <mc/opts-internal.h>

//#define DEBUG
// TODO: Account for some kind of estd-flavor ostream also
#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
// TODO: Identify a better way to identify presence of C++ std::cerr
#if __ADSPBLACKFIN__
// blackfin doesn't have cerr, and probably others don't also
#define MCMEM_CERR  ::std::cout
#else
#define MCMEM_CERR  ::std::cerr
#endif
#define FEATURE_MCMEM_ASSERT_ENABLE
#endif

// TODO: Consolidate all this into estd
#ifdef _MSC_VER
#if _MSVC_LANG >= 201100
#define __CPP11__
#define FEATURE_CPP_STRNCPY_S
#endif
#if _MSVC_LANG >= 201400
#define __CPP14__
#endif
#else

#if __cplusplus >= 201103L
#define __CPP11__
#endif
#if __cplusplus >= 201703L
#define FEATURE_CPP_STRNCPY_S
#endif

#endif

#if __ADSPBLACKFIN__ || defined(_MSC_VER)
#define PACKED
#define USE_PRAGMA_PACK
#else
#define PACKED __attribute__ ((packed))
#endif

#ifdef __ADSPBLACKFIN__
// FIX: Not sure if this really is a double here
typedef double float32_t;
#else
typedef float float32_t;
typedef double float64_t;
#endif

#if __ADSPBLACKFIN__ || defined(_MSC_VER)
#define COAP_HOST_LITTLE_ENDIAN
#elif defined(__GNUC__)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define COAP_HOST_BIG_ENDIAN
#else
#define COAP_HOST_LITTLE_ENDIAN
#endif

#endif

// TODO: Make this generate log warnings or something
#ifdef FEATURE_MCMEM_ASSERT_ENABLE
#include <iostream>
#define ASSERT(expected, actual) if((expected) != (actual)) \
{ MCMEM_CERR << "ASSERT: (" << __func__ << ") " << ::std::endl; }
// This will generate warnings but the program will keep going
#define ASSERT_WARN(expected, actual, message) if((expected) != (actual)) \
{ MCMEM_CERR << "WARN: (" << __func__ << ") " << message << ::std::endl; }
// This will generate a warning and issue a 'return' statement immediately
#define ASSERT_ABORT(expected, actual, message)
// This will halt the program with a message
// One may *carefully* use << syntax within message
#define ASSERT_ERROR(expected, actual, message) if((expected) != (actual)) \
{ MCMEM_CERR << "ERROR: (" << __func__ << ") " << message << ::std::endl; }
#else
#define ASSERT(expected, actual)
// This will generate warnings but the program will keep going
#define ASSERT_WARN(expected, actual, message)
// This will generate a warning and issue a 'return' statement immediately
#define ASSERT_ABORT(expected, actual, message)
// This will halt the program with a message
#define ASSERT_ERROR(expected, actual, message)
#endif


// NOTE: untested old trick for byte swapping without a temp variable
#define COAP_SWAP_2_BYTES(val0, val1) { val0 ^= val1; val1 ^= val0; val0 ^= val1; }

#ifdef COAP_HOST_BIG_ENDIAN
#define COAP_HTONS(int_short) int_short
#define COAP_NTOHS(int_short) int_short
#define COAP_NTOH_2_BYTES(val0, val1)    {}
// network order bytes
// Never mind, doing bit shifting *this* way always produces the desired
// result regardless of endianness
//#define COAP_UINT16_FROM_NBYTES(val0, val1) (((uint16_t)val0) << 8 | val1)
#elif defined(COAP_HOST_LITTLE_ENDIAN)
#define COAP_HTONS(int_short) SWAP_16(int_short)
#define COAP_NTOHS(int_short) SWAP_16(int_short)
//#define COAP_UINT16_FROM_NBYTES(val0, val1) (((uint16_t)val1) << 8 | val0)
#define COAP_NTOH_2_BYTES(val0, val1)    COAP_SWAP_2_BYTES(val0, val1)
#else
#error "Unknown processor endianness"
#endif


// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#ifdef __ADSPBLACKFIN__
#define ENVIRONMENT32
#endif
