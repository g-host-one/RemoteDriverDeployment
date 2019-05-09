#pragma once
#include <iostream>
#include <fstream>
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
	char file_name[MAX_NAME];
	long long file_size;
} driver_info;

std::string GetFileName(std::string filepath);
bool ArgsParser(int argc, char* argv[], std::string* file, std::string* host);
bool TargetConnect(SOCKET* Socket, std::string host);