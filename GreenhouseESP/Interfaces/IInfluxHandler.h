#ifndef I_INFLUX_HANDLER_H
#define I_INFLUX_HANDLER_H

#include <string>

enum class InfluxStatus
{
    SUCCESS,
    ERR_INFLUX_CONNECTION
};

class IInfluxHandler
{
public:
    virtual ~IInfluxHandler() {}

    virtual const char* getLastErrorMessage() = 0;
};

#endif // I_INFLUX_HANDLER_H