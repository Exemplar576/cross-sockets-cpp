#include "Socket.hpp"
#include <iostream>
#include <thread>

void echo(Socket c);

int main()
{
    Socket s(AddressFamily::Internetwork, SocketType::Stream, Protocol::TCP);
    const char* HOST = "0.0.0.0";
    const int PORT = 3000;
    s.Bind(HOST, PORT);
    s.Listen();
    std::cout << "[+]Listening for Connections on " << HOST << " at Port " << PORT << std::endl;
    while (true)
    {
        Socket c = s.Accept();
        std::cout << "Client Connected: " << c.RemoteEndpoint() << std::endl;
        std::thread t = std::thread(echo, c);
    }
}
void echo(Socket c)
{
    try
    {
        c.Send(c.Receive(4096));
        c.Close();
    }
    catch (SocketException e) 
    { 
        std::cerr << e.what() << std::endl;
    }
} 
