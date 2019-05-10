#include "main.h"

int main(int argc, char* argv[])
{
	std::string driver_file, target_address;
	std::ifstream driver;
	SOCKET target;
	char buffer[BUF_SIZE];

	if (ArgsParser(argc, argv, &driver_file, &target_address))
		return 1;

	if (service_info.service_action == 'l')
	{
		driver.open(driver_file, std::ios::in | std::ios::binary | std::ios::ate);

		if (!driver.is_open())
		{
			std::cerr << "Wrong file path or file doesn`t exist" << std::endl;
			return 1;
		}
	}

	std::cout << "Trying connect to " << target_address << std::endl;

	if (TargetConnect(&target, target_address))
		return 1;

	std::cout << "Sending service action" << std::endl;
	send(target, (char*)& service_info, sizeof(service_info), 0);

	if (service_info.service_action == 'l')
	{
		driver_info.file_size = driver.tellg();
		strcpy_s(driver_info.file_name, MAX_NAME, GetFileName(driver_file).c_str());

		std::cout << "Sending driver" << std::endl;
		send(target, (char*)& driver_info, sizeof(driver_info), 0);

		driver.seekg(0, std::ios::beg);

		while (!driver.eof())
		{
			driver.read(buffer, BUF_SIZE);
			send(target, buffer, BUF_SIZE, 0);
		}

		driver.close();
	}
		
	recv(target, buffer, BUF_SIZE, 0);
	std::cout << "Target: " << buffer << std::endl;

	closesocket(target);
	WSACleanup();

	return 0;
}

std::string GetFileName(std::string filepath)
{
	size_t pos = filepath.find_last_of("\\/");
	return filepath.substr(pos+1);
}

bool ArgsParser(int argc, char* argv[], std::string* file, std::string* host)
{
	if (argc < 5) {
		std::cerr << "Usage: " << argv[0] << " -s service [-l file | -u] host[:port] \n\n";
		std::cerr << "\t-s service \t Register driver as <service> on target machine\n";
		std::cerr << "\t-l file \t Load driver from <file> and start on target machine\n";
		std::cerr << "\t-u \t\t Stop driver on target machine\n";
		std::cerr << "\thost \t\t IP addres of target machine\n";
		std::cerr << "\tport \t\t Port of target machine. Default: 5000" << std::endl;
		return 1;
	}

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
			switch (argv[i][1])
			{
			case 's':
				//service name
				i++;
				if (argv[i][0] == '-')
				{
					std::cerr << "Service name not provided" << std::endl;
					return 1;
				}

				strcpy_s(service_info.service_name, MAX_NAME, argv[i]);
				break;
			case 'u':
				if (file->empty())
					service_info.service_action = 'u';
				break;
			case 'l':
				//load driver
				i++;
				if (argv[i][0] == '-')
				{
					std::cerr << "Driver file not provided" << std::endl;
					return 1;
				}

				*file = argv[i];
				service_info.service_action = 'l';
				break;
			default:
				break;
			}
		else
			if (i == argc-1) //host+port
				*host = argv[i];
	}

	return 0;
}

bool TargetConnect(SOCKET* Socket, std::string host)
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

	size_t div = host.find(':');
	int port;

	if (div == -1)
		port = 5000;
	else
		port = atoi(host.substr(div+1).c_str());
	
	host = host.substr(0, div);

	ZeroMemory(&Addr, sizeof(Addr));    // clear the struct
	Addr.sin_family = AF_INET;    // set the address family
	Addr.sin_port = htons(port);    // set the port
	iResult = inet_pton(AF_INET, host.c_str(), &Addr.sin_addr);

	if (iResult != 1)
	{
		std::cerr << "Invalid IP address. Error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	*Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (SOCKET_ERROR == connect(*Socket, (sockaddr*)& Addr, sizeof(Addr)))
	{
		std::cerr << "Connection failed. Error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}
	return 0;
}
