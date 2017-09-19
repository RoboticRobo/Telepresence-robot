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

bool isRecord = false;

int check_wall(Mat depthImg) {
	int x_wall = 50;
	int y_wall = 50;
	//int min_depth = 9999;
	for (int i = 320 - x_wall / 2; i < 320 + x_wall / 2; i++) {
		for (int j = 100 - y_wall / 2; j < 100 + y_wall / 2; j++) {
			int depth = depthImg.at<USHORT>(i, j);
			/*if (depth > min_depth && depth != 0) {
				min_depth = depth;
			}*/
			if (depth < 800 && depth > 600 && depth != 0) {
				
				cout << "wall" << endl;
				return 1;
			
			}
			else if(depth <=600 && depth!=0) {
				return 2;
			}
		}

	}
	return 0;
}

int main()
{
	int portNum = 1500;
	int client, server;
	bool isExit = false;
	const int bufsize = 1024;
	char buffer[bufsize];

	struct sockaddr_in server_addr;
	socklen_t size;

	WSADATA wsaData;
	server = 1;

	
	char mode = cvWaitKey(0);
	
	if(mode == 's' || mode == 'c') {

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR) {
			cout << "Error initialising WSA.\n" << endl;
			return -1;
		}

		client = socket(AF_INET, SOCK_STREAM, 0);

		if (client < 0)
		{
			cout << "\nError establishing socket..." << endl;
			exit(1);
		}


		cout << "\n=> Socket server has been created..." << endl;

		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htons(INADDR_ANY);
		server_addr.sin_port = htons(portNum);

		if ((bind(client, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0)
		{
			cout << "=> Error binding connection, the socket has already been established..." << endl;
			return -1;
		}

		size = sizeof(server_addr);
		cout << "=> Looking for clients..." << endl;

		listen(client, 1);

		int clientCount = 1;
		server = accept(client, (struct sockaddr *)&server_addr, &size);

		if (server < 0)
			cout << "=> Error on accepting..." << endl;
	}
	
	
	
	
	if (server > 0)
	{
		
		if (mode == 's' || mode == 'c') {
			strcpy(buffer, "=> Server connected...\n");
			send(server, buffer, bufsize, 0);
			cout << "=> Connected with the client #" << clientCount << ", you are good to go..." << endl;	
		}		

		
		if(mode == 'c') {

			while(true) {	
				cout << "Client: ";
				recv(server, buffer, bufsize, 0);
				cout << buffer << endl;
			}

		} else {
			CreateData	robotData;
			RobotConnector	robot;


			Mat depthImg;
			Mat colorImg;
			Mat indexImg;
			Mat pointImg;

			ofstream	record;
			record.open("../data/robot.txt");

			if (!robot.Connect(Create_Comport))
			{
				cout << "Error : Can't connect to robot @" << Create_Comport << endl;
				int a;
				cin >> a;
				return -1;
			}

			KinectConnector kin = KinectConnector();
			if (!kin.Connect()) {
				int a;
				cin >> a;

				return 1;
			}

			robot.DriveDirect(0, 0);
			cvNamedWindow("Robot");


			while (true)
			{

				kin.GrabData(depthImg, colorImg, indexImg, pointImg);
				imshow("depthImg", depthImg);
				imshow("colorImg", colorImg);

				char c;
				if( mode == 's' ) {
					cout << "Client: ";
					recv(server, buffer, bufsize, 0);
					cout << buffer << endl;
					c = buffer[0];
				}
				else {
					c = cvWaitKey(30);
				}

				if (c == 27) break;

				double vx, vz;
				vx = vz = 0.0;

				switch (c)
				{
					case 'w': vx = +1; break;
					case 's': vx = -1; break;
					case 'a': vz = +1; break;
					case 'd': vz = -1; break;
						//case ' ': vx = vz = 0; break;
					case 'c': robot.Connect(Create_Comport); break;
					default: vx = 1; break;
				}

				if (check_wall(depthImg)==1) {
					vx *= 0.5;
					vz *= 0.5;
				}else if (check_wall(depthImg) == 2) {
					if (vx > 0)
						vx = 0;
					
				}

				double vl = vx - vz;
				double vr = vx + vz;


				int velL = (int)(vl*Create_MaxVel);
				int velR = (int)(vr*Create_MaxVel);

				int color = (abs(velL) + abs(velR)) / 4;
				color = (color < 0) ? 0 : (color > 255) ? 255 : color;

				int inten = (robotData.cliffSignal[1] + robotData.cliffSignal[2]) / 8 - 63;
				inten = (inten < 0) ? 0 : (inten > 255) ? 255 : inten;

				//cout << color << " " << inten << " " << robotData.cliffSignal[1] << " " << robotData.cliffSignal[2] << endl;

				robot.LEDs(velL > 0, velR > 0, color, inten);





				if (!robot.DriveDirect(velL, velR))
					cout << "SetControl Fail" << endl;

				if (!robot.ReadData(robotData))
					cout << "ReadData Fail" << endl;

				if (isRecord)
					record << robotData.cliffSignal[0] << "\t" << robotData.cliffSignal[1] << "\t" << robotData.cliffSignal[2] << "\t" << robotData.cliffSignal[3] << endl;

				cout << "Robot " << robotData.infrared << endl;


				//cout << min_depth << endl;

			}
			
		
			robot.Disconnect();
		}
	}


	return 0;
}


