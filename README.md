# MeePlan101

## Installing on to Wio Terminal
### How to build using PlatformIO
1. Install [VSCode](https://code.visualstudio.com/docs/setup/setup-overview).
2. Get [PlatformIO IDE](https://docs.platformio.org/en/latest/integration/ide/vscode.html#installation) for VSCode.
3. Clone/Download this project as zip using the Code button and open this project from PIO Home tab.
 ![image](https://user-images.githubusercontent.com/3771813/145669259-bd7ebbbc-3e0a-4cae-b494-95e01d9874e2.png)
 ![image](https://user-images.githubusercontent.com/3771813/145669310-6453a1ab-97e5-49d5-87a3-d19f233b25ac.png)
4. Connect your Wio Terminal to your PC.
5. In PlatformIO button, choose _**Upload**_.

     ![image](https://user-images.githubusercontent.com/3771813/137593065-921381fa-5a84-4cdb-9876-b036e0041c28.png)
* \**optional*\* In `platformio.ini`, change monitor_port to match your setup

## Connecting Wifi
When booting up for the first time, grab your phone and look for `MeePlanTerm` in your device wifi setting to connect Wio Terminal to Wifi
Select Wifi and enter in your Wifi password then submit.

*If no login menu popup, open your browser and go to `192.168.1.1`*


## Finding Device ID
Device ID is shown on the first boot but if your already booted once,
the device ID of your Wio Terminal is located in the `About MeePlan` section in the setting page (you can only access this when your device is connected).

Make sure to restart Wio Terminal after entering device ID on website.

## Troubleshooting
Tl;dr Turning it off and on again usually works / check your wifi setting.

* *Wio Terminal Froze or show white page* - Try turning Wio Terminal off and on again.
* *Stuck on Wifi screen* - Try connecting to wifi using the step above then wait a sec(if no wifi shows up means its trying to connect), if that doesn't work have you try turning it off and on again?
* *Clock and Date is inaccurate* - Pretty sure turning it off and on *again* would work, if that doesn't work try changing Wifi by choosing `Wifi setting` option in the setting page.
* *NOT CONNECTED at the top of the screen* - When first booting up please wait a minute to let Wio Terminal connect to the server, if its stay for more than a minute please check your Wifi connection/ change your `Wifi setting` in the setting page.

If that doesn't work then the server is probably dead, you could try running it yourself using [this](https://github.com/nut1414/MeePlan101-backend) then changing server url location in the src file.

