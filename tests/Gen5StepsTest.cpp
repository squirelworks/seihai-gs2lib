// Seihai project, 2026-10-31: see CHANGES-SEIHAI.md.
//
// This file is part of the Seihai fork of gs2lib and is released under the
// GNU General Public License version 3 or later (see LICENSE.md).
//
// Checks the gen-5 step rules (CEncryption::Gen5Steps). The expected bytes
// were computed separately from the cipher's definition: the iterator starts
// at 0x04A80B38 and steps as it * 0x08088405 + key, and its little-endian
// bytes are XORed onto each 4-byte group.

#include <cstdio>
#include <cstdlib>
#include <string>

#include <CEncryption.h>
#include <IUtil.h>

static int failures = 0;

#define CHECK(cond)                                                          \
    do                                                                       \
    {                                                                        \
        if (!(cond))                                                         \
        {                                                                    \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

static CString zeros(int length)
{
    CString s;
    for (int i = 0; i < length; ++i)
        s.writeChar(0);
    return s;
}

static std::string hex(const CString& s)
{
    static const char digits[] = "0123456789abcdef";
    std::string out;
    for (int i = 0; i < s.length(); ++i)
    {
        const auto b = static_cast<uint8_t>(s[i]);
        out += digits[b >> 4];
        out += digits[b & 15];
    }
    return out;
}

static CEncryption codec(uint8_t key, Gen5Steps steps)
{
    CEncryption c;
    c.setGen(ENCRYPT_GEN_5);
    c.setGen5Steps(steps);
    c.reset(key);
    return c;
}

static CString encode(CEncryption& c, int length, uint8_t type)
{
    c.limitFromType(type);
    return c.encrypt(zeros(length));
}

int main()
{
    // Every client but 5.12: a 7-byte zlib body steps twice (rounded up).
    {
        auto c = codec(0x4B, Gen5Steps::PerStartedGroup);
        CHECK(hex(encode(c, 7, COMPRESS_ZLIB)) == "6318d1d63a86c0");
        CHECK(hex(encode(c, 4, COMPRESS_ZLIB)) == "6d87c89b");
    }

    // 5.12: the same body steps once and leaves bytes 4 to 6 in the clear;
    // the next body then gets the second step.
    {
        auto c = codec(0x4B, Gen5Steps::PerWholeGroup);
        CHECK(hex(encode(c, 7, COMPRESS_ZLIB)) == "6318d1d6000000");
        CHECK(hex(encode(c, 4, COMPRESS_ZLIB)) == "3a86c0dd");
    }

    // 5.12 on bz2 bodies too.
    {
        auto c = codec(0x4B, Gen5Steps::PerWholeGroup);
        CHECK(hex(encode(c, 7, COMPRESS_BZ2)) == "6318d1d6000000");
    }

    // 5.12's uncompressed bodies, and whole groups, step as everyone else's.
    for (const auto& [length, type] : {std::pair{7, COMPRESS_UNCOMPRESSED}, std::pair{8, COMPRESS_ZLIB}, std::pair{100, COMPRESS_BZ2}, std::pair{61, COMPRESS_UNCOMPRESSED}})
    {
        auto up = codec(9, Gen5Steps::PerStartedGroup);
        auto down = codec(9, Gen5Steps::PerWholeGroup);
        CHECK(encode(up, length, static_cast<uint8_t>(type)) == encode(down, length, static_cast<uint8_t>(type)));
        CHECK(encode(up, 4, COMPRESS_ZLIB) == encode(down, 4, COMPRESS_ZLIB));
    }

    // 5.12: a zlib body under 4 bytes is not XORed and does not step.
    {
        auto c = codec(0x4B, Gen5Steps::PerWholeGroup);
        CHECK(hex(encode(c, 3, COMPRESS_ZLIB)) == "000000");
        CHECK(hex(encode(c, 4, COMPRESS_ZLIB)) == "6318d1d6");
    }

    // 5.12: a long compressed body still steps at most 4 times.
    {
        auto down = codec(0x21, Gen5Steps::PerWholeGroup);
        auto up = codec(0x21, Gen5Steps::PerStartedGroup);
        CHECK(encode(down, 23, COMPRESS_ZLIB) == encode(up, 23, COMPRESS_ZLIB));
        CHECK(encode(down, 4, COMPRESS_ZLIB) == encode(up, 4, COMPRESS_ZLIB));
    }

    // decrypt() undoes encrypt() under both rules, frame after frame, for
    // every short length and type.
    for (const auto steps : {Gen5Steps::PerStartedGroup, Gen5Steps::PerWholeGroup})
    {
        auto tx = codec(0x37, steps);
        auto rx = codec(0x37, steps);
        for (int length = 1; length <= 40; ++length)
        {
            for (const uint8_t type : {COMPRESS_UNCOMPRESSED, COMPRESS_ZLIB, COMPRESS_BZ2})
            {
                CString plain;
                for (int i = 0; i < length; ++i)
                    plain.writeChar(static_cast<char>(i * 7 + type));
                tx.limitFromType(type);
                CString wire = tx.encrypt(plain);
                rx.limitFromType(type);
                rx.decrypt(wire);
                CHECK(wire == plain);
            }
        }
    }

    // reset() keeps the rule; only 5.12 rounds down.
    {
        auto c = codec(0x4B, Gen5Steps::PerWholeGroup);
        c.reset(0x10);
        CHECK(c.getGen5Steps() == Gen5Steps::PerWholeGroup);
        CHECK(CEncryption::gen5StepsForClient(CLVER_5_12) == Gen5Steps::PerWholeGroup);
        CHECK(CEncryption::gen5StepsForClient(CLVER_5_07) == Gen5Steps::PerStartedGroup);
        CHECK(CEncryption::gen5StepsForClient(CLVER_5_31x) == Gen5Steps::PerStartedGroup);
        CHECK(CEncryption::gen5StepsForClient(CLVER_2_22) == Gen5Steps::PerStartedGroup);
        CHECK(CEncryption::gen5StepsForClient(CLVER_UNKNOWN) == Gen5Steps::PerStartedGroup);
        CHECK(getVersionID("G3D14097") == CLVER_5_12);
    }

    if (failures != 0)
    {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    std::printf("Gen5StepsTest: all checks passed\n");
    return EXIT_SUCCESS;
}
