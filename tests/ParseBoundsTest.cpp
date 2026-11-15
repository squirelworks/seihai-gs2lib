// Modified by the Seihai project, 2026-11-15: see CHANGES-SEIHAI.md.
//
// This file is part of the Seihai fork of gs2lib and is released under the
// GNU General Public License version 3 or later (see LICENSE.md).
//
// Lengths and indexes the public parsers take from the caller. In-process
// only: no socket. The same calls stopped under ASan or UBSan before the
// bounds checks, or did not return (empty replaceAll).

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include <CEncryption.h>
#include <CString.h>
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

static bool sameBytes(const CString& got, const unsigned char* expect, int n)
{
    if (got.length() != n)
        return false;
    return std::memcmp(got.text(), expect, static_cast<size_t>(n)) == 0;
}

static CString fromBytes(const unsigned char* p, int n)
{
    CString s;
    if (n > 0)
        s.write(reinterpret_cast<const char*>(p), n);
    return s;
}

static void checkInRange()
{
    CHECK(CString("hello").base64encode() == CString("aGVsbG8="));
    CHECK(CString("QQ==").base64decode() == CString("A"));
    const unsigned char aaaa[4] = {'A', 'A', 'A', 'A'};
    CString decoded = fromBytes(aaaa, 4).base64decode();
    CHECK(decoded.length() == 3);
    CHECK(decoded.length() == 3 && decoded[0] == 0 && decoded[1] == 0 && decoded[2] == 0);

    const unsigned char enc[4] = {0x6c, 0x64, 0x5c, 0x54};
    const unsigned char dec[4] = {0x24, 0xe2, 0xa1, 0x60};
    CHECK(sameBytes(CString("abcd").encodesimple(4), enc, 4));
    CHECK(sameBytes(CString("abcd").decodesimple(4), dec, 4));
    CHECK(CString("abcd").codesimplefix0(4) == CString("abcd"));

    CHECK(CString("abc").find("b", 0) == 1);
    CHECK(CString("hello").findi("E", 1) == 1);
    CHECK(CString("hello").right(2) == CString("lo"));
    CHECK(CString("hello").right(100) == CString("hello"));
    CHECK(CString("hello").right(0).isEmpty());
    CHECK(CString("ab").replaceAll("a", "x") == CString("xb"));

    char dest[4] = {};
    CString readable("ab");
    CHECK(readable.read(dest, 2) == 2);
    CHECK(dest[0] == 'a' && dest[1] == 'b');

    CString round = CString("hello hello hello").zcompress().zuncompress();
    CHECK(round == CString("hello hello hello"));

    const unsigned char wsHello[7] = {0x82, 0x05, 'h', 'e', 'l', 'l', 'o'};
    CString small("hello");
    webSocketFixOutgoingPacket(small);
    CHECK(sameBytes(small, wsHello, 7));

    const unsigned char frame[11] = {
        0x82, 0x85, 0x01, 0x02, 0x03, 0x04,
        static_cast<unsigned char>('h' ^ 1),
        static_cast<unsigned char>('e' ^ 2),
        static_cast<unsigned char>('l' ^ 3),
        static_cast<unsigned char>('l' ^ 4),
        static_cast<unsigned char>('o' ^ 1),
    };
    CString incoming = fromBytes(frame, 11);
    CHECK(webSocketFixIncomingPacket(incoming) == 5);
    CHECK(incoming == CString("hello"));
}

static void checkOutOfRange()
{
    CHECK(CString().base64encode().isEmpty());
    CHECK(CString("A").base64encode() == CString("QQ=="));

    CString heap(std::string(40, 'A'));
    heap.setSize(0);
    CHECK(heap.base64decode().isEmpty());
    CHECK(heap.gtokenize().isEmpty());

    const unsigned char high[4] = {0xff, 0xff, 0xff, 0xff};
    CString highOut = fromBytes(high, 4).base64decode();
    CHECK(highOut.length() == 3);

    CHECK(CString("abcd").encodesimple() == CString("abcd").encodesimple(4));
    CHECK(CString("abcd").decodesimple() == CString("abcd").decodesimple(4));
    CHECK(CString("abcd").codesimplefix0() == CString("abcd").codesimplefix0(4));
    CHECK(CString("hi").encodesimple().length() == 2);
    CHECK(CString("hi").decodesimple().length() == 2);
    CHECK(CString("hi").codesimplefix0().length() == 2);

    std::string wide(40, 'a');
    wide[10] = 'b';
    CString heapFind(wide);
    CHECK(heapFind.find("b", -1) == 10);
    CHECK(heapFind.find(CString("b"), -1) == 10);
    CHECK(heapFind.findi(CString("B"), -1) == 10);

    char dest[4] = {1, 2, 3, 4};
    CString readable("ab");
    CHECK(readable.read(dest, -1) == 0);
    CHECK(dest[0] == 1 && dest[1] == 2);
    CString writable;
    char src[4] = {'a', 'b', 'c', 'd'};
    CHECK(writable.write(src, -1) == 0);
    CHECK(writable.isEmpty());

    CHECK(CString("hello").right(INT_MIN).isEmpty());
    CHECK(CString("hello").right(-1).isEmpty());

    CHECK(CString("ab").replaceAll("", "x") == CString("ab"));

    CString sized("hi");
    sized.setSize(100000);
    CHECK(sized.length() == 2);
    CHECK(sized.length() >= 2 && sized[0] == 'h' && sized[1] == 'i');
    CHECK(sized.toString().size() == static_cast<size_t>(sized.length()));

    CString grown(std::string(40, 'Q'));
    grown.setSize(100000);
    CHECK(grown.length() < 100000);
    CHECK(grown.toString().size() == static_cast<size_t>(grown.length()));

    unsigned char raw[8] = {0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0xff, 0x00};
    CString deflated = fromBytes(raw, 8);
    deflated.setSize(0);
    CHECK(deflated.zuncompress().isEmpty());

    const unsigned char one[1] = {0x82};
    CString shortBin = fromBytes(one, 1);
    CHECK(webSocketFixIncomingPacket(shortBin) == static_cast<unsigned int>(-1));

    const unsigned char sixtyFour[6] = {0x82, 0xff, 1, 2, 3, 4};
    CString shortLong = fromBytes(sixtyFour, 6);
    CHECK(webSocketFixIncomingPacket(shortLong) == static_cast<unsigned int>(-1));

    CString big(std::string(70000, 'Q'));
    webSocketFixOutgoingPacket(big);
    CHECK(big.length() == 70010);
    CHECK(static_cast<unsigned char>(big[0]) == 0x82);
    CHECK(static_cast<unsigned char>(big[1]) == 127);
}

static void checkGenerator()
{
    uint32_t state = 0xC0FFEEu;
    auto next = [&]() {
        state = state * 1664525u + 1013904223u;
        return state;
    };
    for (int n = 0; n < 200; ++n)
    {
        const int len = static_cast<int>(next() % 48);
        std::string raw(static_cast<size_t>(len), '\0');
        for (int i = 0; i < len; ++i)
            raw[static_cast<size_t>(i)] = static_cast<char>(next() & 0xff);
        CString s(raw);
        s.base64encode();
        s.base64decode();
        s.gtokenize();
        s.guntokenize();
        s.zuncompress();
        s.bzuncompress();
        s.encodesimple();
        s.decodesimple();
        s.codesimplefix0();
        s.find("a", -1);
        s.find("a", 0);
        s.right(INT_MIN);
        if (!s.isEmpty())
            s.replaceAll("", "z");
        CString frame = s;
        webSocketFixIncomingPacket(frame);
        if (len > 0)
        {
            CEncryption codec;
            codec.setGen(next() % 6);
            codec.reset(static_cast<uint8_t>(next()));
            CString body = s;
            codec.decrypt(body);
        }
    }
}

int main()
{
    checkInRange();
    checkOutOfRange();
    checkGenerator();

    if (failures != 0)
    {
        std::fprintf(stderr, "ParseBoundsTest: %d checks failed\n", failures);
        return 1;
    }
    std::printf("ParseBoundsTest: all checks passed\n");
    return 0;
}
