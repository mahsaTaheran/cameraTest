/*
 * configTest.cpp
 *
 *  Created on: Oct 28, 2020
 *      Author: mala
 */


#include "Configuration.h"
#include "grabberTest.h"
#include "FrameGrabber.h"
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#define PCO_ERRT_H_CREATE_OBJECT



int main(int ac, char* av[])
{
	Configuration pcoConfig(ac,av);
	pcoConfig.initialize();
	pcoConfig.parseConfigFile();
	FrameGrabber siso_grabber;
	grabberTest sisoTests(&siso_grabber, &pcoConfig);
	int result = sisoTests.initialize();
	/*if (result==0){
			//return the error
		cout<<"framegrabber initialization failed with error"<<result<<std::endl;
		return 5;
	}*/
	int count = 0;
	if (pcoConfig.getImageCount()>0){
		while ((count<= pcoConfig.getImageCount())&& (cv::waitKey(1) != 'q')){
			int result= sisoTests.runGrab(count);
				if (result !=0){
					cout<<"grabbing failed at"<<count<<"with error"<<result<<std::endl;
				}
				count++;
			}
			sisoTests.close();
		/*}else {
			while (cv::waitKey(1) != 'q'){
				int result= sisoTests.runGrab(count);
				if (result !=0){
					cout<<"grabbing failed at"<<count<<"with error"<<result<<std::endl;
				}
				count++;
			}

			sisoTests.close();*/
		}
	return 0;
}
	
	
