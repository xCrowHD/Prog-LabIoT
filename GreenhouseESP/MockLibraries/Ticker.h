#ifndef MOCK_TICKER_H
#define MOCK_TICKER_H

#include <functional>

class Ticker {
public:
    Ticker() {}

    template <typename T>
    void attach(float seconds, T callback) {}
    void detach() {}
};

#endif // MOCK_TICKER_H