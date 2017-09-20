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

DWORD WINAPI streamVideo(LPVOID lpParameter)
{
	int& server = *((int*)lpParameter);

	VideoCapture cap(1);

	while (true) {
		Mat src;
		Mat dst;
		Size size(160, 120);

		cap >> src;
		resize(src, dst, size);

		int dstSize = dst.total() * dst.elemSize();

		unsigned char * p = dst.data;
		const char * c = (const char *)p;

		if (send(server, c, dstSize, 0) == SOCKET_ERROR)
			break;

		cvWaitKey(40);
	}

	return 0;
}

int check_wall(Mat depthImg) {
	int x_wall = 200;
	int y_wall = 2;
	int count = 0;
	for (int i = 320 - x_wall / 2; i < 320 + x_wall / 2; i++) {
		for (int j = 300 - y_wall / 2; j < 300 + y_wall / 2; j++) {
			int depth = depthImg.at<USHORT>(j, i);

			if (depth == 0)
				count++;
			else if (depth < 800 && depth > 600)
				return 1;
			else if (depth <= 600)
				return 2;
		}
	}
	if (count == 400)
		return 3;
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
		}

		//////////////////////////////////////////////
		// video streaming
		//////////////////////////////////////////////

		if (mode == 's' || mode == 'c') {
			DWORD myThreadID;
			HANDLE myHandle = CreateThread(0, 0, streamVideo, &server, 0, &myThreadID);

			strcpy(buffer, "=> Server connected...\n");
			send(server, buffer, bufsize, 0);
		}


		//////////////////////////////////////////////
		// control robot
		//////////////////////////////////////////////
		if (mode == 'c') {

			while (true) {
				cout << "receive: ";
				if ((nReadBytes = recv(server, buffer, bufsize, 0)) == SOCKET_ERROR)
					break;

				cout << buffer << endl;
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

			robot.DriveDirect(0, 0);
			cvNamedWindow("Robot");

			while (true)
			{

				double vx, vz;
				vx = vz = 0.0;

				if (mode == 's') {
					cout << "receive: ";
					if ((nReadBytes = recv(server, buffer, bufsize, 0)) == SOCKET_ERROR)
						break;

					vx = ((buffer[1] - '0')) / 10.0;
					if (buffer[0] == '-')
						vx *= -1;

					vz = ((buffer[4] - '0')) / 10.0;
					if (buffer[3] == '-')
						vz *= -1;

					cout << "vx = " << vx << ", vz = " << vz << endl;
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

				kin.GrabData(depthImg, colorImg, indexImg, pointImg);

				if (mode != 's' && mode != 'c')
					imshow("colorImg", colorImg);

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