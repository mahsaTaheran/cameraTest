/*
 * grabberTest.h
 *
 *  Created on: Oct 29, 2020
 *      Author: mala
 */

#ifndef GRABBERTEST_H_
#define GRABBERTEST_H_

#include "FrameGrabber.h"
#include "Configuration.h"
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>


class grabberTest{

public:
	grabberTest(FrameGrabber *thisGrabber,Configuration *thisConfiguration);
	virtual ~grabberTest();
	int initialize();
	int setCentroidingSettings();
	//should include adding
	int runGrab(int imageCount);
	void close();

	string path_to_image;
	string path_to_centroid;
	string settingProfile;
	ofstream centroidFile;
private:

	void saveImage(const cv::Mat &image, int image_Count);
	void saveCentroid(const CentroidingResult centroiding_data, int image_Count);
	void showImage(const ImagingResult image_data);
	void showImageAndCentroid(const CentroidingResult centroiding_data);
	FrameGrabber *pcoGrabber =nullptr;
	Configuration *pcoConfiguration= nullptr;
	cv::Mat cameraImage;
	std::chrono::nanoseconds time{0};

};


#endif /* GRABBERTEST_H_ */
