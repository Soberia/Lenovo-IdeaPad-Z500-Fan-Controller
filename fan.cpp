#include <vector>
#include <string>
#include <thread>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <tchar.h>
#include <windows.h>
#include <SetupAPI.h>

#include "fan.hpp"
#include "ec.hpp"
#include "driver.hpp"

#pragma comment(lib, "SetupAPI.lib")

EmbeddedController ec;
CleanDust cleanDust;
INT8 temperature = 0;
INT interval = 1000;
BOOL driver = TRUE;

class InputParser
{
public:
    InputParser(INT &argc, CHAR **argv)
    {
        for (INT i = 1; i < argc; ++i)
            this->inputs.push_back(std::string(argv[i]));
    }

    const std::string &value(const std::string &option)
    {
        std::vector<std::string>::const_iterator itr = std::find(this->inputs.begin(), this->inputs.end(), option);
        if (itr != this->inputs.end() && ++itr != this->inputs.end())
            return *itr;
        return this->empty;
    }

    bool parameter(const std::string &option)
    {
        return std::find(this->inputs.begin(), this->inputs.end(), option) != this->inputs.end();
    }

private:
    std::vector<std::string> inputs;
    std::string empty = "";
};

CleanDust::CleanDust()
{
    this->hDevice = CreateFileW(
        L"\\\\.\\EnergyDrv",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
}

BOOL CleanDust::driverExistence()
{
    SP_DEVINFO_DATA deviceInfoData;
    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(NULL, L"ACPI\\VPC2004", NULL, DIGCF_ALLCLASSES);
    deviceInfoData.cbSize = sizeof(deviceInfoData);

    return deviceInfoSet != INVALID_HANDLE_VALUE &&
           SetupDiEnumDeviceInfo(deviceInfoSet, 0, &deviceInfoData);
}

VOID CleanDust::close()
{
    CloseHandle(this->hDevice);
}

BOOL CleanDust::status()
{
    DWORD bufferSize = 0x4;
    CHAR flag = this->lpInBuffer[8];
    *(DWORD *)this->lpInBuffer = 0xE;

    this->bResult = DeviceIoControl(
        this->hDevice,
        0x831020C4,
        this->lpInBuffer,
        bufferSize,
        this->lpOutBuffer,
        bufferSize,
        &this->bytesReturned,
        NULL);

    if (this->bResult)
        if (*this->lpOutBuffer & 1 && *this->lpOutBuffer != flag && (INT) * this->lpOutBuffer == 0x3)
            return TRUE;
        else
            return FALSE;

    throw std::invalid_argument("No Response");
}

VOID CleanDust::run(BOOL status)
{
    *(DWORD *)this->lpInBuffer = 0x6;
    *((DWORD *)this->lpInBuffer + 2) = status ? 0x1 : 0x0;

    DeviceIoControl(this->hDevice, 0x831020C0, this->lpInBuffer, 0xC, NULL, 0x0, &this->bytesReturned, NULL);
}

/** Print Dust Removal activeness, CPU temperature and fan speed */
VOID monitor()
{
    BYTE cpuTemperature;
    BYTE cpuTemperatureCopy;
    FLOAT fanSpeed;
    FLOAT fanSpeedCopy;
    FLOAT fanSpeedMaxCoefficient = (FLOAT)100 / (FLOAT)FAN_SPEED_MAX;

    COORD cursorPosition;
    cursorPosition.X = 0;
    cursorPosition.Y = 0;

    system("cls");
    while (TRUE)
    {
        cpuTemperatureCopy = ec.readByte(CPU_TEMPERATURE_REGISTER);
        fanSpeedCopy = ec.readByte(FAN_SPEED_REGISTER) * fanSpeedMaxCoefficient;
        // Using previous values if read operation failed
        cpuTemperature = cpuTemperatureCopy ? cpuTemperatureCopy : cpuTemperature;
        fanSpeed = fanSpeedCopy ? fanSpeedCopy : fanSpeed;

        std::cout << "FAN STATUS = " << (cpuTemperature >= temperature ? "\033[1;32mACTIVE " : "\033[1;31mPASSIVE") << "\033[0m" << std::endl
                  << "FAN SPEED  = " << std::left << std::setw(4) << std::to_string((UINT8)fanSpeed > 100 ? 100 : (UINT8)fanSpeed) + "%" << std::endl
                  << "CPU TEMP   = " << std::left << std::setw(4) << std::to_string((UINT8)cpuTemperature) + "c" << std::endl;
        Sleep(interval);

        // Moving console pointer to the top left to prevent screen flickering
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursorPosition);
    }
}

/**
 * Control Dust Removal procedure.
 * 
 * Default Dust Removal procedure takes 2 minute to complete.
 * During this process every 7 second, fan will be stopped completely
 * for 2 second before spins again for another 7 second until the
 * whole procedure is over.
 * The best we can do for reducing fan inactiveness time is to
 * let the fan spins for 7 second and then stop the Dust Removal
 * procedure and after 1 second continue the procedure again.
 * By this way, during that 1 second, fan won't stop completely
 * and still is spinning by normal speed.
 * However Dust Removal procedure is controlled automatically
 * by embedded controller itself and sometimes may suddenly stop
 * the procedure during that 7 second of spinning and this leads to
 * fan stop spinning for something from 1 to 7 seconds.
 * For workaround about this problem we can check the Dust Removal status
 * registers in embedded controller repeatedly during that 7 second
 * of spinning and rewrite them when procedure is stopped, but I have
 * found no evidence to prove this is actually working!
*/
VOID controller()
{
    BOOL checkTemperature = driver && temperature;
    while (TRUE)
    {
        if (!checkTemperature || (checkTemperature && ec.readByte(CPU_TEMPERATURE_REGISTER) >= temperature))
        {
            cleanDust.run(TRUE);

            if (driver)
                for (UINT8 i = 0; i < CONTROLLER_RUNTIME; i++)
                {
                    Sleep(CONTROLLER_STEP);
                    // Resetting the procedure when Dust Removal process suddenly stopped.
                    if ((ec.readByte(FAN_DUST_CLEAN_OFF_REGISTER_1) == FAN_DUST_CLEAN_OFF_VALUE_1) &&
                        (ec.readByte(FAN_DUST_CLEAN_OFF_REGISTER_2) == FAN_DUST_CLEAN_OFF_VALUE_2))
                    {
                        ec.writeByte(FAN_DUST_CLEAN_OFF_REGISTER_1, FAN_DUST_CLEAN_ON_VALUE_1);
                        ec.writeByte(FAN_DUST_CLEAN_OFF_REGISTER_2, FAN_DUST_CLEAN_ON_VALUE_2);
                        Sleep(CONTROLLER_STEP);
                        break;
                    }
                }
            else
                Sleep(CONTROLLER_RUNTIME * 1000);

            cleanDust.run(FALSE);
        }

        Sleep(CONTROLLER_STEP);
    }
}

BOOL WINAPI exitHandler(DWORD fdwCtrlType)
{
    if (fdwCtrlType == CTRL_C_EVENT || fdwCtrlType == CTRL_CLOSE_EVENT)
    {
        if (driver)
            ec.close();

        cleanDust.run(FALSE);
        cleanDust.close();
        std::cout << "Fan controller is stopped successfully" << std::endl;
        exit(0);
    }
    return TRUE;
}

INT main(INT argc, CHAR *argv[])
{
    // Parsing command line options
    InputParser parser = InputParser(argc, argv);
    if (parser.parameter("-h") || parser.parameter("--help"))
    {
        std::cout << std::endl
                  << APP_NAME << " v" << VERSION
                  << std::endl
                  << "Usage: fan-controller.exe [OPTIONS]...\n\n"
                  << "Options:\n"
                  << "  -h, --help                      Display this help and exit\n"
                  << "  -v, --version                   Display application version and exit\n"
                  << "  -i, --interval <NUM>            CPU temperature and fan speed display interval,\n"
                  << "                                  default is 1000ms (0 to enable silent mode)\n"
                  << "  -t, --temperature <NUM>         Enable maximum fan speed only after CPU temperature (Celsius) exceed this value,\n"
                  << "                                  max acceptable value is CPU Tjunction temperature which is 105c,\n"
                  << "                                  default is 0 (always enable)\n";
        return 0;
    }
    else if (parser.parameter("-v") || parser.parameter("--version"))
    {
        std::cout << APP_NAME << " v" << VERSION << std::endl;
        return 0;
    }
    if (parser.parameter("-i") || parser.parameter("--interval"))
    {
        std::string option = parser.value("-i");
        if (option == "")
            option = parser.value("--temperature");
        interval = atoi(option.c_str());
    }
    if (parser.parameter("-t") || parser.parameter("--temperature"))
    {
        std::string option = parser.value("-t");
        if (option == "")
            option = parser.value("--temperature");
        temperature = atoi(option.c_str());
    }
    if (interval < 0 || temperature < 0 || temperature > 105)
    {
        std::cerr << "Invalid Parameter" << std::endl;
        return 1;
    }

    // Chechking device support
    cleanDust = CleanDust();
    BOOL supported = TRUE;
    if (cleanDust.hDevice == INVALID_HANDLE_VALUE)
    {
        supported = FALSE;
        if (!cleanDust.driverExistence())
        {
            std::cerr << "\"Lenovo ACPI-Compliant Virtual Power Controller\" driver is not installed on your device"
                      << std::endl
                      << std::endl;
            system("pause");
            return 1;
        }
    }
    else
        try
        {
            cleanDust.status();
        }
        catch (std::invalid_argument)
        {
            supported = FALSE;
        }

    if (!supported)
    {
        std::cerr << "Your device is not supported"
                  << std::endl
                  << std::endl;
        system("pause");
        return 1;
    }

    // Checking existence of "WinRing0" driver file
    std::string driverLoadFailedMsg = "Fan Controller might not work correctly!";
    ec = EmbeddedController();
    if (!ec.driverFileExist)
    {
        driver = FALSE;
#ifdef _WIN64
        std::wcerr << L"\"" OLS_DRIVER_FILE_NAME_WIN_NT_X64 "\"";
#else
        BOOL wow64 = FALSE;
        IsWow64Process(GetCurrentProcess(), &wow64);
        std::wcerr << L"\"" << (wow64 ? OLS_DRIVER_FILE_NAME_WIN_NT_X64 : OLS_DRIVER_FILE_NAME_WIN_NT) << "\"";
#endif
        std::cerr << " not found, " + driverLoadFailedMsg << std::endl;
    }
    else if (!ec.driverLoaded)
    {
        driver = FALSE;
        std::cerr << "Could not load the driver, make sure you have administrator privileges. "
                  << driverLoadFailedMsg
                  << std::endl;
    }

    // Attaching the exit handler and running the app
    SetConsoleCtrlHandler(exitHandler, TRUE);
    if (driver && interval)
    {
        std::thread monitorThread(monitor);
        std::thread controllerThread(controller);
        monitorThread.join();
        controllerThread.join();
    }

    // Monitor is disabled, running app in the main thread
    ShowWindow(GetConsoleWindow(), SW_HIDE); // Hiding the console
    controller();
}
