#pragma once

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string_view>

struct Json {
    const char *value;
    size_t len;
};

struct JsonStream {
    virtual void object(const std::string_view &parent, const std::string_view &key) = 0;
    virtual void array(const std::string_view &parent, const std::string_view &key) = 0;
    virtual void value(const std::string_view &parent, const std::string_view &key,
                       const std::string_view &val) = 0;
    virtual void value(const std::string_view &parent, const std::string_view &key, int val) = 0;
    virtual void value(const std::string_view &parent, const std::string_view &key, double val) = 0;
};

inline static void parseArray(JsonStream &js, const std::string_view &parent,
                              const std::string_view &mkey, const char *str, size_t len, size_t &i);
inline static void parseObject(JsonStream &js, const std::string_view &parent,
                               const std::string_view &mkey, const char *str, size_t len,
                               size_t &i);

inline static bool validDigit(char c) { return (c >= '0' && c <= '9') || c == '-'; }

inline static void parseNumber(JsonStream &js, const std::string_view &parent,
                               const std::string_view &mkey, const char *str, size_t len,
                               size_t &i) {
    assert(validDigit(str[i]));
    const char *tok = str + i;
    bool floatingPoint{false};
    for (; i < len; ++i) {
        if (str[i] == '.') {
            floatingPoint = true;
        } else if (!validDigit(str[i])) {
            break;
        }
    }
    if (floatingPoint) {
        double val = atof(tok);
        js.value(parent, mkey, val);
    } else {
        int val = atoi(tok);
        js.value(parent, mkey, val);
    }
    --i;
}

inline static void parseString(JsonStream &js, const std::string_view &parent,
                               const std::string_view &mkey, const char *str, size_t len,
                               size_t &i) {
    assert(str[i] == '"');
    ++i;
    const char *tok = str + i;
    for (; i < len; ++i) {
        if (str[i] == '"') {
            js.value(parent, mkey, {tok, size_t((str + i) - tok)});
            break;
        }
    }
}

inline static void parseKey(JsonStream &js, const std::string_view &parent, const char *str,
                            size_t len, size_t &i) {
    assert(str[i] == '"');
    ++i;
    const char *key = str + i;
    for (; i < len; ++i) {
        if (str[i] == '"') {
            break;
        }
    }
    ++i;
    std::string_view kkey = std::string_view(key, (str + i - 1 - key));
    for (; i < len; ++i) {
        switch (str[i]) {
        case '{':
            parseObject(js, parent, kkey, str, len, i);
            return;
        case '[':
            parseArray(js, parent, kkey, str, len, i);
            return;
        case '"':
            parseString(js, parent, kkey, str, len, i);
            return;
        default:
            if (validDigit(str[i])) {
                parseNumber(js, parent, kkey, str, len, i);
                return;
            }
            break;
        }
    }
}

inline static void parseObject(JsonStream &js, const std::string_view &parent,
                               const std::string_view &mkey, const char *str, size_t len,
                               size_t &i) {
    assert(str[i] == '{');
    js.object(parent, mkey);
    ++i;
    for (; i < len; ++i) {
        switch (str[i]) {
        case '}':
            return;
        case '"':
            parseKey(js, parent, str, len, i);
            break;
        default:
            break;
        }
    }
}

inline static void parseArray(JsonStream &js, const std::string_view &parent,
                              const std::string_view &mkey, const char *str, size_t len,
                              size_t &i) {
    assert(str[i] == '[');
    js.array(parent, mkey);
    ++i;
    for (; i < len; ++i) {
        switch (str[i]) {
        case '{':
            parseObject(js, mkey, {}, str, len, i);
            break;
        case '[':
            parseArray(js, mkey, {}, str, len, i);
            break;
        case ']':
            return;
        default:
            if (validDigit(str[i])) {
                parseNumber(js, mkey, {}, str, len, i);
            }
            break;
        }
    }
}

inline static JsonStream &loadJson(JsonStream &js, const char *str, size_t len) {
    for (size_t i{0}; i < len; ++i) {
        switch (str[i]) {
        case '{':
            parseObject(js, {}, {}, str, len, i);
            break;
        case '[':
            parseArray(js, {}, {}, str, len, i);
            break;
        default:
            break;
        }
    }
    return js;
}

#define JSON_TEST_A "[{\"test\":[{\"inner\":0.0,\"second\":\"string\"}]}]"

struct TestJsonStream : public JsonStream {
    virtual void object(const std::string_view &parent, const std::string_view &key) override {
        printf("object: %.*s.%.*s\n", (int)parent.length(), parent.data(), (int)key.length(),
               key.data());
    }
    virtual void array(const std::string_view &parent, const std::string_view &key) override {
        printf("array: %.*s.%.*s\n", (int)parent.length(), parent.data(), (int)key.length(),
               key.data());
    }
    virtual void value(const std::string_view &parent, const std::string_view &key,
                       const std::string_view &val) override {
        printf("value: %.*s.%.*s=%.*s\n", (int)parent.length(), parent.data(), (int)key.length(),
               key.data(), (int)val.length(), val.data());
    }
    virtual void value(const std::string_view &parent, const std::string_view &key,
                       int val) override {
        printf("value: %.*s.%.*s=%d\n", (int)parent.length(), parent.data(), (int)key.length(),
               key.data(), val);
    }
    virtual void value(const std::string_view &parent, const std::string_view &key,
                       double val) override {
        printf("value: %.*s.%.*s=%f\n", (int)parent.length(), parent.data(), (int)key.length(),
               key.data(), val);
    }
};

inline static void testJson() {
    TestJsonStream testJsonStream;
    loadJson(testJsonStream, JSON_TEST_A, strlen(JSON_TEST_A));
    printf("\n");
}