#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <conio.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <Windows.h>

#include "RobotConnector.h"

#include "cv.h"
#include "highgui.h"

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
#define PORT_NUM 1500
#define IP_SERVER "192.168.43.131"
#define RADIUS 150

bool isRecord = false;
int drag = 0, select_flag = 0;
int mouseX, mouseZ;
bool mouseClick = 0;

boolean inCircle(int x, int y, int r) {
	return (x*x) + (y*y) <= (r*r);
}


DWORD WINAPI streamVideo(LPVOID lpParameter)
{
	int& client = *((int*)lpParameter);

	Mat img = Mat::zeros(96, 128, CV_8UC3);
	int imgSize = img.total() * img.elemSize();
	uchar *iptr = img.data;

	while (true) {
		cout << "p";
		if (recv(client, (char *)iptr, imgSize, MSG_WAITALL) != SOCKET_ERROR) {
			cout << "AAAAA";
			Mat dst; Size size(640, 480);
			resize(img, dst, size);

			imshow("CV Video Client", dst);
			cvWaitKey(10);
		}
	}

	return 0;
}

void mouseCallBack(int event, int x, int y, int flags, void* userdata)
{
	x -= RADIUS;
	y -= RADIUS;

	if (event == EVENT_LBUTTONDOWN || (event == EVENT_MOUSEMOVE && mouseClick))
	{
		if (inCircle(x, y, RADIUS)) {
			mouseClick = 1;
			mouseZ = (-1)*(x);
			mouseX = (-1)*(y);
		}
	}
	else if (event == CV_EVENT_LBUTTONUP)
		mouseClick = 0;
}

int main()
{
	//////////////////////////////////////////////
	// socket
	//////////////////////////////////////////////
	int client;
	const int bufsize = 1024;
	char buffer[bufsize];
	char* ip = IP_SERVER;

	struct sockaddr_in server_addr;

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR) {
		cout << "\nError initialising WSA. Error code: " << WSAGetLastError() << endl;
		return -1;
	}


	if ((client = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR) {
		cout << "\nError establishing socket.. Error code: " << WSAGetLastError() << endl;
		return -1;
	}

	cout << "\n=> Socket client has been created..." << endl;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	inet_pton(AF_INET, ip, &server_addr.sin_addr);

	if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		cout << "\n Socket has been established. Error code: " << WSAGetLastError() << endl;
		return -1;
	}

	cout << "=> Connection to the server " << inet_ntoa(server_addr.sin_addr) << " with port number: " << PORT_NUM << endl;

	cout << "=> Awaiting confirmation from the server..." << endl;
	recv(client, buffer, bufsize, 0);
	cout << "=> Connection confirmed, you are good to go..." << endl;


	//DWORD timeout = 30;
	//setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));


	DWORD myThreadID;
	HANDLE myHandle = CreateThread(0, 0, streamVideo, &client, 0, &myThreadID);


	//////////////////////////////////////////////
	// control
	//////////////////////////////////////////////
	Mat mat(RADIUS * 2, RADIUS * 2, CV_8UC3, Scalar(0, 0, 0));
	circle(mat, Point(RADIUS, RADIUS), RADIUS, Scalar(255, 255, 255), -1);
	imshow("Control", mat);
	setMouseCallback("Control", mouseCallBack, NULL);

	while (true) {
		Mat matClick = mat.clone();

		if (mouseClick) {
			circle(matClick, Point(-1 * mouseZ + RADIUS, -1 * mouseX + RADIUS), 40, Scalar(0, 0, 0), -1);

			double tempx = (1.0 * mouseX / (RADIUS + 1));
			double tempz = (1.0 * mouseZ / (RADIUS + 1));
			double length = sqrt(tempx*tempx + tempz*tempz);

			int vx = (int)(tempx * 10.0 * length);
			int vz = (int)(tempz * 10.0 * length);

			string str_send = "";
			str_send += vx >= 0 ? "+" : "-";
			str_send += to_string(abs(vx));
			str_send += "|";
			if (vx < 0)
				vz *= -1;
			str_send += vz >= 0 ? "+" : "-";
			str_send += to_string(abs(vz));
			str_send += '\0';

			//			cout << "send: " << str_send << "=> mean: " << vx - vz << "|" << vx + vz << endl;
			send(client, str_send.c_str(), str_send.size(), 0);
		}

		imshow("Control", matClick);
		cvWaitKey(100);
	}

	return 0;
}