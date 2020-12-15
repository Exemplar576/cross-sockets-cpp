#include "Socket.hpp"
#include <memory>
#include <fstream>

fd_set Socket::master;

void Socket::Init(const char* HOST, int PORT)
{
#ifdef WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw SocketException("[SocketError] Failed to create socket, perhaps the library is not supported on this OS.");
	}
#endif
	s = socket(AF, TYPE, PROTOCOL);
	sockaddr_in addr;
	inet_pton(AF, HOST, &addr.sin_addr.s_addr);
	addr.sin_family = AF;
	addr.sin_port = htons(PORT);
	address_info = addr;
}
Socket::Socket() { }
Socket::Socket(AddressFamily af, SocketType type, Protocol protocol)
{
	AF = af;
	TYPE = type;
	PROTOCOL = protocol;
}
void Socket::Select(std::vector<Socket>* sock_list, SelectMode mode, int time)
{
	fd_set copy = master;
	std::vector<Socket> list;
	timeval times;
	times.tv_sec = time;
	times.tv_usec = 0;
	switch (mode)
	{
	case Read:
		select(0, &copy, nullptr, nullptr, !time ? nullptr : &times);
		break;
	case Write:
		select(0, nullptr, &copy, nullptr, !time ? nullptr : &times);
		break;
	case SelectMode::Error:
		select(0, nullptr, nullptr, &copy, !time ? nullptr : &times);
		break;
	default:
		throw SocketException("[SocketError] Invalid Select Flag");
	}
	for (int e = 0; e < sock_list->size(); e++)
	{
		if (FD_ISSET(sock_list->operator[](e).s, &copy))
		{
			list.push_back(sock_list->operator[](e));
		}
	}
	sock_list->clear();
	for (int e = 0; e < list.size(); e++)
	{
		sock_list->push_back(list[e]);
	}
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
	return this->remoteEndpoint;
}
void Socket::Bind(std::string HOST, int PORT)
{
	Init(HOST.c_str(), PORT);
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
	FD_ZERO(&master);
	FD_SET(s, &master);
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
	inet_ntop(AF, &client_info.sin_addr, client_sock.remoteEndpoint, INET_ADDRSTRLEN);
	client_sock.client = true;
	FD_SET(client_sock.s, &master);
	return client_sock;
}
void Socket::Connect(std::string HOST, int PORT)
{
	Init(HOST.c_str(), PORT);
	if (connect(s, (sockaddr*)&address_info, sizeof(address_info)) < 0)
	{
		this->Close();
		throw SocketException("[SocketError] Failed to connect to the specified ip address and port.");
	}
	FD_ZERO(&master);
	FD_SET(s, &master);
}
void Socket::Send(std::string message)
{
	if (send(s, message.c_str(), message.length(), 0) < 0)
	{
		this->Close();
		throw SocketException("[SocketError] Failed to send bytes to host.");
	}
}
std::string Socket::Receive(int bytes)
{
	std::unique_ptr<char> receive_buffer(new char[bytes]);
	int Result = recv(s, receive_buffer.get(), bytes, 0);
	if (Result > 0)
	{
		std::string trim(receive_buffer.get(), (size_t)Result);
		return trim;
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
	if (sendto(s, message.c_str(), message.length(), 0, (sockaddr*)&address_info, sizeof(address_info)) < 0)
	{
		this->Close();
		throw SocketException("[SocketError] Failed to send DGRAM to host.");
	}
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
	std::string ss = this->Receive(1024);
	while (this->Poll(Read, 1))
	{
		ss += this->Receive(1024);
	}
	msg = ss;
	return *this;
}
void Socket::SendFile(std::string path)
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
	shutdown(s, flags);
}
void Socket::Close()
{
	FD_CLR(s, &master);
#ifdef WIN32
	closesocket(s);
	if (!client)
	{
		WSACleanup();
	}
#else
	close(s);
#endif
}
SocketException::SocketException(const char* error)
{
	errors = error;
}
const char* SocketException::what()
{
	return this->errors;
}
