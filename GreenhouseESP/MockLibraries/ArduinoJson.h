#ifndef MOCK_ARDUINO_JSON_H
#define MOCK_ARDUINO_JSON_H

#include <string>
#include <iostream>
#include <cmath>

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

// Dichiarazione anticipata delle classi per evitare l'errore "expected type-specifier"
class JsonObject;

class JsonVariant
{
public:
    template <typename T>
    JsonVariant &operator=(const T &value) { return *this; }

    JsonVariant operator[](const char *key) const { return JsonVariant(); }
    JsonVariant operator[](const std::string &key) const { return JsonVariant(); }

    // Operatori di conversione non ambigui
    operator int() const { return 0; }
    operator float() const { return 0.0f; }
    operator bool() const { return false; }
    operator const char *() const { return ""; }
    operator std::string() const { return ""; }

    // Risolve l'ambiguità nei confronti come strcmp o uguaglianze dirette
    operator JsonObject() const;
    bool operator==(const char *s) const { return true; }
    friend bool operator==(const char *s, const JsonVariant &j) { return true; }
};

class JsonObject
{
public:
    JsonVariant operator[](const char *key) const { return JsonVariant(); }
    bool containsKey(const char *key) const { return true; }
};

inline JsonVariant::operator JsonObject() const
{
    return JsonObject();
}

template <size_t N>
class StaticJsonDocument
{
public:
    JsonVariant operator[](const char *key) const { return JsonVariant(); }
    operator JsonObject() const { return JsonObject(); }
    operator JsonVariant() const { return JsonVariant(); } // Permette il passaggio a isAddressedToMe(doc)

    template <typename T>
    T as() const { return T(); }
    bool containsKey(const char *key) const { return true; }
    void clear() {}
};

// Overload di deserializzazione a 2 e 3 argomenti richiesti da MqttHandler
template <typename T>
DeserializationError deserializeJson(T &doc, const std::string &input) { return DeserializationError(); }
template <typename T>
DeserializationError deserializeJson(T &doc, const char *input) { return DeserializationError(); }
template <typename T>
DeserializationError deserializeJson(T &doc, char *input, size_t len) { return DeserializationError(); }
template <typename T>
DeserializationError deserializeJson(T &doc, const char *input, size_t len) { return DeserializationError(); }

// Overload di serializzazione a 2 argomenti per i buffer di char tradizionali
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