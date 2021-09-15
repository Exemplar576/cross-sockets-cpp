#ifndef SocketLib
#define SocketLib

#ifdef WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>
	#pragma comment(lib, "Ws2_32.lib")
#else
	#include <sys/time.h>
	#include <sys/types.h>
	#include <sys/select.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif
#include <vector>
#include <string>
#include <exception>

enum class AddressFamily { Internetwork = AF_INET, InternetworkV6 = AF_INET6 };
enum class SocketType { Stream = SOCK_STREAM, DGRAM = SOCK_DGRAM };
enum class Protocol { TCP = IPPROTO_TCP, UDP = IPPROTO_UDP };
enum class SelectMode { Read = 0, Write = 1, Error = 2 };
enum class SocketProtection { Sock = SOL_SOCKET, TCP = IPPROTO_TCP, UDP = IPPROTO_UDP };
enum class SocketOption { Broadcast = SO_BROADCAST, Reuseaddr = SO_REUSEADDR, KeepAlive = SO_KEEPALIVE };
#ifdef WIN32
enum class How { Both = SD_BOTH, Send = SD_SEND, Receive = SD_RECEIVE };
#else
enum class How { Both = SHUTDOWN_RDWR, Send = SHUTDOWN_WR, Receive = SHUTDOWN_RD };
#endif

class Socket
{
private:
	void Init();

	int s;
	sockaddr_in address_info;
	int AF;
	int TYPE;
	int PROTOCOL;
	static fd_set master;
	static bool libLoaded;

public:

	//Instantiate a default Tcp Socket.
	Socket();

	//Instantiate a Socket with custom parameters.
	Socket(AddressFamily af, SocketType type, Protocol protocol);

	//Socket Options
	void SetSocketOption(SocketProtection protect, SocketOption options, BOOL active);

	//Check array of sockets for readability, writability, exceptionals and acceptability.
	static void Select(std::vector<Socket>* read, SelectMode mode, int timesec = 0);

	//Check socket for readability, writability, exceptions and acceptability.
	bool Poll(SelectMode mode, int timesec = 0);

	//Compare Socket Values
	friend bool operator==(Socket& lhs, Socket& rhs);
	friend bool operator!=(Socket& lhs, Socket& rhs);

	//Return the endpoint ip address of a connected client.
	std::string RemoteEndpoint();

	//Bind a socket to a set address and port.
	void Bind(const char* HOST, int PORT);

	//Listen for incoming connections.
	void Listen(int backlog = 0);

	//Accept an incoming socket connection.
	Socket Accept();

	//Connect to an endpoint with a set ip address and port.
	void Connect(const char* HOST, int PORT);

	//Send bytes to endpoint.
	void Send(std::string message);

	//Receive number of bytes from endpoint.
	int Receive(char* buffer, int length);
	std::string Receive(int bytes);

	//Send DGRAM packets
	void SendTo(std::string message);

	//Receive DGRAM packets.
	int ReceiveFrom(char* buffer, int length);
	std::string ReceiveFrom(int bytes);

	//Stream operators to send message.
	Socket& operator<<(std::string& msg);

	//Stream operator to receive all available data.
	Socket& operator>>(std::string& msg);

	//Send a file over the stream.
	void SendFile(const char* path);

	//Shutdown communication.
	void Shutdown(How flags);

	//Close and cleanup the socket.
	void Close();
};

class SocketException : virtual public std::exception
{
private:
	const char* errors;
public:
	explicit
	SocketException(const char* error) 
	{
		this->errors = error;
	};
	virtual const char* what() const noexcept
	{ 
		return errors;
	};
	virtual ~SocketException() noexcept {}
};

#endif
