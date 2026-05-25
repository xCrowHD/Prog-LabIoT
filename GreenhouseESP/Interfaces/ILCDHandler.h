#ifndef I_LCD_HANDLER_H
#define I_LCD_HANDLER_H

class ILCDHandler
{
public:
    virtual ~ILCDHandler() {} 

    virtual void addMessage(const char *msgOne, const char *msgSec = "") = 0;
};

#endif // I_LCD_HANDLER