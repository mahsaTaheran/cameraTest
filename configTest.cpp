/*
 * configTest.cpp
 *
 *  Created on: Oct 28, 2020
 *      Author: mala
 */


#include "Configuration.h"
#include "FrameGrabber.h"
#include "grabberTest.h"
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#define PCO_ERRT_H_CREATE_OBJECT



int main(int ac, char* av[])
{
	Configuration *pcoTests = new Configuration(ac,av);
	pcoTests->initialize();
	pcoTests->parseConfigFile();
	FrameGrabber siso_grabber;
	grabberTest *sisoTests = new grabberTest(&siso_grabber, pcoTests);
	int result = sisoTests->initialize();
/*	if (result==0){
		//return the error
		cout<<"framegrabber initialization failed with error"<<result<<std::endl;
		return 5;
	}*/
	int count = 0;
	if (pcoTests->getImageCount()>0){
		while (count<= pcoTests->getImageCount()){
			int result= sisoTests->runGrab(count);
			if (result !=0){
				cout<<"grabbing failed at"<<count<<"with error"<<result<<std::endl;
			}
			count++;
		}
		sisoTests->close();
		delete sisoTests;
	}else {
		while (cv::waitKey(1) != 'q'){
			int result= sisoTests->runGrab(count);
			if (result !=0){
				cout<<"grabbing failed at"<<count<<"with error"<<result<<std::endl;
			}
			count++;
		}

		sisoTests->close();
		delete sisoTests;
	}
return 0;
}
