#include <windows.h>

#include "ec.hpp"
#include "driver.hpp"

EmbeddedController::EmbeddedController(UINT16 retry, UINT16 timeout)
{
    this->retry = retry;
    this->timeout = timeout;

    this->driver = Driver();
    if (this->driver.initialize())
        this->driverLoaded = TRUE;

    this->driverFileExist = driver.driverFileExist;
}

VOID EmbeddedController::close()
{
    this->driver.deinitialize();
    this->driverLoaded = FALSE;
}

BYTE EmbeddedController::readByte(BYTE bRegister)
{
    BYTE result = 0x00;
    this->operation(READ, bRegister, &result);
    return result;
}

BOOL EmbeddedController::writeByte(BYTE bRegister, BYTE value)
{
    return this->operation(WRITE, bRegister, &value);
}

BOOL EmbeddedController::operation(BYTE mode, BYTE bRegister, BYTE *value)
{
    BOOL isRead = mode == READ;
    BYTE operationType = isRead ? RD_EC : WR_EC;

    for (UINT16 i = 0; i < this->retry; i++)
        if (this->status(EC_IBF)) // Wait until IBF is free
        {
            this->driver.writeIoPortByte(EC_SC, operationType); // Write operation type to the Status/Command port
            if (this->status(EC_IBF))                           // Wait until IBF is free
            {
                this->driver.writeIoPortByte(EC_DATA, bRegister); // Write register address to the Data port
                if (this->status(EC_IBF))                         // Wait until IBF is free
                    if (isRead)
                    {
                        if (this->status(EC_OBF)) // Wait until OBF is full
                        {
                            *value = this->driver.readIoPortByte(EC_DATA); // Read from the Data port
                            return TRUE;
                        }
                    }
                    else
                    {
                        this->driver.writeIoPortByte(EC_DATA, *value); // Write to the Data port
                        return TRUE;
                    }
            }
        }

    return FALSE;
}

BOOL EmbeddedController::status(BYTE flag)
{
    BOOL done = flag == EC_OBF ? 0x01 : 0x00;
    for (UINT16 i = 0; i < this->timeout; i++)
    {
        BYTE result = this->driver.readIoPortByte(EC_SC);
        // First and second bit of returned value represent
        // the status of OBF and IBF flags respectively
        if (((done ? ~result : result) & flag) == 0)
            return TRUE;
    }

    return FALSE;
}
