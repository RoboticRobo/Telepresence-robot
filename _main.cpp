#include <stdio.h>
#include <iostream>

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


bool check_wall(Mat depthImg) {
	int x_wall = 10;
	int y_wall = 10;
	//int min_depth = 9999;
	for (int i = 320 - x_wall / 2; i < 320 + x_wall / 2; i++) {
		for (int j = 240 - y_wall / 2; j < 240 + y_wall / 2; j++) {
			int depth = depthImg.at<USHORT>(i, j);
			/*if (depth > min_depth && depth != 0) {
				min_depth = depth;
			}*/
			if (depth < 700 && depth != 0) {
				
				cout << "wall" << endl;
				return true;
			

			}
		}

	}
	return false;
}
int main()
{
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

		char c = cvWaitKey(30);
		if (c == 27) break;

		double vx, vz;
		vx = vz = 0.0;

		switch (c)
		{
		case 'w': vx = +1; break;
		case 's': vx = -1; break;
		case 'a': vz = +1; break;
		case 'd': vz = -1; break;
		case ' ': vx = vz = 0; break;
		case 'c': robot.Connect(Create_Comport); break;
		}

		if (check_wall(depthImg)) {
			vx = 0;
			vz = -1;
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

	return 0;
}


