#include "Socket.hpp"
#include <iostream>
#include <vector>

int main()
{
    Socket s(Internetwork, Stream, TCP);
    const char* HOST = "0.0.0.0";
    const int PORT = 3000;
    s.Bind(HOST, PORT);
    s.Listen();
    std::vector<Socket> connections;
    std::cout << "[+]Listening for Connections on " << HOST << " at Port " << PORT << std::endl;
    while (true)
    {
        if (s.Poll(Read, 1))
        {
            Socket c = s.Accept();
            connections.push_back(c);
            std::cout << "Client Connected: " << c.RemoteEndpoint() << std::endl;
        }
        std::vector<Socket> receive = connections;
        Socket::Select(&receive, Read);
        for (int i = 0; i < receive.size(); i++)
        {
            try
            {
                receive[i].Send(receive[i].Receive(4096));
                receive[i].Close();
            }
            catch (SocketException e) 
            {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}
