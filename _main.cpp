#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <winsock2.h>
#include <io.h>
#include <ws2tcpip.h>


#include "RobotConnector.h"

#include "cv.h"
#include "highgui.h"

#include <Windows.h>
#include <iostream>
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

Mat depthImg;
Mat colorImg;
Mat indexImg;
Mat pointImg;
Mat temp;
int grab = 0;
DWORD WINAPI streamVideo(LPVOID lpParameter)
{
	int& server = *((int*)lpParameter);

			//VideoCapture cap(1);
			//cap >> colorImg;

	cvNamedWindow("streamvideo");

	
	while (true) {

		if (grab == 0) {
			Mat src;
			Mat dst;
			Size size(128, 96);

			resize(colorImg, dst, size);

			int dstSize = dst.total() * dst.elemSize();

			unsigned char * p = dst.data;
			const char * c = (const char *)p;

			cout << "send" << endl;
			if (send(server, c, dstSize, 0) == SOCKET_ERROR)
				break;


			cvWaitKey(50);
		}

	}

	return 0;
}

int check_wall(Mat depthImg) {
	/*int x_wall = 250;
	int y_wall = 2;
	int count = 0;
	for (int i = 320 - x_wall / 2; i < 320 + x_wall / 2; i++) {
	for (int j = 240 - y_wall / 2; j < 240 + y_wall / 2; j++) {
	int depth = depthImg.at<USHORT>(j, i);

	if (depth == 0)
	count++;
	else if (depth < 800 && depth > 600)
	return 1;
	else if (depth <= 600)
	return 2;
	}
	}
	if (count >= 170)
	return 3;
	return 0;
	*/

	int countleft = 0;
	int typeleft = 0;
	int xleft = 0;
	int flagleft = 0;
	for (int i = 0; i < 320; i++) {
		int depth = depthImg.at<USHORT>(240, i);

		int x = (int)(depth * (i - 320) / 5240);
		if (depth == 0)
			countleft++;
		else if (depth < 800 && depth > 600) {
			typeleft = 1;
			xleft = x;
		}
		else if (depth <= 600) {
			typeleft = 2;
			xleft = x;
		}
		else {
			if (x >= -20) {
				flagleft = 1;
			}
		}
	}


	int countright = 0;
	int typeright = 0;
	int xright = 0;
	int flagright = 0;
	for (int i = 639; i >= 320; i--) {
		int depth = depthImg.at<USHORT>(240, i);

		int x = (int)(depth * (i - 320) / 5240);
		if (depth == 0)
			countright++;
		else if (depth < 800 && depth > 600) {
			typeright = 1;
			xright = x;
		}
		else if (depth <= 600) {
			typeright = 2;
			xright = x;
		}
		else {
			if (x <= 20) {
				flagright = 1;
			}
		}
	}


	if (typeleft == 1 || typeleft == 2) {
		if (xleft >= -20)
			return typeleft;
	}

	else if (typeright == 1 || typeright == 2) {
		if (xright <= 20)
			return typeright;
	}
	else if (flagleft == 1 && flagright == 1)
		return 0;
	else {
		if (countleft > 250 || countright > 250)
			return 2;
	}
	return 0;

}

int main()
{

	cout << endl;
	cout << "Please choose mode : " << endl;
	cout << " type 's' for control through socket" << endl;
	cout << " type 'c' for chat through socket" << endl;
	cout << " otherwise for control standalone" << endl;
	cout << "Your mode : ";
	char mode;
	cin >> mode;

	while (true) {

		int portNum = 1500;
		int nReadBytes;
		int client, server;
		bool isExit = false;
		const int bufsize = 1024;
		char buffer[bufsize];

		struct sockaddr_in server_addr;
		socklen_t size;

		WSADATA wsaData;
		server = 1;

		//////////////////////////////////////////////
		// socket
		//////////////////////////////////////////////
		if (mode == 's' || mode == 'c') {

			if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR) {
				cout << "\nError initialising WSA. Error code: " << WSAGetLastError() << endl;
				continue;
			}

			if ((client = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR) {
				cout << "\nError establishing socket.. Error code: " << WSAGetLastError() << endl;
				continue;
			}

			cout << "\n=> Socket server has been created..." << endl;

			server_addr.sin_family = AF_INET;
			server_addr.sin_addr.s_addr = htons(INADDR_ANY);
			server_addr.sin_port = htons(portNum);

			size = sizeof(server_addr);


			if ((bind(client, (struct sockaddr*)&server_addr, size)) == SOCKET_ERROR) {
				cout << "\n Socket has been established. Error code: " << WSAGetLastError() << endl;
				continue;
			}

			cout << "=> Looking for clients..." << endl;

			listen(client, 1);
			if ((server = accept(client, (struct sockaddr *)&server_addr, &size)) == SOCKET_ERROR) {
				cout << "=> Error on accepting..." << endl;
				continue;
			}

			DWORD timeout = 50;
			setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

			cout << "start" << endl;
		}

		//////////////////////////////////////////////
		// video streaming
		//////////////////////////////////////////////

		if (mode == 's' || mode == 'c') {
			strcpy(buffer, "=> Server connected...\n");
			send(server, buffer, bufsize, 0);
		}


		//////////////////////////////////////////////
		// control robot
		//////////////////////////////////////////////
		if (mode == 'c') {

			while (true) {

				if ((nReadBytes = recv(server, buffer, bufsize, 0)) == SOCKET_ERROR)
					if (WSAGetLastError() != WSAETIMEDOUT)
						break;

				cvWaitKey(30);
			}

			if (nReadBytes == SOCKET_ERROR) {
				closesocket(server);
				closesocket(client);
				continue;
			}
		}
		else {

			CreateData	robotData;
			RobotConnector	robot;
			KinectConnector kin = KinectConnector();

			if (!robot.Connect(Create_Comport) || !kin.Connect()) {
				cout << "Error : Can't connect to robot or kinect" << endl;

				if (mode == 's' || mode == 'c') {
					closesocket(server);
					closesocket(client);
				}
				continue;
			}
			kin.GrabData(depthImg, colorImg, indexImg, pointImg);

			if (mode == 's') {
				DWORD myThreadID;
				HANDLE myHandle = CreateThread(0, 0, streamVideo, &server, 0, &myThreadID);
			}


			robot.DriveDirect(0, 0);
			cvNamedWindow("Robot");

			int counter = 0;
			grab = 0;
			while (true)
			{

				double vx, vz;
				if (counter == 0)
					vx = vz = 0.0;

				if (mode == 's') {
					if ((nReadBytes = recv(server, buffer, bufsize, 0)) == SOCKET_ERROR) {
						if (WSAGetLastError() != WSAETIMEDOUT)
							break;
						else if (counter > 0)
							counter--;
					}
					else {
						counter = 3;

						vx = ((buffer[1] - '0')) / 10.0;
						if (buffer[0] == '-')
							vx *= -1;

						vz = ((buffer[4] - '0')) / 10.0;
						if (buffer[3] == '-')
							vz *= -1;
					}
				}
				else {
					char c = 0;
					c = cvWaitKey(30);
					switch (c) {
					case 'w': vx = +1; break;
					case 's': vx = -1; break;
					case 'a': vz = +1; break;
					case 'd': vz = -1; break;
					default: vx = 0; break;
					}
				}

				grab = 1;
				kin.GrabData(depthImg, colorImg, indexImg, pointImg);
				grab = 0;

				if (mode != 's' && mode != 'c')
					imshow("stream", colorImg);

				int status_wall = check_wall(depthImg);

				if (status_wall == 1) {
					vx *= 0.5;
					vz *= 0.5;
				}
				else if (status_wall == 2 || status_wall == 3) {
					vx = vx > 0 ? 0 : vx;
				}

				double vl = vx - vz;
				double vr = vx + vz;

				int velL = (int)(vl*Create_MaxVel);
				int velR = (int)(vr*Create_MaxVel);

				robot.DriveDirect(velL, velR);

				if (mode == 's')
					cvWaitKey(30);
			}

			if (mode == 's' || mode == 'c') {
				closesocket(server);
				closesocket(client);
			}

			robot.Disconnect();
		}
	}

	return 0;
}

//////////////////////////////////////////////
// debug
//////////////////////////////////////////////
/*
int x_wall = 50;
int y_wall = 50;

for (int i = 320 - x_wall / 2; i < 320 + x_wall / 2; i++) {
for (int j = 300 - y_wall / 2; j < 300 + y_wall / 2; j++) {
colorImg.at<Vec3b>(j*2, i*2) = Vec3b(255, 255, 255);
}
}
cout << colorImg.cols<<" " << colorImg.rows << endl;
cout << depthImg.at<USHORT>(320, 300) << endl;

*/