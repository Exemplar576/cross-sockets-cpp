#include "Socket.hpp"
#include <iostream>
#include <string>

int amain()
{
	Socket s(Internetwork, Stream, TCP);
	while (true)
	{
		try
		{
			s.Connect("10.86.7.182", 3000);
			std::string msg;
			std::cin >> msg;
			s.Send(msg);
			std::cout << s.Receive(1024) << std::endl;
			s.Close();
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
}