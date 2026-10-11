// Checks the shared data readers: Scripting::JSON's typed reads (missing keys and other
// types fall back) and StrUtils' text formats for numbers, vectors and rects.
#include "../src/Scripting/JSONReader.h"
#include "../src/StrUtils.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

bool near(float a, float b) { return std::fabs(a - b) < 0.0001f; }
bool near(vector2 a, vector2 b) { return near(a.x, b.x) && near(a.y, b.y); }
bool same(const RECT& a, const RECT& b) { return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom; }

} // namespace

int main()
{
    using namespace Scripting;
    using namespace StrUtils;

    simdjson::dom::parser parser;
    simdjson::dom::element json = parser.parse(simdjson::padded_string(std::string(R"({
        "decimal": 2.5, "integer": 3, "big": 18446744073709551615, "negative": -7,
        "yes": true, "no": false, "one": 1, "name": "Idle", "list": [1, 2]
    })")));

    // Numbers read as either type; anything else, or a missing key, is the fallback.
    assert(near(JSON::readFloat(json["decimal"]), 2.5f));
    assert(near(JSON::readFloat(json["integer"]), 3.0f));
    assert(near(JSON::readFloat(json["name"], 9.0f), 9.0f));
    assert(near(JSON::readFloat(json["missing"], 4.0f), 4.0f));
    assert(JSON::readInt(json["decimal"]) == 3); // rounded
    assert(JSON::readInt(json["negative"]) == -7);
    assert(JSON::readInt64(json["decimal"]) == 3);
    assert(JSON::readInt64(json["missing"], -1) == -1);
    assert(JSON::readBool(json["yes"]) && !JSON::readBool(json["no"], true));
    assert(JSON::readBool(json["one"]));
    assert(JSON::readBool(json["missing"], true) && !JSON::readBool(json["name"]));
    assert(JSON::readString(json["name"]) == "Idle");
    assert(JSON::readString(json["integer"], "none") == "none");
    assert(JSON::readString(json["missing"]).empty());

    // An element already in hand reads the same way.
    for (simdjson::dom::element item : json["list"]) {
        assert(JSON::readInt64(item) >= 1);
    }

    // Any scalar as text.
    assert(JSON::readText(json["name"]) == "Idle");
    assert(JSON::readText(json["yes"]) == "true");
    assert(JSON::readText(json["integer"]) == "3");
    assert(JSON::readText(json["decimal"]) == "2.5");
    assert(JSON::readText(json["big"]) == "18446744073709551615");
    assert(JSON::readText(json["list"]).empty() && JSON::readText(json["missing"]).empty());

    // Numbers in text: a leading number, otherwise the fallback.
    assert(near(floatFromString("1.5", 0.0f), 1.5f));
    assert(near(floatFromString(" 2px", 0.0f), 2.0f));
    assert(near(floatFromString("", 7.0f), 7.0f) && near(floatFromString("fast", 7.0f), 7.0f));
    assert(intFromString("42", 0) == 42 && intFromString("-3.9", 0) == -3);
    assert(intFromString("", 5) == 5 && intFromString("x", 5) == 5);

    // "x,y"
    assert(near(vectorFromString("0,-8"), vector2(0, -8)));
    assert(near(vectorFromString(" 4.5 , 6 "), vector2(4.5f, 6)));
    assert(near(vectorFromString("3", vector2(1, 1)), vector2(1, 1)));
    assert(near(vectorFromString("", vector2(2, 2)), vector2(2, 2)));
    assert(vectorToString(vector2(0, -8)) == "0,-8" && vectorToString(vector2(1.5f, 2)) == "1.5,2");
    assert(near(vectorFromString(vectorToString(vector2(-3.25f, 7))), vector2(-3.25f, 7)));

    // "x,y,width,height", keyed or positional; empty text is the fallback.
    assert(same(rectFromString("64,0,64,80"), RECT{ 64, 0, 128, 80 }));
    assert(same(rectFromString("{ X=8, Y=16, Width=32, Height=48 }"), RECT{ 8, 16, 40, 64 }));
    assert(same(rectFromString("", RECT{ -1, -1, -1, -1 }), RECT{ -1, -1, -1, -1 }));
    assert(same(rectFromString(""), RECT{}));
    assert(rectToString(RECT{ 64, 0, 128, 80 }) == "64,0,64,80");
    assert(same(rectFromString(rectToString(RECT{ 5, 6, 12, 20 })), RECT{ 5, 6, 12, 20 }));

    std::printf("scripting test passed\n");
    return 0;
}
