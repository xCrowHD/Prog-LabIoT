#ifndef MOCK_ARDUINO_JSON_H
#define MOCK_ARDUINO_JSON_H

#include <string>
#include <iostream>
#include <cmath>
#include <cstring>

#ifndef ARDUINO
inline size_t strlcpy(char *dst, const char *src, size_t siz)
{
    size_t len = std::strlen(src);
    if (siz > 0)
    {
        size_t n = (len >= siz) ? siz - 1 : len;
        std::memcpy(dst, src, n);
        dst[n] = '\0';
    }
    return len;
}
#endif

class DeserializationError
{
public:
    operator bool() const { return false; }
    const char *c_str() const { return "Ok"; }
};

// Dichiarazioni anticipate
class JsonObject;
class JsonArray;

class JsonVariant
{
public:
    template <typename T>
    JsonVariant &operator=(const T &value) { return *this; }

    JsonVariant operator[](const char *key) const { return JsonVariant(); }
    JsonVariant operator[](const std::string &key) const { return JsonVariant(); }

    // Operatori di conversione espliciti e ampliati per evitare ambiguità
    operator int() const { return 0; }
    operator unsigned int() const { return 0; } // Risolve l'ambiguità con uint32_t
    operator float() const { return 0.0f; }
    operator double() const { return 0.0; }
    operator bool() const { return false; }
    operator const char *() const { return ""; }
    operator std::string() const { return ""; }

    // Supporto al metodo .to<T>() (es. doc["actions"].to<JsonArray>())
    template <typename T>
    T to() { return T(); }

    // conversioni verso i tipi contenitore
    operator JsonObject() const;
    operator JsonArray() const;

    bool operator==(const char *s) const { return true; }
    friend bool operator==(const char *s, const JsonVariant &j) { return true; }
};

class JsonObject
{
public:
    JsonVariant operator[](const char *key) const { return JsonVariant(); }
    bool containsKey(const char *key) const { return true; }
};

class JsonArray
{
public:
    // Permette chiamate come actionsJson.add(_actions[i])
    template <typename T>
    void add(const T &value) {}

    JsonVariant operator[](size_t index) const { return JsonVariant(); }
};

// Definizioni degli operatori di conversione posticipate per conoscere i tipi completi
inline JsonVariant::operator JsonObject() const
{
    return JsonObject();
}

inline JsonVariant::operator JsonArray() const
{
    return JsonArray();
}

template <size_t N>
class StaticJsonDocument
{
public:
    JsonVariant operator[](const char *key) const { return JsonVariant(); }
    operator JsonObject() const { return JsonObject(); }
    operator JsonVariant() const { return JsonVariant(); }

    template <typename T>
    T as() const { return T(); }
    bool containsKey(const char *key) const { return true; }
    void clear() {}
};

// Overload di deserializzazione
template <typename T>
DeserializationError deserializeJson(T &doc, const std::string &input) { return DeserializationError(); }
template <typename T>
DeserializationError deserializeJson(T &doc, const char *input) { return DeserializationError(); }
template <typename T>
DeserializationError deserializeJson(T &doc, char *input, size_t len) { return DeserializationError(); }
template <typename T>
DeserializationError deserializeJson(T &doc, const char *input, size_t len) { return DeserializationError(); }

// Overload di serializzazione
template <typename T>
size_t serializeJson(const T &doc, char *buffer, size_t bufSize)
{
    if (bufSize > 0)
        buffer[0] = '\0';
    return 0;
}
template <typename T, size_t N>
size_t serializeJson(const T &doc, char (&buffer)[N])
{
    buffer[0] = '\0';
    return 0;
}
template <typename T>
size_t serializeJson(const T &doc, std::ostream &output) { return 0; }

#endif // MOCK_ARDUINO_JSON_H