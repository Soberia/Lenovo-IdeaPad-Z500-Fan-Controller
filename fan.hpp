#ifndef FAN_H
#define FAN_H

auto constexpr VERSION = "0.1";
auto constexpr APP_NAME = "Lenovo IdeaPad Z500 Fan Controller";

constexpr UINT8 CONTROLLER_RUNTIME = 7;  // Second
constexpr UINT16 CONTROLLER_STEP = 1000; // Milisecond

constexpr BYTE FAN_DUST_CLEAN_ON_VALUE_1 = 0x01;
constexpr BYTE FAN_DUST_CLEAN_ON_VALUE_2 = 0x00;
constexpr BYTE FAN_DUST_CLEAN_OFF_VALUE_1 = 0x00;
constexpr BYTE FAN_DUST_CLEAN_OFF_VALUE_2 = 0x08;
constexpr BYTE FAN_DUST_CLEAN_OFF_REGISTER_1 = 0x01;
constexpr BYTE FAN_DUST_CLEAN_OFF_REGISTER_2 = 0xAB;
constexpr BYTE FAN_SPEED_MAX = 0x23;
constexpr BYTE FAN_SPEED_REGISTER = 0x06;
constexpr BYTE CPU_TEMPERATURE_REGISTER = 0xB0;

/**
 * Access Lenovo Energy Manager's Dust Removal feature directly
 * through Lenovo ACPI-Compliant Virtual Power Controller driver.
 */
class CleanDust
{
public:
    HANDLE hDevice;
    CleanDust();

    /**
     * Check availability of Lenovo ACPI-Compliant Virtual Power Controller driver.
     * @return Whether driver is installed or not.
     */
    static BOOL driverExistence();

    /** Close the driver resources */
    VOID close();

    /**
     * Dust Removal status.
     * @return Whether Dust Removal is running or not.
     * Throw an exception if device made no response.
     */
    BOOL status();

    /**
     * Activing or deactivating the Dust Removal.
     * @param status Whether Dust Removal should be enabled or not.
     */
    VOID run(BOOL status);

private:
    BOOL bResult;
    DWORD bytesReturned;
    CHAR *lpInBuffer = (CHAR *)operator new(0xC);
    CHAR *lpOutBuffer = lpInBuffer + 0x8;
};

#endif
