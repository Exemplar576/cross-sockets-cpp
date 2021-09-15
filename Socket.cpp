#include "Socket.hpp"
#include <memory>
#include <algorithm>
#include <fstream>

fd_set Socket::master;
bool Socket::libLoaded = false;

void Socket::Init()
{
	if (!libLoaded)
	{
#ifdef WIN32
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			throw SocketException("[SocketError] Failed to create socket, perhaps the library is not supported on this OS.");
		}
#endif
		FD_ZERO(&master);
		libLoaded = true;
	}
	s = socket(AF, TYPE, PROTOCOL);
	FD_SET(s, &master);
}
Socket::Socket() 
{
	AF = AF_INET;
	TYPE = SOCK_STREAM;
	PROTOCOL = IPPROTO_TCP;
	Init();
}
Socket::Socket(AddressFamily af, SocketType type, Protocol protocol)
{
	AF = (int)af;
	TYPE = (int)type;
	PROTOCOL = (int)protocol;
	Init();
}
void Socket::SetSocketOption(SocketProtection protect, SocketOption option, BOOL active)
{
	if (setsockopt(s, (int)protect, (int)option, (const char*)active, sizeof(BOOL)) < 0)
	{
		throw SocketException("[SocketError] Failed to set option.");
	}
}
void Socket::Select(std::vector<Socket>* sock_list, SelectMode mode, int time)
{
	fd_set copy = master;
	timeval times;
	times.tv_sec = time;
	times.tv_usec = 0;
	switch (mode)
	{
	case SelectMode::Read:
		select(0, &copy, nullptr, nullptr, !time ? nullptr : &times);
		break;
	case SelectMode::Write:
		select(0, nullptr, &copy, nullptr, !time ? nullptr : &times);
		break;
	case SelectMode::Error:
		select(0, nullptr, nullptr, &copy, !time ? nullptr : &times);
		break;
	default:
		throw SocketException("[SocketError] Invalid Select Flag");
	}
	std::remove_if(sock_list->begin(), sock_list->end(), [&copy](Socket sock) { return !FD_ISSET(sock.s, &copy); });
}
bool Socket::Poll(SelectMode mode, int time)
{
	std::vector<Socket> sock = { *this };
	Socket::Select(&sock, mode, time);
	return sock.size() > 0;
}
bool operator==(Socket& lhs, Socket& rhs)
{
	return lhs.s == rhs.s;
}
bool operator!=(Socket& lhs, Socket& rhs)
{
	return lhs.s != rhs.s;
}
std::string Socket::RemoteEndpoint()
{
	int addrLength = AF == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
	std::unique_ptr<char> buffer(new char[addrLength]);
	inet_ntop(AF, &address_info.sin_addr, buffer.get(), addrLength);
	return std::string(buffer.get());
}
void Socket::Bind(const char* HOST, int PORT)
{
	sockaddr_in addr;
	inet_pton(AF, HOST, &addr.sin_addr.s_addr);
	addr.sin_family = AF;
	addr.sin_port = htons(PORT);
	address_info = addr;
	if (bind(s, (sockaddr*)&address_info, sizeof(address_info)) < 0)
	{
		this->Close();
		throw SocketException("[SocketError] Failed to bind to the specified ip address and port.");
	}
}
void Socket::Listen(int backlog)
{
	if (listen(s, backlog) < 0)
	{  
		this->Close();
		throw SocketException("[SocketError] Failed to start listening.");
	}
}
Socket Socket::Accept()
{
	Socket client_sock;
	sockaddr_in client_info = { 0 };
	socklen_t addrsize = sizeof(client_info);
	client_sock.s = accept(s, (sockaddr*)&client_info, &addrsize);
	if (client_sock.s <= 0)
	{
		this->Close();
		throw SocketException("[SocketError] Accept Error.");
	}
	client_sock.address_info = client_info;
	FD_SET(client_sock.s, &master);
	return client_sock;
}
void Socket::Connect(const char* HOST, int PORT)
{
	sockaddr_in addr;
	inet_pton(AF, HOST, &addr.sin_addr.s_addr);
	addr.sin_family = AF;
	addr.sin_port = htons(PORT);
	address_info = addr;
	if (connect(s, (sockaddr*)&address_info, sizeof(address_info)) < 0)
	{
		this->Close();
		throw SocketException("[SocketError] Failed to connect to the specified ip address and port.");
	}
}
void Socket::Send(std::string message)
{
	if (send(s, message.c_str(), message.size(), 0) < 0)
	{
		this->Close();
		throw SocketException("[SocketError] Failed to send bytes to host.");
	}
}
int Socket::Receive(char* buffer, int length)
{
	int Result = recv(s, buffer, length, 0);
	if (Result > 0)
	{
		return Result;
	}
	else if (Result == 0)
	{
		this->Close();
		throw SocketException("[SocketError] Connection to the host has been closed.");
	}
	else
	{
		this->Close();
		throw SocketException("[SocketError] Failed to receive bytes from host.");
	}
}
std::string Socket::Receive(int bytes)
{
	std::unique_ptr<char> receive_buffer(new char[bytes]);
	int Result = recv(s, receive_buffer.get(), bytes, 0);
	if (Result > 0)
	{
		return std::string(receive_buffer.get(), Result);
	}
	else if (Result == 0)
	{
		this->Close();
		throw SocketException("[SocketError] Connection to the host has been closed.");
	}
	else
	{
		this->Close();
		throw SocketException("[SocketError] Failed to receive bytes from host.");
	}
}
void Socket::SendTo(std::string message)
{
	if (sendto(s, message.c_str(), message.size(), 0, (sockaddr*)&address_info, sizeof(address_info)) < 0)
	{
		this->Close();
		throw SocketException("[SocketError] Failed to send DGRAM to host.");
	}
}
int Socket::ReceiveFrom(char* buffer, int length)
{
	socklen_t addr_size = sizeof(&address_info);
	int Result = recvfrom(s, buffer, length, 0, (sockaddr*)&address_info, &addr_size);
	if (Result < 0)
	{
		this->Close();
		throw SocketException("[SocketError] Failed to receive DGRAM packets.");
	}
	return Result;
}
std::string Socket::ReceiveFrom(int bytes)
{
	std::unique_ptr<char> receive_buffer(new char[bytes]);
	socklen_t addr_size = sizeof(&address_info);
	int Result = recvfrom(s, receive_buffer.get(), bytes, 0, (sockaddr*)&address_info, &addr_size);
	if (Result < 0)
	{
		this->Close();
		throw SocketException("[SocketError] Failed to receive DGRAM packets.");
	}
	return std::string(receive_buffer.get(), Result);
}
Socket& Socket::operator<<(std::string& msg)
{
	this->Send(msg);
	return *this;
}
Socket& Socket::operator>>(std::string& msg)
{
	std::string ss(this->Receive(1024));
	while (this->Poll(SelectMode::Read, 1))
	{
		ss += std::string(this->Receive(1024));
	}
	msg = ss.c_str();
	return *this;
}
void Socket::SendFile(const char* path)
{
	std::ifstream file(path, std::ios::binary);
	file.seekg(0, std::ios::end);
	std::streampos count = file.tellg();
	file.seekg(0, std::ios::beg);
	std::unique_ptr<char> ptr(new char[count]);
	char* buffer = ptr.get();
	file.read(buffer, count);
	file.close();
	while (count > 0)
	{
		int result = send(s, buffer, count, 0);
		if (result < 0)
		{
			this->Close();
			throw SocketException("[SocketError] Failed to send file to host.");
		}
		else 
		{
			count -= result;
			buffer += result;
		}
	}
}
void Socket::Shutdown(How flags)
{
	shutdown(s, (int)flags);
}
void Socket::Close()
{
	FD_CLR(s, &master);
#ifdef WIN32
	closesocket(s);
#else
	close(s);
#endif
	if (master.fd_count < 1)
	{
#ifdef WIN32
		WSACleanup();
#endif
		libLoaded = false;
	}
}
