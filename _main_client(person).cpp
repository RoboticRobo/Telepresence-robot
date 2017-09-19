#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <conio.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>

#include "RobotConnector.h"

#include "cv.h"
#include "highgui.h"

#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <NuiApi.h>
#include <NuiImageCamera.h>
#include <NuiSensor.h>
#include <KinectConnector.h>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

#define Create_Comport "COM3"

bool isRecord = false;
int drag = 0 , select_flag = 0;

boolean inCircle(int x, int y, int r) {
	return (x*x) + (y*y) <= (r*r);
}

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{	
	select_flag = 0;
	if (event == EVENT_LBUTTONDOWN && !drag && !select_flag)
	{
		if (inCircle(x-320, y-240, 240)) {
			cout << (x - 320) << " " << (-1)*(y - 240) << endl;
			drag = 1;
		}
	}

	if (event == EVENT_MOUSEMOVE && drag && !select_flag) {
		if (inCircle(x - 320, y - 240, 240)) {
			cout << (x - 320) << " " << (-1)*(y - 240) << endl;
		}
	}
	if (event == CV_EVENT_LBUTTONUP && drag && !select_flag)
	{
		drag = 0;
		select_flag = 1;
	}
}

int main()
{
	Mat mat(480, 640, CV_8UC3, cv::Scalar(0, 0, 0) );
	circle(mat, Point(320, 240), 240, cv::Scalar(255, 255, 255	), -1);
	int client;
	int portNum = 1500; // NOTE that the port number is same for both client and server
	bool isExit = false;
	const int bufsize = 1024;
	char buffer[bufsize];
	char* ip = "127.0.0.1";

	struct sockaddr_in server_addr;


	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR) {
		cout << "Error initialising WSA.\n" << endl;
		return -1;
	}


	client = socket(AF_INET, SOCK_STREAM, 0);

	// DWORD timeout = 1 * 1000;
	// setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));


	if (client < 0)
	{
		cout << "\nError establishing socket..." << endl;
		exit(1);
	}

	cout << "\n=> Socket client has been created..." << endl;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portNum);
	inet_pton(AF_INET, ip, &server_addr.sin_addr);

	if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
		cout << "=> Connection to the server " << inet_ntoa(server_addr.sin_addr) << " with port number: " << portNum << endl;
	else {
		cout << WSAGetLastError();
		return 0;
	}

	cout << "=> Awaiting confirmation from the server..." << endl; //line 40
	recv(client, buffer, bufsize, 0);
	cout << "=> Connection confirmed, you are good to go...";

	cout << "\n\n=> Enter # to end the connection\n" << endl;

	cout << "Client: ";
	while (true) {
		setMouseCallback("Mat", CallBackFunc, NULL);
		imshow("Mat",mat);
		waitKey(20);

		if (kbhit() != 0) {
			buffer[0] = getch();
			buffer[1] = 0;
			cout << buffer[0];
			send(client, buffer, bufsize, 0);
			cout << endl;
			cout << "Client: ";
		}
	}



	cout << "\n=> Connection terminated.\nGoodbye...\n";

	close(client);
	return 0;
}