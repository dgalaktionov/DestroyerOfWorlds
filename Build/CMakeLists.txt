cmake_minimum_required(VERSION 3.0)

project(cryptopp C CXX ASM)

set(PROJECT_DESCRIPTION "DestroyerOfWorlds")
set(CMAKE_PROJECT_NAME "DestroyerOfWorlds")
#set(CMAKE_BUILD_TYPE "Debug")
#set(PROJECTNAME_ARCHITECTURE "x86")
set(CMAKE_COMPILER_IS_GNUCXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-O2)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_VERBOSE_MAKEFILE ON)
    add_definitions(-O0)
    add_definitions(-g)
    add_definitions(-DDEBUG)
    add_link_options(-g)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT PROJECTNAME_ARCHITECTURE)
    set(PROJECTNAME_ARCHITECTURE "x86")
elseif(PROJECTNAME_ARCHITECTURE STREQUAL "x86")
    add_definitions(-m32)
    add_link_options(-m32)
elseif(PROJECTNAME_ARCHITECTURE STREQUAL "x64")
    add_definitions(-m64)
    add_link_options(-m64)

    set(X64_SRCS
            ../ThirdParty/cryptopp/x64dll.asm
            ../ThirdParty/cryptopp/x64masm.asm
            )
    # NOT ACTUALLY USING THEM AS THEY HAVE MASM SYNTAX
endif(NOT PROJECTNAME_ARCHITECTURE)

message("PROJECT = ${CMAKE_PROJECT_NAME}")
message("CMAKE_BUILD_CONFIG = ${CMAKE_BUILD_TYPE}|${PROJECTNAME_ARCHITECTURE}")

include_directories(
        .
        ../ThirdParty
        ../Code
        ../ThirdParty/cryptopp
        ../Code/core/include
        ../Code/network/include
        ../Code/protocol/include
        ../Code/tests/include

)


# Compiler options
add_definitions(-ffast-math)
add_definitions(-msse2)
add_definitions(-Wall)
add_definitions(-Wextra)
add_definitions(-std=c++17)
add_definitions(
        -D_CRT_SECURE_NO_WARNINGS
        -D_WINSOCK_DEPRECATED_NO_WARNINGS
        -D_LIB
        -DCRYPTOPP_DISABLE_SSSE3
)

if(WIN32)
    # Resource options
endif(WIN32)

# Library path
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -L. ")

# Link options
add_link_options(-lm -lpthread -pthread -Wl,--no-as-needed -lrt -fPIC)

# Define the CXX sources
set (CXX_SRCS
        ../ThirdParty/cryptopp/zlib.cpp
        ../ThirdParty/cryptopp/zinflate.cpp
        ../ThirdParty/cryptopp/xtrcrypt.cpp
        ../ThirdParty/cryptopp/xtr.cpp
        ../ThirdParty/cryptopp/tigertab.cpp
        ../ThirdParty/cryptopp/tftables.cpp
        ../ThirdParty/cryptopp/square.cpp
        ../ThirdParty/cryptopp/sosemanuk.cpp
        ../ThirdParty/cryptopp/skipjack.cpp
        ../ThirdParty/cryptopp/simple.cpp
        ../ThirdParty/cryptopp/shacal2.cpp
        ../ThirdParty/cryptopp/serpent.cpp
        ../ThirdParty/cryptopp/seal.cpp
        ../ThirdParty/cryptopp/salsa.cpp
        ../ThirdParty/cryptopp/safer.cpp
        ../ThirdParty/cryptopp/rw.cpp
        ../ThirdParty/cryptopp/rng.cpp
        ../ThirdParty/cryptopp/rdtables.cpp
        ../ThirdParty/cryptopp/seed.cpp
        ../ThirdParty/cryptopp/rc2.cpp
        ../ThirdParty/cryptopp/queue.cpp
        ../ThirdParty/cryptopp/pubkey.cpp
        ../ThirdParty/cryptopp/pssr.cpp
        ../ThirdParty/cryptopp/rijndael.cpp
        ../ThirdParty/cryptopp/pkcspad.cpp
        ../ThirdParty/cryptopp/sha3.cpp
        ../ThirdParty/cryptopp/pch.cpp
        ../ThirdParty/cryptopp/rc6.cpp
        ../ThirdParty/cryptopp/rc5.cpp
        ../ThirdParty/cryptopp/oaep.cpp
        ../ThirdParty/cryptopp/nbtheory.cpp
        ../ThirdParty/cryptopp/rsa.cpp
        ../ThirdParty/cryptopp/sharkbox.cpp
        ../ThirdParty/cryptopp/ida.cpp
        ../ThirdParty/cryptopp/tea.cpp
        ../ThirdParty/cryptopp/cpu.cpp
        ../ThirdParty/cryptopp/basecode.cpp
        ../ThirdParty/cryptopp/dll.cpp
        ../ThirdParty/cryptopp/shark.cpp
        ../ThirdParty/cryptopp/default.cpp
        ../ThirdParty/cryptopp/elgamal.cpp
        ../ThirdParty/cryptopp/chacha_avx.cpp
        ../ThirdParty/cryptopp/blake2b_simd.cpp
        ../ThirdParty/cryptopp/adler32.cpp
        ../ThirdParty/cryptopp/cryptlib.cpp
        ../ThirdParty/cryptopp/des.cpp
        ../ThirdParty/cryptopp/vmac.cpp
        ../ThirdParty/cryptopp/blake2.cpp
        ../ThirdParty/cryptopp/crc.cpp
        ../ThirdParty/cryptopp/sha.cpp
        ../ThirdParty/cryptopp/cast.cpp
        ../ThirdParty/cryptopp/osrng.cpp
        ../ThirdParty/cryptopp/ccm.cpp
        ../ThirdParty/cryptopp/gcm.cpp
        ../ThirdParty/cryptopp/cbcmac.cpp
        ../ThirdParty/cryptopp/blowfish.cpp
        ../ThirdParty/cryptopp/mars.cpp
        ../ThirdParty/cryptopp/dh.cpp
        ../ThirdParty/cryptopp/bfinit.cpp
        ../ThirdParty/cryptopp/panama.cpp
        ../ThirdParty/cryptopp/channels.cpp
        ../ThirdParty/cryptopp/base64.cpp
        ../ThirdParty/cryptopp/md2.cpp
        ../ThirdParty/cryptopp/arc4.cpp
        ../ThirdParty/cryptopp/luc.cpp
        ../ThirdParty/cryptopp/dh2.cpp
        ../ThirdParty/cryptopp/whrlpool.cpp
        ../ThirdParty/cryptopp/chacha_simd.cpp
        ../ThirdParty/cryptopp/blake2s_simd.cpp
        ../ThirdParty/cryptopp/ripemd.cpp
        ../ThirdParty/cryptopp/algparam.cpp
        ../ThirdParty/cryptopp/hex.cpp
        ../ThirdParty/cryptopp/chacha.cpp
        ../ThirdParty/cryptopp/3way.cpp
        ../ThirdParty/cryptopp/asn.cpp
        ../ThirdParty/cryptopp/authenc.cpp
        ../ThirdParty/cryptopp/dessp.cpp
        ../ThirdParty/cryptopp/zdeflate.cpp
        ../ThirdParty/cryptopp/algebra.cpp
        ../ThirdParty/cryptopp/md4.cpp
        ../ThirdParty/cryptopp/mqueue.cpp
        ../ThirdParty/cryptopp/strciphr.cpp
        ../ThirdParty/cryptopp/gf2n.cpp
        ../ThirdParty/cryptopp/squaretb.cpp
        ../ThirdParty/cryptopp/blumshub.cpp
        ../ThirdParty/cryptopp/dsa.cpp
        ../ThirdParty/cryptopp/polynomi.cpp
        ../ThirdParty/cryptopp/eax.cpp
        ../ThirdParty/cryptopp/base32.cpp
        ../ThirdParty/cryptopp/fips140.cpp
        ../ThirdParty/cryptopp/ec2n.cpp
        ../ThirdParty/cryptopp/ecp.cpp
        ../ThirdParty/cryptopp/filters.cpp
        ../ThirdParty/cryptopp/gzip.cpp
        ../ThirdParty/cryptopp/emsa2.cpp
        ../ThirdParty/cryptopp/files.cpp
        ../ThirdParty/cryptopp/casts.cpp
        ../ThirdParty/cryptopp/sse_simd.cpp
        ../ThirdParty/cryptopp/rabin.cpp
        ../ThirdParty/cryptopp/eprecomp.cpp
        ../ThirdParty/cryptopp/gf256.cpp
        ../ThirdParty/cryptopp/hrtimer.cpp
        ../ThirdParty/cryptopp/keccak_core.cpp
        ../ThirdParty/cryptopp/wake.cpp
        ../ThirdParty/cryptopp/gf2_32.cpp
        ../ThirdParty/cryptopp/randpool.cpp
        ../ThirdParty/cryptopp/gfpcrypt.cpp
        ../ThirdParty/cryptopp/gost.cpp
        ../ThirdParty/cryptopp/twofish.cpp
        ../ThirdParty/cryptopp/cmac.cpp
        ../ThirdParty/cryptopp/ttmac.cpp
        ../ThirdParty/cryptopp/hmac.cpp
        ../ThirdParty/cryptopp/idea.cpp
        ../ThirdParty/cryptopp/integer.cpp
        ../ThirdParty/cryptopp/iterhash.cpp
        ../ThirdParty/cryptopp/mqv.cpp
        ../ThirdParty/cryptopp/esign.cpp
        ../ThirdParty/cryptopp/marss.cpp
        ../ThirdParty/cryptopp/tiger.cpp
        ../ThirdParty/cryptopp/eccrypto.cpp
        ../ThirdParty/cryptopp/misc.cpp
        ../ThirdParty/cryptopp/md5.cpp
        ../ThirdParty/cryptopp/modes.cpp
        )

#[[set_source_files_properties(
        ${CXX_SRCS} PROPERTIES COMPILE_FLAGS
        " -m32 -ffast-math -O0 -g -msse2 -Wall -Wextra -std=c++17")]]

#[[set_source_files_properties(
        ${X64_SRCS} PROPERTIES COMPILE_FLAGS
        "-x assembler-with-cpp -march=native")]]

if(WIN32)
    enable_language(RC)
    set(CMAKE_RC_COMPILE_OBJECT
            "<CMAKE_RC_COMPILER> ${RC_OPTIONS} -O coff -i <SOURCE> -o <OBJECT>")
endif(WIN32)


add_library(cryptopp STATIC ${CXX_SRCS} ${C_SRCS})


file(GLOB_RECURSE CORE_SRC ../Code/core/src/*.cpp)
add_library(core STATIC ${CORE_SRC})

file(GLOB_RECURSE NETWORK_SRC ../Code/network/src/*.cpp)
add_library(network STATIC ${NETWORK_SRC})
#target_link_libraries(network core)

file(GLOB_RECURSE PROTOCOL_SRC ../Code/protocol/src/*.cpp)
add_library(protocol STATIC ${PROTOCOL_SRC})
target_link_libraries(protocol core cryptopp)

file(GLOB_RECURSE TESTS_SRC ../Code/tests/src/*.cpp)
add_executable(tests ${TESTS_SRC})
target_link_libraries(tests core network protocol cryptopp)
