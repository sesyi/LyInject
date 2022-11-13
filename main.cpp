#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <Windows.h>

#pragma comment(lib,"user32.lib")
#pragma comment(lib,"advapi32.lib")

// ------------------------------------------------------------------------------
// 结构定义
// ------------------------------------------------------------------------------

// 通信结构体定义
typedef struct
{
	CHAR ProcessName[256];
	CHAR InjectPath[512];
}InjectProcessStruct, *LPInjectProcessStruct;

// ------------------------------------------------------------------------------
// 定义驱动功能号和名字，提供接口给应用程序调用
// ------------------------------------------------------------------------------

#define IOCTL_IO_InjectDll        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ------------------------------------------------------------------------------
// 驱动控制类
// ------------------------------------------------------------------------------

class cDrvCtrl
{
public:
	cDrvCtrl()
	{
		m_pSysPath = NULL;
		m_pServiceName = NULL;
		m_pDisplayName = NULL;
		m_hSCManager = NULL;
		m_hService = NULL;
		m_hDriver = INVALID_HANDLE_VALUE;
	}
	~cDrvCtrl()
	{
		CloseServiceHandle(m_hService);
		CloseServiceHandle(m_hSCManager);
		CloseHandle(m_hDriver);
	}

	// 安装驱动
	BOOL Install(PCHAR pSysPath, PCHAR pServiceName, PCHAR pDisplayName)
	{
		m_pSysPath = pSysPath;
		m_pServiceName = pServiceName;
		m_pDisplayName = pDisplayName;
		m_hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (NULL == m_hSCManager)
		{
			m_dwLastError = GetLastError();
			return FALSE;
		}
		m_hService = CreateServiceA(m_hSCManager, m_pServiceName, m_pDisplayName,
			SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
			m_pSysPath, NULL, NULL, NULL, NULL, NULL);
		if (NULL == m_hService)
		{
			m_dwLastError = GetLastError();
			if (ERROR_SERVICE_EXISTS == m_dwLastError)
			{
				m_hService = OpenServiceA(m_hSCManager, m_pServiceName, SERVICE_ALL_ACCESS);
				if (NULL == m_hService)
				{
					CloseServiceHandle(m_hSCManager);
					return FALSE;
				}
			}
			else
			{
				CloseServiceHandle(m_hSCManager);
				return FALSE;
			}
		}
		return TRUE;
	}

	// 启动驱动
	BOOL Start()
	{
		if (!StartServiceA(m_hService, NULL, NULL))
		{
			m_dwLastError = GetLastError();
			return FALSE;
		}
		return TRUE;
	}

	// 关闭驱动
	BOOL Stop()
	{
		SERVICE_STATUS ss;
		GetSvcHandle(m_pServiceName);
		if (!ControlService(m_hService, SERVICE_CONTROL_STOP, &ss))
		{
			m_dwLastError = GetLastError();
			return FALSE;
		}
		return TRUE;
	}

	// 移除驱动
	BOOL Remove()
	{
		GetSvcHandle(m_pServiceName);
		if (!DeleteService(m_hService))
		{
			m_dwLastError = GetLastError();
			return FALSE;
		}
		return TRUE;
	}

	// 打开驱动
	BOOL Open(PCHAR pLinkName)
	{
		if (m_hDriver != INVALID_HANDLE_VALUE)
			return TRUE;
		m_hDriver = CreateFileA(pLinkName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (m_hDriver != INVALID_HANDLE_VALUE)
			return TRUE;
		else
			return FALSE;
	}

	// 安装并运行驱动
	VOID InstallAndRun()
	{
		char szSysFile[MAX_PATH] = { 0 };
		char szSvcLnkName[] = "LyInject";;
		GetAppPath(szSysFile);
		strcat(szSysFile, "LyInject.sys");

		Install(szSysFile, szSvcLnkName, szSvcLnkName);
		Start();
		Open("\\\\.\\LyInject");
	}

	// 移除并关闭驱动
	VOID RemoveAndStop()
	{
		Stop();
		Remove();
		CloseHandle(m_hDriver);
	}

	// 发送控制信号
	BOOL IoControl(DWORD dwIoCode, PVOID InBuff, DWORD InBuffLen, PVOID OutBuff, DWORD OutBuffLen, DWORD *RealRetBytes)
	{
		DWORD dw;
		BOOL b = DeviceIoControl(m_hDriver, CTL_CODE_GEN(dwIoCode), InBuff, InBuffLen, OutBuff, OutBuffLen, &dw, NULL);
		if (RealRetBytes)
			*RealRetBytes = dw;
		return b;
	}

	// 注入驱动
	BOOL InjectDll(CHAR *ProcessName, CHAR *InjectPath)
	{
		InjectProcessStruct ptr = { 0 };
		DWORD lpBytesReturned = 0;
		DWORD ref = 0;

		strcpy(ptr.ProcessName, ProcessName);
		strcpy(ptr.InjectPath, InjectPath);

		if (DeviceIoControl(m_hDriver, IOCTL_IO_InjectDll, &ptr, sizeof(InjectProcessStruct), &ref, sizeof(DWORD), &lpBytesReturned, NULL))
		{
			return TRUE;
		}
		return FALSE;
	}
private:
	// 获取服务句柄
	BOOL GetSvcHandle(PCHAR pServiceName)
	{
		m_pServiceName = pServiceName;
		m_hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (NULL == m_hSCManager)
		{
			m_dwLastError = GetLastError();
			return FALSE;
		}
		m_hService = OpenServiceA(m_hSCManager, m_pServiceName, SERVICE_ALL_ACCESS);
		if (NULL == m_hService)
		{
			CloseServiceHandle(m_hSCManager);
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}

	// 获取控制信号对应字符串
	DWORD CTL_CODE_GEN(DWORD lngFunction)
	{
		return (FILE_DEVICE_UNKNOWN * 65536) | (FILE_ANY_ACCESS * 16384) | (lngFunction * 4) | METHOD_BUFFERED;
	}

	// 获取完整路径
	void GetAppPath(char *szCurFile)
	{
		GetModuleFileNameA(0, szCurFile, MAX_PATH);
		for (SIZE_T i = strlen(szCurFile) - 1; i >= 0; i--)
		{
			if (szCurFile[i] == '\\')
			{
				szCurFile[i + 1] = '\0';
				break;
			}
		}
	}

public:
	DWORD m_dwLastError;
	PCHAR m_pSysPath;
	PCHAR m_pServiceName;
	PCHAR m_pDisplayName;
	HANDLE m_hDriver;
	SC_HANDLE m_hSCManager;
	SC_HANDLE m_hService;
};

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
