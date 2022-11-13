# Lyinject 驱动级DLL注入

本注入驱动程序目前只支持`Windows 10`版本`18363.592`具体版本信息如下，请核对是否支持否则不生效。

![image](https://user-images.githubusercontent.com/52789403/201512472-bffb949b-9f31-40f5-b782-852236de88e7.png)

当需要注入DLL时只需要调用`DriveControl.InjectDll`传入参数即可，详细实现代码请参考`ConsoleApplication`中的源代码，如下实现了将`msg.dll`强行插入到`msgbox.exe`进程内。
```c
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <Windows.h>

#pragma comment(lib,"user32.lib")
#pragma comment(lib,"advapi32.lib")

int main(int argc, char *argv[])
{
	cDrvCtrl DriveControl;
	DriveControl.InstallAndRun();

	// 注入DLL到程序,支持32位与64位注入
	DriveControl.InjectDll("msgbox.exe", "c://msg.dll");

	DriveControl.RemoveAndStop();

	system("pause");
	return 0;
}
```

运行注入程序即可弹出DLL提示框。

![image](https://user-images.githubusercontent.com/52789403/201512753-e6691ff7-f505-4c01-8e87-d97671977153.png)
