#include "Socket.hpp"
#include <iostream>
#include <string>

int main()
{
	while (true)
	{
		try
		{
			Socket s(AddressFamily::Internetwork, SocketType::Stream, Protocol::TCP);
			s.Connect("192.168.1.192", 3000);
			std::string msg;
			std::cin >> msg;
			s.Send(msg);
			std::cout << s.Receive(1024) << std::endl;
			s.Close();
		}
		catch (SocketException e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
}
