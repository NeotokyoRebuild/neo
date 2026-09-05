// Minimal JSON value, parser and serializer for the mailbox files.
// Deliberately tiny and dependency free so the vendored loader stays readable.
// Supports the full JSON grammar (RFC 8259); numbers are kept as int64 when
// they are written without fraction or exponent, otherwise as double.
#ifndef NTRE_HR_JSON_H
#define NTRE_HR_JSON_H

#include <cstdint>
#include <string>
#include <vector>

namespace hr {
namespace json {

enum class Type { Null, Bool, Number, String, Array, Object };

class Value {
public:
    Value() = default;

    static Value null() { return Value(); }
    static Value boolean(bool b);
    static Value integer(int64_t i);
    static Value number(double d);
    static Value string(const std::string& s);
    static Value array();
    static Value object();

    Type type() const { return type_; }
    bool is_null() const { return type_ == Type::Null; }
    bool is_bool() const { return type_ == Type::Bool; }
    bool is_number() const { return type_ == Type::Number; }
    bool is_string() const { return type_ == Type::String; }
    bool is_array() const { return type_ == Type::Array; }
    bool is_object() const { return type_ == Type::Object; }

    bool as_bool(bool def = false) const { return is_bool() ? bool_ : def; }
    int64_t as_int(int64_t def = 0) const;
    double as_double(double def = 0.0) const;
    const std::string& as_string() const;

    // Arrays and objects.
    size_t size() const;
    const Value& at(size_t i) const;          // arrays; returns a null value when out of range
    Value& push(const Value& v);              // arrays; returns *this
    const Value* get(const char* key) const;  // objects; nullptr when missing
    Value& set(const std::string& key, const Value& v); // objects; replaces; returns *this
    const std::vector<std::string>& keys() const { return keys_; }
    const std::vector<Value>& items() const { return items_; } // array items, or object values in key order

    // Serialize. indent <= 0 gives a single line.
    std::string dump(int indent = 2) const;

private:
    void dump_into(std::string& out, int indent, int depth) const;

    Type type_ = Type::Null;
    bool bool_ = false;
    bool is_int_ = false;
    int64_t int_ = 0;
    double dbl_ = 0.0;
    std::string str_;
    std::vector<std::string> keys_; // objects only
    std::vector<Value> items_;      // arrays and objects
};

// Parse a complete document. On failure returns false and describes the error with offset.
bool parse(const std::string& text, Value& out, std::string& err);

// Convenience accessors with defaults, for flat protocol objects.
int64_t get_int(const Value& obj, const char* key, int64_t def);
bool get_bool(const Value& obj, const char* key, bool def);
std::string get_string(const Value& obj, const char* key, const std::string& def);

} // namespace json
} // namespace hr

#endif
