# **💡 About**
Embedded Controller (EC) is a device responsible for feeding other parts of the system the electric voltage they need.
Therefore if EC pass more voltage to the cooling system, this cause fan to spins at higher speed.
`Lenovo IdeaPad Z500` (all variants) uses `ENE KB9012QF A3` EC. For controlling the EC we can change the EC registers to achieve functionality we want,
but for this model and according to its datasheet, I couldn't find the registers which was dedicated to controlling the fan speed.
So as a last resort, I reverse engineering the `Lenovo Energy Manager` (old version for Windows 8) software to find how `Dust Removal` feature of this app
actually works and build this program with it. This software communicate to the EC through `Lenovo ACPI-Compliant Virtual Power Controller` kernel driver and if you're interested you can study the source code.

At the end this program only periodically active `Dust Removal` to force the fan to start spinning.
Here's how this works:
> `Lenovo Energy Manager`'s default `Dust Removal` procedure takes 2 minute to complete. During this process every 7 second, fan will be stopped completely
for 2 second before spins again for another 7 second until the whole procedure is over. The best we can do for reducing fan inactiveness time is to
let the fan spins for 7 second and then stop the `Dust Removal` procedure and after 1 second continue the procedure again.
By this way, during that 1 second, fan won't stop completely and still is spinning by normal speed.
However `Dust Removal` procedure is controlled automatically by EC itself and sometimes may suddenly stop the procedure during that 7 second
of spinning and this leads to fan stop spinning for something from 1 to 7 seconds. For workaround about this problem we can check the `Dust Removal` status
registers in EC repeatedly during that 7 second of spinning and rewrite them when procedure is stopped, but I have
found no evidence to prove this is actually working!
All of this means the fan might stops working from 1 to 7 seconds sometimes!

# **📋 How to use**
First you need `Lenovo ACPI-Compliant Virtual Power Controller` driver. If you installed `Lenovo Energy Manager` before, you should have it already.
You can check it with `Device Manager` under `System devices` to make sure the driver is present.
However if you don't have the driver, you can [get it from Lenovo support website](https://support.lenovo.com/ph/en/downloads/ds548269-lenovo-energy-management-for-windows-10-64-bit-ideapad-5-14alc05).
</br>
Second this program has to run with administrator privileges to work properly.
You can [download it from here](https://github.com/Soberia/Lenovo-IdeaPad-Z500-Fan-Controller/releases).

Now you can just open `FanController.exe` and fan should start working and you should see this on the console:
```cmd
FAN STATUS = ACTIVE
FAN SPEED  = 71%
CPU TEMP   = 52c
```
If you want the fan works only after CPU temperature exceeds certain value, you should run the program with this parameter:
```cmd
FanController.exe --temperature 70
```
If you want the program always runs with system startup, press `Win + R` and type `shell:startup`,
then make a new shortcut on the opened window and put this on it: (you should insert compelete program path)
```cmd
C:\FanController.exe --interval 0 --temperature 70
```
And now the program always runs at boot time and in background mode, and only enables the fan when CPU temperature exceeds 70°C.
For disable it just remove the shortcut file.
</br>
For more information about parameters, you can try this:
```cmd
FanController.exe --help
```

# **🔨 Build from source**
You need to compile your app with `c++17` flag or newer versions.
</br>
You need `WinRing0` driver to access the hardware, make sure you place `WinRing0x64.sys` or `WinRing0.sys` beside your binary files.

# **⚠️ Disclaimer**
**Author of this software is not responsible for damage of any kind, use it at your own risk!**
</br>
This program shouldn't be used with any Lenovo laptop model except `IdeaPad Z500`.
</br>
But if you're brave enough to test this on your system and that works for you, [let me know](https://github.com/Soberia/Lenovo-IdeaPad-Z500-Fan-Controller/issues)
to include your laptop model for other people that may come to this.
