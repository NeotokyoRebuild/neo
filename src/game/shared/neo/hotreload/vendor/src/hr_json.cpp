#include "hr_json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace hr {
namespace json {

namespace {
const std::string kEmpty;
const Value kNull;
} // namespace

Value Value::boolean(bool b) { Value v; v.type_ = Type::Bool; v.bool_ = b; return v; }
Value Value::integer(int64_t i) { Value v; v.type_ = Type::Number; v.is_int_ = true; v.int_ = i; v.dbl_ = static_cast<double>(i); return v; }
Value Value::number(double d) { Value v; v.type_ = Type::Number; v.is_int_ = false; v.dbl_ = d; v.int_ = static_cast<int64_t>(d); return v; }
Value Value::string(const std::string& s) { Value v; v.type_ = Type::String; v.str_ = s; return v; }
Value Value::array() { Value v; v.type_ = Type::Array; return v; }
Value Value::object() { Value v; v.type_ = Type::Object; return v; }

int64_t Value::as_int(int64_t def) const {
    if (!is_number()) return def;
    return is_int_ ? int_ : static_cast<int64_t>(dbl_);
}
double Value::as_double(double def) const { return is_number() ? dbl_ : def; }
const std::string& Value::as_string() const { return is_string() ? str_ : kEmpty; }

size_t Value::size() const { return (is_array() || is_object()) ? items_.size() : 0; }
const Value& Value::at(size_t i) const { return (is_array() && i < items_.size()) ? items_[i] : kNull; }
Value& Value::push(const Value& v) {
    if (!is_array()) { *this = array(); }
    items_.push_back(v);
    return *this;
}
const Value* Value::get(const char* key) const {
    if (!is_object()) return nullptr;
    for (size_t i = 0; i < keys_.size(); ++i)
        if (keys_[i] == key) return &items_[i];
    return nullptr;
}
Value& Value::set(const std::string& key, const Value& v) {
    if (!is_object()) { *this = object(); }
    for (size_t i = 0; i < keys_.size(); ++i)
        if (keys_[i] == key) { items_[i] = v; return *this; }
    keys_.push_back(key);
    items_.push_back(v);
    return *this;
}

namespace {

void escape_into(std::string& out, const std::string& s) {
    out.push_back('"');
    for (unsigned char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        default:
            if (c < 0x20) {
                char buf[8];
                snprintf(buf, sizeof buf, "\\u%04x", c);
                out += buf;
            } else {
                out.push_back(static_cast<char>(c));
            }
        }
    }
    out.push_back('"');
}

void newline(std::string& out, int indent, int depth) {
    if (indent <= 0) return;
    out.push_back('\n');
    out.append(static_cast<size_t>(indent * depth), ' ');
}

} // namespace

void Value::dump_into(std::string& out, int indent, int depth) const {
    switch (type_) {
    case Type::Null: out += "null"; break;
    case Type::Bool: out += bool_ ? "true" : "false"; break;
    case Type::Number: {
        char buf[64];
        if (is_int_) {
            snprintf(buf, sizeof buf, "%lld", static_cast<long long>(int_));
        } else if (std::isfinite(dbl_)) {
            snprintf(buf, sizeof buf, "%.17g", dbl_);
        } else {
            snprintf(buf, sizeof buf, "null");
        }
        out += buf;
        break;
    }
    case Type::String: escape_into(out, str_); break;
    case Type::Array:
        out.push_back('[');
        for (size_t i = 0; i < items_.size(); ++i) {
            if (i) out.push_back(',');
            newline(out, indent, depth + 1);
            items_[i].dump_into(out, indent, depth + 1);
        }
        if (!items_.empty()) newline(out, indent, depth);
        out.push_back(']');
        break;
    case Type::Object:
        out.push_back('{');
        for (size_t i = 0; i < keys_.size(); ++i) {
            if (i) out.push_back(',');
            newline(out, indent, depth + 1);
            escape_into(out, keys_[i]);
            out += indent > 0 ? ": " : ":";
            items_[i].dump_into(out, indent, depth + 1);
        }
        if (!keys_.empty()) newline(out, indent, depth);
        out.push_back('}');
        break;
    }
}

std::string Value::dump(int indent) const {
    std::string out;
    dump_into(out, indent, 0);
    if (indent > 0) out.push_back('\n');
    return out;
}

// ---- parser -------------------------------------------------------------

namespace {

class Parser {
public:
    Parser(const std::string& text, std::string& err) : s_(text.data()), n_(text.size()), err_(err) {}

    bool parse_document(Value& out) {
        skip_ws();
        if (!parse_value(out, 0)) return false;
        skip_ws();
        if (pos_ != n_) return fail("trailing characters");
        return true;
    }

private:
    bool fail(const char* what) {
        char buf[128];
        snprintf(buf, sizeof buf, "json: %s at offset %zu", what, pos_);
        err_ = buf;
        return false;
    }

    void skip_ws() {
        while (pos_ < n_ && (s_[pos_] == ' ' || s_[pos_] == '\t' || s_[pos_] == '\n' || s_[pos_] == '\r')) ++pos_;
    }

    bool match_literal(const char* lit) {
        size_t len = strlen(lit);
        if (pos_ + len <= n_ && memcmp(s_ + pos_, lit, len) == 0) { pos_ += len; return true; }
        return false;
    }

    bool parse_value(Value& out, int depth) {
        if (depth > 64) return fail("nesting too deep");
        if (pos_ >= n_) return fail("unexpected end of input");
        char c = s_[pos_];
        if (c == '{') return parse_object(out, depth);
        if (c == '[') return parse_array(out, depth);
        if (c == '"') { std::string str; if (!parse_string(str)) return false; out = Value::string(str); return true; }
        if (c == 't') { if (match_literal("true")) { out = Value::boolean(true); return true; } return fail("bad literal"); }
        if (c == 'f') { if (match_literal("false")) { out = Value::boolean(false); return true; } return fail("bad literal"); }
        if (c == 'n') { if (match_literal("null")) { out = Value::null(); return true; } return fail("bad literal"); }
        if (c == '-' || (c >= '0' && c <= '9')) return parse_number(out);
        return fail("unexpected character");
    }

    bool parse_number(Value& out) {
        size_t start = pos_;
        bool is_int = true;
        if (s_[pos_] == '-') ++pos_;
        if (pos_ >= n_ || !(s_[pos_] >= '0' && s_[pos_] <= '9')) return fail("bad number");
        if (s_[pos_] == '0') { ++pos_; }
        else { while (pos_ < n_ && s_[pos_] >= '0' && s_[pos_] <= '9') ++pos_; }
        if (pos_ < n_ && s_[pos_] == '.') {
            is_int = false; ++pos_;
            if (pos_ >= n_ || !(s_[pos_] >= '0' && s_[pos_] <= '9')) return fail("bad fraction");
            while (pos_ < n_ && s_[pos_] >= '0' && s_[pos_] <= '9') ++pos_;
        }
        if (pos_ < n_ && (s_[pos_] == 'e' || s_[pos_] == 'E')) {
            is_int = false; ++pos_;
            if (pos_ < n_ && (s_[pos_] == '+' || s_[pos_] == '-')) ++pos_;
            if (pos_ >= n_ || !(s_[pos_] >= '0' && s_[pos_] <= '9')) return fail("bad exponent");
            while (pos_ < n_ && s_[pos_] >= '0' && s_[pos_] <= '9') ++pos_;
        }
        std::string tok(s_ + start, pos_ - start);
        if (is_int) {
            errno = 0;
            long long v = strtoll(tok.c_str(), nullptr, 10);
            if (errno == ERANGE) { out = Value::number(strtod(tok.c_str(), nullptr)); }
            else { out = Value::integer(v); }
        } else {
            out = Value::number(strtod(tok.c_str(), nullptr));
        }
        return true;
    }

    static void append_utf8(std::string& out, uint32_t cp) {
        if (cp < 0x80) { out.push_back(static_cast<char>(cp)); }
        else if (cp < 0x800) { out.push_back(static_cast<char>(0xC0 | (cp >> 6))); out.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
        else if (cp < 0x10000) { out.push_back(static_cast<char>(0xE0 | (cp >> 12))); out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F))); out.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
        else { out.push_back(static_cast<char>(0xF0 | (cp >> 18))); out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F))); out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F))); out.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
    }

    bool parse_hex4(uint32_t& v) {
        if (pos_ + 4 > n_) return fail("bad unicode escape");
        v = 0;
        for (int i = 0; i < 4; ++i) {
            char c = s_[pos_++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= static_cast<uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') v |= static_cast<uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= static_cast<uint32_t>(c - 'A' + 10);
            else return fail("bad unicode escape");
        }
        return true;
    }

    bool parse_string(std::string& out) {
        ++pos_; // opening quote
        while (true) {
            if (pos_ >= n_) return fail("unterminated string");
            unsigned char c = static_cast<unsigned char>(s_[pos_++]);
            if (c == '"') return true;
            if (c < 0x20) return fail("control character in string");
            if (c != '\\') { out.push_back(static_cast<char>(c)); continue; }
            if (pos_ >= n_) return fail("bad escape");
            char e = s_[pos_++];
            switch (e) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                uint32_t cp;
                if (!parse_hex4(cp)) return false;
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    if (pos_ + 6 <= n_ && s_[pos_] == '\\' && s_[pos_ + 1] == 'u') {
                        pos_ += 2;
                        uint32_t lo;
                        if (!parse_hex4(lo)) return false;
                        if (lo >= 0xDC00 && lo <= 0xDFFF) cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        else return fail("bad surrogate pair");
                    } else {
                        return fail("lone surrogate");
                    }
                }
                append_utf8(out, cp);
                break;
            }
            default: return fail("bad escape");
            }
        }
    }

    bool parse_array(Value& out, int depth) {
        ++pos_;
        out = Value::array();
        skip_ws();
        if (pos_ < n_ && s_[pos_] == ']') { ++pos_; return true; }
        while (true) {
            skip_ws();
            Value item;
            if (!parse_value(item, depth + 1)) return false;
            out.push(item);
            skip_ws();
            if (pos_ >= n_) return fail("unterminated array");
            if (s_[pos_] == ',') { ++pos_; continue; }
            if (s_[pos_] == ']') { ++pos_; return true; }
            return fail("expected , or ]");
        }
    }

    bool parse_object(Value& out, int depth) {
        ++pos_;
        out = Value::object();
        skip_ws();
        if (pos_ < n_ && s_[pos_] == '}') { ++pos_; return true; }
        while (true) {
            skip_ws();
            if (pos_ >= n_ || s_[pos_] != '"') return fail("expected key");
            std::string key;
            if (!parse_string(key)) return false;
            skip_ws();
            if (pos_ >= n_ || s_[pos_] != ':') return fail("expected :");
            ++pos_;
            skip_ws();
            Value item;
            if (!parse_value(item, depth + 1)) return false;
            out.set(key, item);
            skip_ws();
            if (pos_ >= n_) return fail("unterminated object");
            if (s_[pos_] == ',') { ++pos_; continue; }
            if (s_[pos_] == '}') { ++pos_; return true; }
            return fail("expected , or }");
        }
    }

    const char* s_;
    size_t n_;
    size_t pos_ = 0;
    std::string& err_;
};

} // namespace

bool parse(const std::string& text, Value& out, std::string& err) {
    Parser p(text, err);
    return p.parse_document(out);
}

int64_t get_int(const Value& obj, const char* key, int64_t def) {
    const Value* v = obj.get(key);
    return v ? v->as_int(def) : def;
}
bool get_bool(const Value& obj, const char* key, bool def) {
    const Value* v = obj.get(key);
    return v ? v->as_bool(def) : def;
}
std::string get_string(const Value& obj, const char* key, const std::string& def) {
    const Value* v = obj.get(key);
    return (v && v->is_string()) ? v->as_string() : def;
}

} // namespace json
} // namespace hr
