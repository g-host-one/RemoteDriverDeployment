#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUF_SIZE 1024 
#define MAX_NAME 255

struct
{
	char service_action;
	char service_name[MAX_NAME];
} service_info;

struct
{
	char filename[MAX_NAME];
	long long filesize;
} driver_info;

std::string GetDirectory(std::string filepath);
bool ArgsParser(int argc, char* argv[], std::string* address, int* port);
bool IsElevated();
bool StartListen(SOCKET* Socket, std::string host, int port);
std::string DoStopService(SC_HANDLE hSCManager, char* service_name);
std::string DoStartService(SC_HANDLE hSCManager, char* service_name, const char* file_path);