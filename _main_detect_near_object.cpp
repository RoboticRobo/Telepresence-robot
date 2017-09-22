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

Mat depthImg;
Mat colorImg;
Mat indexImg;
Mat pointImg;

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
		cout << depthImg.at<USHORT>(y / 2, x / 2) << endl;
	}
}

int main() {
	KinectConnector kin = KinectConnector();
	if (!kin.Connect()) return 1;

	while (true) {

		kin.GrabData(depthImg, colorImg, indexImg, pointImg);

		setMouseCallback("colorImg", CallBackFunc, NULL);

		for (int i = 0; i<1280; i++) {
			for (int j = 0; j < 960; j++) {
				if (depthImg.at<USHORT>(j / 2, i / 2) == 0)
					colorImg.at<Vec3b>(j, i) = Vec3b(0, 0, 255);
				else if (depthImg.at<USHORT>(j / 2, i / 2) <= 600)
					colorImg.at<Vec3b>(j, i) = Vec3b(255, 0, 0);
				else if (depthImg.at<USHORT>(j / 2, i / 2) <= 800)
					colorImg.at<Vec3b>(j, i) = Vec3b(0, 255, 0);

				if (j == 480) {
					if (i >= 540 && i <= 740) {
						colorImg.at<Vec3b>(j, i) = Vec3b(0, 0, 0);
					}
				}
			}
		}

		//imshow("depthImg", depthImg);
		imshow("colorImg", colorImg);

		waitKey(100);
	}
	return 0;
}