#include "main.h"

int main(int argc, char* argv[])
{
	std::string listen_address, filepath;
	std::ofstream file;
	SOCKET listener, client;
	sockaddr_in clientAddress;
	SC_HANDLE sc_manager_handle;
	int clientAddressLen = sizeof(clientAddress);
	int port;
	char buffer[BUF_SIZE];

	if (!IsElevated())
	{
		std::cerr << "Need admin privilages" << std::endl;
		system("pause");
		return 1;
	}

	sc_manager_handle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (NULL == sc_manager_handle)
	{
		std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
		system("pause");
		return 1;
	}

	if (ArgsParser(argc, argv, &listen_address, &port))
	{
		system("pause");
		return 1;
	}


	if (StartListen(&listener, listen_address, port))
	{
		system("pause");
		return 1;
	}

	std::cout << "Listening " << listen_address << ":" << port << "\n";

	while (true)
	{
		std::cout << "\nWaiting for client ...\n";

		if (client = accept(listener, (sockaddr*)& clientAddress, &clientAddressLen))
		{
			inet_ntop(AF_INET, &clientAddress.sin_addr, buffer, 16);
			std::cout << "Client connected: " << buffer <<
				":" << ntohs(clientAddress.sin_port) << "\n\n";

			std::cout << "Receiving service info ...\n";

			recv(client, (char*)& service_info, sizeof(service_info), 0);

			std::cout << "Stopping service: " << service_info.service_name << "\n";

			std::string info = DoStopService(sc_manager_handle, service_info.service_name);

			if (service_info.service_action == 'u') 
			{
				std::cerr << info << std::endl;
				send(client, info.c_str(), info.length()+1, 0);
				
				closesocket(client);
				continue;
			}
			
			if (service_info.service_action == 'l')
			{
				std::cout << "\nReceiving driver info ...\n";
				
				recv(client, (char*)& driver_info, sizeof(driver_info), 0);

				std::cout << "Driver name: " << driver_info.filename << "\n";
				std::cout << "Driver size: " << driver_info.filesize << "\n";

				filepath = GetDirectory(std::string(argv[0])) + std::string(driver_info.filename);
				file.open(filepath, std::ios::out | std::ios::binary | std::ios::trunc);

				if (!file.is_open())
				{
					std::cerr << "Error open file for writing" << std::endl;
					send(client, "Error open file for writing", 28, 0);
					closesocket(client);
					continue;
				}

				std::cout << "\nReceiving driver file ...\n";

				long bytes_left = driver_info.filesize;
				long bytes_read;

				std::cout << "Bytes left: ";

				while (bytes_left > 0)
				{
					std::cout << bytes_left << " ";

					bytes_read = recv(client, buffer, BUF_SIZE, 0);
					if (bytes_left < BUF_SIZE)
						file.write(buffer, bytes_left);
					else
						file.write(buffer, BUF_SIZE);

					bytes_left -= bytes_read;
				}

				std::cout << "\n\n";

				file.close();

				std::cout << "Deploing driver ..." << std::endl;


				filepath = "\\??\\" + GetDirectory(std::string(argv[0])) + std::string(driver_info.filename);

				info += "\n" + DoStartService(sc_manager_handle, service_info.service_name, filepath.c_str());

				std::cerr << info << std::endl;
				send(client, info.c_str(), info.length() + 1, 0);

				closesocket(client);
				continue;
			}


		}
	}

	CloseServiceHandle(sc_manager_handle);
	closesocket(listener);
	WSACleanup();
	return 0;
}

std::string GetDirectory(std::string filepath)
{
	char buf[MAX_NAME];
	GetCurrentDirectoryA(MAX_NAME, buf);

	return std::string(buf)+"\\";
}

bool ArgsParser(int argc, char* argv[], std::string* address, int* port)
{
	if (argc == 1)
	{
		*address = "0.0.0.0";
		*port = 5000;
		return 0;
	}
	else
		if (argc == 2)
		{
			*address = argv[1];
			size_t div = address->find(':');

			if (div != -1)
			{
				*port = atoi(address->substr(div + 1).c_str());
				*address = address->substr(0, div);
				return 0;
			}
		}
		
	std::cerr << "Usage: " << argv[0] << " [address:port] \n\n";
	std::cerr << "\taddress \t IP addres listen. Default: 0.0.0.0\n";
	std::cerr << "\tport \t\t Port listen on target. Default: 5000" << std::endl;

	return 1;
}

bool IsElevated() {
	bool fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) {
		CloseHandle(hToken);
	}
	return fRet;
}

bool StartListen(SOCKET* Socket, std::string host, int port)
{
	WSADATA Winsock;
	sockaddr_in Addr;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &Winsock);
	if (iResult != 0)
	{
		std::cerr << "WSAStartup failed. Error: " << iResult << std::endl;
		return 1;
	}

	ZeroMemory(&Addr, sizeof(Addr));
	Addr.sin_family = AF_INET;
	Addr.sin_port = htons(port);
	iResult = inet_pton(AF_INET, host.c_str(), &Addr.sin_addr);

	if (iResult != 1)
	{
		std::cerr << "Invalid IP address. Error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	*Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (SOCKET_ERROR == bind(*Socket, (sockaddr*)& Addr, sizeof(Addr)))
	{
		std::cerr << "Binding failed. Error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	if (listen(*Socket, 1) == SOCKET_ERROR)
	{
		std::cerr << "Listening error. Error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}
}

std::string DoStopService(SC_HANDLE hSCManager, char* service_name)
{
	std::string res;
	SC_HANDLE sc_service_handle;
	SERVICE_STATUS_PROCESS ssp;
	int error;
	DWORD dwBytesNeeded;
	DWORD dwWaitTime;
	DWORD dwStartTime = GetTickCount64();
	DWORD dwTimeout = 30000; // 30-second time-out

	sc_service_handle = OpenService(hSCManager, service_name, GENERIC_ALL);

	if (NULL == sc_service_handle)
	{
		error = GetLastError();
		if (ERROR_SERVICE_DOES_NOT_EXIST != error)
		{
			return "OpenService failed. Error: " + std::to_string(error);
		}
		return "Service does not exist";
	}

	// Make sure the service is not already stopped.

	if (!QueryServiceStatusEx(sc_service_handle, SC_STATUS_PROCESS_INFO,
		(LPBYTE)& ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
	{
		CloseServiceHandle(sc_service_handle);
		return "QueryServiceStatusEx failed. Error: " + std::to_string(GetLastError());
	}

	if (ssp.dwCurrentState == SERVICE_STOPPED)
	{
		CloseServiceHandle(sc_service_handle);
		return "Service is already stopped.";
	}

	while (ssp.dwCurrentState == SERVICE_STOP_PENDING)
	{
		dwWaitTime = ssp.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(sc_service_handle, SC_STATUS_PROCESS_INFO,
			(LPBYTE)& ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
		{
			CloseServiceHandle(sc_service_handle);
			return "QueryServiceStatusEx failed. Error: " + std::to_string(GetLastError());
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
		{
			CloseServiceHandle(sc_service_handle);
			return "Service stopped successfully.";
		}

		if (GetTickCount64() - dwStartTime > dwTimeout)
		{
			CloseServiceHandle(sc_service_handle);
			return "Service stop timed out.";
		}
	}

	// Send a stop code to the service.

	if (!ControlService(sc_service_handle, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)& ssp))
	{
		CloseServiceHandle(sc_service_handle);
		return "ControlService failed. Error: " + std::to_string(GetLastError());
	}

	// Wait for the service to stop.

	while (ssp.dwCurrentState != SERVICE_STOPPED)
	{
		Sleep(ssp.dwWaitHint);

		if (!QueryServiceStatusEx(sc_service_handle, SC_STATUS_PROCESS_INFO,
			(LPBYTE)& ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
		{
			CloseServiceHandle(sc_service_handle);
			return "QueryServiceStatusEx failed. Error: " + std::to_string(GetLastError());
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
			break;

		if (GetTickCount64() - dwStartTime > dwTimeout)
		{
			CloseServiceHandle(sc_service_handle);
			return "Service stop timed out.";
		}
	}

	return "Service stopped successfully";
}

std::string DoStartService(SC_HANDLE hSCManager, char* service_name, const char* file_path)
{
	SC_HANDLE sc_service_handle;
	int error;

	sc_service_handle = OpenService(hSCManager, service_name, GENERIC_ALL);

	if (NULL == sc_service_handle)
	{
		error = GetLastError();
		if (ERROR_SERVICE_DOES_NOT_EXIST == error)
		{
			sc_service_handle = CreateService(hSCManager, service_name, service_name,
				SC_MANAGER_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
				SERVICE_ERROR_NORMAL, file_path, NULL, NULL, NULL, NULL, NULL);

			if (NULL == sc_service_handle)
			{
				return "CreateService failed. Error: " + std::to_string(GetLastError());
			}
		}
		return "OpenService failed. Error: " + std::to_string(error);
	}

	if (ChangeServiceConfig(sc_service_handle, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE,
		file_path, NULL, NULL, NULL, NULL, NULL, NULL) == 0)
	{
		return "ChangeServiceConfig failed. Error: " + std::to_string(GetLastError());
	}

	if (StartService(sc_service_handle, 0, NULL) == 0)
	{
		return "StartService failed. Error: " + std::to_string(GetLastError());
	}

	return "Service started successfully";
}