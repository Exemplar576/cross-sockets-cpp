#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include <string>
#include <vector>

enum AddressFamily { Internetwork = AF_INET, InternetworkV6 = AF_INET6 };
enum SocketType { Stream = SOCK_STREAM, DGRAM = SOCK_DGRAM };
enum Protocol { TCP = IPPROTO_TCP, UDP = IPPROTO_UDP };
enum SelectMode { Read = 0, Write = 1, Error = 2 };
#ifdef WIN32
enum How { Both = SD_BOTH, Send = SD_SEND, Receive = SD_RECEIVE };
#else
enum How { Both = SHUTDOWN_RDWR, Send = SHUTDOWN_WR, Receive = SHUTDOWN_RD };
#endif

class Socket
{
private:
	void Init(const char* HOST, int PORT);
	int s = -1;
	sockaddr_in address_info = { 0 };
	int AF = AF_INET;
	int TYPE = SOCK_STREAM;
	int PROTOCOL = IPPROTO_TCP;
	bool client = false;
	char remoteEndpoint[INET_ADDRSTRLEN] = "";
	static fd_set master;
	static int listen_s;
	static std::vector<int> fd;

public:
	//Instantiate a default Tcp Socket.
	Socket();

	//Instantiate a Socket with custom parameters.
	Socket(AddressFamily af, SocketType type, Protocol protocol);

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
	void Bind(std::string HOST, int PORT);

	//Listen for incoming connections.
	void Listen(int backlog = 0);

	//Accept and incoming socket connection.
	Socket Accept();

	//Connect to an endpoint with a set ip address and port.
	void Connect(std::string HOST, int PORT);

	//Send bytes to endpoint.
	void Send(std::string message);

	//Receive number of bytes from endpoint.
	std::string Receive(int bytes);

	//Send DGRAM packets
	void SendTo(std::string message);

	//Receive DGRAM packets.
	std::string ReceiveFrom(int bytes);

	//Stream operators to send message.
	Socket& operator<<(std::string& msg);

	//Stream operator to receive all available data.
	Socket& operator>>(std::string& msg);

	//Send a file over the stream.
	void SendFile(std::string path);

	//Shutdown communication.
	void Shutdown(How flags);

	//Close and cleanup the socket.
	void Close();
};

class SocketException
{
private:
	const char* errors;
public:
	SocketException(const char* error);
	const char* what();
};
