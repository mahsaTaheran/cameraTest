/*
 * configTest.cpp
 *
 *  Created on: Oct 28, 2020
 *      Author: mala
 */


#include "Configuration.h"
#include "FrameGrabber.h"
#include "grabberTest.h"
#include <opencv2/highgui.hpp>



int main(int ac, char* av[])
{
	Configuration *pcoTests = new Configuration(ac,av);
	pcoTests->initialize();
	pcoTests->parseConfigFile();

	FrameGrabber *siso_grabber;
	grabberTest sisoTests = new grabberTest(siso_grabber, pcoTests);
	if (int result = sisoTests.initialize()){
		//return the error
		cout<<"framegrabber initialization failed with error"<<result<<std::endl;
	}
	int count = 0;

	if (pcoTests.imageCount!=0){
		while (count<= imageCount){
			int result= sisoTests.runGrab(count);
			if (result !=0){
				cout<<"grabbing failed at"<<count<<"with error"<<result<<std::endl;
			}
			count++;
		}
		int result =sisoTests.close();
	}else {
		while (cv::waitKey(100) != 'q')){
			int result= sisoTests.runGrab(count);
			if (result !=0){
				cout<<"grabbing failed at"<<count<<"with error"<<result<<std::endl;
			}
			count++;
		}

		int result =sisoTests.close();
	}
return 0;
}
