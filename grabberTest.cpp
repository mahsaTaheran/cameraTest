/*
 * grabberTest.cpp
 *
 *  Created on: Oct 29, 2020
 *      Author: mala
 */

//should initializes the setting within the constructor or in initialization maybe?

#include "grabberTest.h"
#include <sys/stat.h>

grabberTest::grabberTest(FrameGrabber *thisGrabber, Configuration *thisConfiguration): pcoGrabber(thisGrabber), pcoConfiguration(thisConfiguration){


}

grabberTest::~grabberTest(){

	//delete framegrabber

	//delete configuration
}

//TODO return value
int grabberTest::initialize(){


	if (PcoFgError_t error = pcoGrabber->init()) {
	        std::cerr << "FrameGrabber error: " << error.fgError << "PCO Error: " << error.pcoError;
	        std::cerr << pcoGrabber->pcoLastErrorDescription() << " " << pcoGrabber->fgLastErrorDescription() << '\n';
	        return error.fgError;
	}

	//here set the imagesize
	if(PcoError_t error = pcoGrabber->setImageSize(pcoConfiguration->getRoiWidth(), pcoConfiguration->getRoiHeight()){
	        	std::cerr << pcoGrabber->pcoLastErrorDescription() << "\n";
	    	}
//
	 if (PcoFgError_t error = pcoGrabber->startRecording()) {
	        std::cerr << pcoGrabber->pcoLastErrorDescription() << " " << pcoGrabber->fgLastErrorDescription() << '\n';
	        return error;
	    }

	if(PcoError_t error = pcoGrabber->setDelayAndExposure(pcoConfiguration->getDelayValue(), pcoConfiguration->getExposureValue())){
        	std::cerr << pcoGrabber->pcoLastErrorDescription() << "\n";
    	}
	settingProfile = pcoConfiguration->getSettingID();
	if(int result = setCentroidingSettings()){
		 std::cerr << result<< "\n";
		 return result;
	 }
	 
	 
	 
return 0;
}

int grabberTest::setCentroidingSettings(){
	 if (pcoConfiguration->isSaveCentroid()){
		 path_to_centroid =pcoConfiguration->getCentroidPath()+settingProfile+".csv";
		 //path_to_FPS=pcoConfiguration->getCentroidPath()+settingProfile+"FPS"+".csv";
		 //path_to_centroid=pcoConfiguration.getCentroidPath()+settingProfile+".csv";
				cout<<"path"<<path_to_centroid<<std::endl;
				centroidFile.open(path_to_centroid);
				centroidFile<<"imageCount,centroidX,centroidY,maxGreyValue,SNR,meanAfterThreshold\n";

				//FPSFile.open(path_to_FPS);
				//FPSFile<<"imageCount,FPS\n";
			}



	 if (pcoConfiguration->isSaveImage()){
		 path_to_image = pcoConfiguration->getImagePath()+settingProfile;
		int result= mkdir(path_to_image.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	 }

	pcoGrabber->setMinimumMean(pcoConfiguration->getMinimumMean());
	pcoGrabber->setMaximumMean(pcoConfiguration->getMaximumMean());
	pcoGrabber->setMaximumGreyValue(pcoConfiguration->getMaximumGreyValue());
	pcoGrabber->setMinimumSNR(pcoConfiguration->getMinimumSNR());
	pcoGrabber->setMaximumMeanAfterThresholding(pcoConfiguration->getMeanAfterTresholding());
	cameraImage = pcoGrabber->grabImage().image.clone();
	//uint16_t RoI[2]= {512,512};
	cv::Rect2i roi = {{1000, 1000}, cv::Size{pcoConfiguration->getRoiWidth(), pcoConfiguration->getRoiHeight()}};
	//cv::Rect2i roi{{1000, 1000}, cv::Size{RoI[0], RoI[1]}};
	if (FgError_t error = pcoGrabber->setCentroidingRoi(roi)) {
	    std::cerr << pcoGrabber->fgLastErrorDescription() << '\n';
	    return error;
	 }
	
	// NOTE (Joris Nonnast):
	//! This value should not be hardcoded. It should at least be a an entry in the config file.
	//! Note that the threshold heavily impacts the centroiding results.
	//! In an ideal scenario 60 % (0.6) should suffice. 
	//! Depending on the noise and other bright objects in the image, a value of 80 % (0.8) or higher could be required.
	if(FgError_t error = pcoGrabber->setStarThresholding(0.5)){
	      std::cerr << pcoGrabber->fgLastErrorDescription() << error.fgError;
	      return error;
	}
	return 0;
}


// NOTE (Joris Nonnast)
//! This is something we did unfortunately not test. The continuos grabbing without delay.
//! As we always displayed the images on screen. Despite having a very high pollrate (cv::waitKey(1))
//! The drawing of the image on screen does take some time. This might've been enough time to not spot the
//! anomalies that seem to appear on very high framerates.
//! As you told as, these anomalies seem to disappear on exposure times above 12ms. This should give a framerate of 
//! around 83 fps. If we or you are not able to resolve the issue, or the issue lies in the framegrabber not being able to handle this datarate

//! a possible workaround could be setting the delay value as such, that the sum of delay and exposure is above 12ms. 
int grabberTest::runGrab(int imageCount){
//todO MAKE COUNTfps OPTIONAL
//TODO MAKE SHOWIMAGE OPTIONAL

	auto start = std::chrono::high_resolution_clock::now();
	if (pcoConfiguration->isGrabCentroid()){
		//auto start = std::chrono::high_resolution_clock::now();
		auto centroiding_data = pcoGrabber->grabImageAndCentroid();
		std::cout << "Centroid: " << centroiding_data.centroiding_coordinates.x << " " << centroiding_data.centroiding_coordinates.y << "\n";

		if(centroiding_data.image.empty()){
			 std::cerr << "Empty image" << "\n";
			 return 1;
		}
		if (centroiding_data.centroiding_enum != centroiding::no_errors) {
			// NOTE (Joris Nonnast):
			//! I must admit that the naming of the enum values might be a bit confusing, good naming is hard.
			//! The centroiding enum, does not imply framegrabber errors. Framegrabber errors are forwarded from
			//! the silicon software sdk, failures in the centroiding algorithm are not part of it.
			
			//! Note, that the algorithm does not really fail inside the FPGA calculations. It will always return values.
			//! Those values might be faulty though.
			//! The settings like minimumSNR etc. lay an arbitrary threshold. We say, if the SNR, ... is too low/high, the values returned, are 
			//! very likely to be garbage. The image properties, like mean or maximum greyvalue are accessible through the image_info_roi member
			//! of the CentroidingResult struct returned by grabImageAndCentroid().
			
			//! The returned enum should be checked in a switch statement.
			//! You can find the enum values in the FrameGrabberHelper.hpp 
		     std::cerr << "Error while centroiding... last fg error: " << pcoGrabber->fgLastErrorDescription() << "\n";
		     //this is not correct!
		}
		/*auto end = std::chrono::high_resolution_clock::now();
		time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
		std::cout<< (imageCount / (time.count() * 1e-9))<<"\n";
		lastFPS=imageCount / (time.count() * 1e-9);

		*/

		//also we should check for possible errors in pco and fg later
		showImageAndCentroid(centroiding_data);
		auto end = std::chrono::high_resolution_clock::now();
		time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
		std::cout<< (imageCount / (time.count() * 1e-9))<<"\n";
		if (pcoConfiguration->isSaveCentroid()){
		    saveCentroid(centroiding_data,imageCount);
		    //saveFPS(currentFPS, imageCount);
		}

		if (pcoConfiguration->isSaveImage()){
		    saveImage(centroiding_data.image, imageCount);
		}

	}else if (pcoConfiguration->isGrabImage()){
			auto image_data = pcoGrabber->grabImage();
			if(image_data.image.empty()){
				std::cerr << "Empty image" << "\n";
				return 1;
			}
			if (image_data.fg_error) {
				std::cerr << "Error while imaging... last  error: " << pcoGrabber->fgLastErrorDescription() << "\n";
			}
			//showImage(image_data);
			cameraImage = image_data.image;
			cv::resize(cameraImage, cameraImage, {1024, 1024});
			cv::imshow("LastImage", cameraImage);
			auto end = std::chrono::high_resolution_clock::now();
			time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
			std::cout<< (imageCount / (time.count() * 1e-9))<<"\n";

			if (pcoConfiguration->isSaveImage()){
				saveImage(image_data.image, imageCount);
			}
	}else {
		std::cout<<"nothing to do!"<<"\n";
	}

	/*	if (countFPS){
			auto end = std::chrono::high_resolution_clock::now();
			countFPS(start, end);
			saveFPS(duration, imageCount)
		}*/
return 0;
}



void grabberTest::saveImage(const cv::Mat &image, int image_Count){

		std::string filepath= path_to_image+"/"+settingProfile+std::to_string(image_Count)+".tif";
		if (!pcoGrabber->write_image_async(filepath, image)){
			std::cout<<"save failed!"<<"\n";
		}
}

void grabberTest::showImageAndCentroid(const CentroidingResult centroiding_data){

				cameraImage = centroiding_data.image;
		        cv::circle(cameraImage, {(int) centroiding_data.centroiding_coordinates.x + pcoGrabber->getRoi().x,
		                           (int) centroiding_data.centroiding_coordinates.y + pcoGrabber->getRoi().y}, 20, {50000, 50000, 50000});
		        cv::rectangle(cameraImage, pcoGrabber->getRoi(), {500000, 50000, 50000});
		        cv::putText(cameraImage, "Max grey value: " + std::to_string(centroiding_data.image_info_roi.max_grey_value),
		                    {10, 10}, cv::FONT_HERSHEY_PLAIN, 1, {50000});
		        cv::putText(cameraImage, "SNR: " + std::to_string(centroiding_data.image_info_roi.signal_to_noise_ratio),
		                    {10, 30}, cv::FONT_HERSHEY_PLAIN, 1, {50000});
		        cv::putText(cameraImage, "Mean: " + std::to_string(centroiding_data.image_info_roi.mean_before_threshold),
		                    {10, 50}, cv::FONT_HERSHEY_PLAIN, 1, {50000});
		        cv::putText(cameraImage,
		                    "Mean after threshold: " + std::to_string(centroiding_data.image_info_roi.mean_after_treshold),
		                    {10, 70}, cv::FONT_HERSHEY_PLAIN, 1, {50000});
		        /*cv::putText(image, "Image count: " + std::to_string(image_count), {10, 90}, cv::FONT_HERSHEY_PLAIN, 1,
		                    {50000});*/
		        cv::resize(cameraImage, cameraImage, {1024, 1024});
		        cv::imshow("CentroidView", cameraImage);

	}

void grabberTest::showImage(const ImagingResult image_data){

		cv::Mat image = image_data.image;
		cv::resize(image, image, {1024, 1024});
		cv::imshow("LastImage", image);
	}

void grabberTest::saveCentroid(const CentroidingResult centroiding_data, int image_Count){
		centroidFile<<image_Count<<","<<centroiding_data.centroiding_coordinates.x<<","<<centroiding_data.centroiding_coordinates.y<<","<<centroiding_data.image_info_roi.max_grey_value<<","<<centroiding_data.image_info_roi.signal_to_noise_ratio<<","<<centroiding_data.image_info_roi.mean_after_treshold<<"\n";
	}

	void grabberTest::close(){
 if (PcoFgError_t error = pcoGrabber->stopRecording()) {
	        std::cerr << pcoGrabber->pcoLastErrorDescription() << " " << pcoGrabber->fgLastErrorDescription() << '\n';

	    }
		if (pcoConfiguration->isSaveCentroid()){
			centroidFile.close();
			//FPSFile.close();
		}
	}

/*
	void grabberTest::saveFPS(double currentFPS, int image_Count){
FPSFile<<image_Count<<","<<currentFPS<<"\n";
	}

	grabberTest::countFPS(){
		time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	}
*/

