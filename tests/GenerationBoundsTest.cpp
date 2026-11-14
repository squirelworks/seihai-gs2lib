// Modified by the Seihai project, 2026-11-15: see CHANGES-SEIHAI.md.
//
// This file is part of the Seihai fork of gs2lib and is released under the
// GNU General Public License version 3 or later (see LICENSE.md).
//
// setGen() stores a generation and reset() uses it as an index into the
// iterator table. An out-of-range generation must stay on a real entry.
// In-process codec only: no socket and no remote client.

#include <cstdint>
#include <cstdio>
#include <limits>

#include <CEncryption.h>

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

// reset() is the call that indexes the iterator table. encrypt() and
// decrypt() then run on whatever generation was stored.
static void indexTable(CEncryption& codec, uint8_t key)
{
    codec.reset(key);
    CString body("abc");
    CString encoded = codec.encrypt(body);
    codec.decrypt(encoded);
}

int main()
{
    // Every named generation is a table index and must be kept as given.
    for (uint32_t gen = 0; gen <= static_cast<uint32_t>(ENCRYPT_GEN_6); ++gen)
    {
        CEncryption codec;
        codec.setGen(gen);
        CHECK(codec.getGen() == gen);
        indexTable(codec, 0x21);
        CHECK(codec.getGen() == gen);
    }

    // 6 is one past the last named generation. Larger values used to clamp
    // to that same out-of-range index.
    const uint32_t outOfRange[] = {
        static_cast<uint32_t>(ENCRYPT_GEN_6) + 1U,
        static_cast<uint32_t>(ENCRYPT_GEN_6) + 2U,
        64U,
        std::numeric_limits<uint32_t>::max(),
    };
    for (const uint32_t gen : outOfRange)
    {
        CEncryption codec;
        codec.setGen(gen);
        CHECK(codec.getGen() == static_cast<uint32_t>(ENCRYPT_GEN_6));
        indexTable(codec, 0x21);
        CHECK(codec.getGen() == static_cast<uint32_t>(ENCRYPT_GEN_6));
    }

    if (failures != 0)
    {
        std::fprintf(stderr, "GenerationBoundsTest: %d checks failed\n", failures);
        return 1;
    }
    std::printf("GenerationBoundsTest: all checks passed\n");
    return 0;
}
