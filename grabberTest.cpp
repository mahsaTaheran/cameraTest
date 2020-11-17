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
		 //path_to_centroid=pcoConfiguration.getCentroidPath()+settingProfile+".csv";
				cout<<"path"<<path_to_centroid<<std::endl;
				centroidFile.open(path_to_centroid);
				centroidFile<<"imageCount,centroidX,centroidY,maxGreyValue,SNR,meanAfterThreshold\n";
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
	
	if(FgError_t error = pcoGrabber->setStarThresholding(0.5)){
	      std::cerr << pcoGrabber->fgLastErrorDescription() << error.fgError;
	      return error;
	}
	return 0;
}


int grabberTest::runGrab(int imageCount){
//todO MAKE COUNTfps OPTIONAL
//TODO MAKE SHOWIMAGE OPTIONAL

	auto start = std::chrono::high_resolution_clock::now();
	if (pcoConfiguration->isGrabCentroid()){

		auto centroiding_data = pcoGrabber->grabImageAndCentroid();
		std::cout << "Centroid: " << centroiding_data.centroiding_coordinates.x << " " << centroiding_data.centroiding_coordinates.y << "\n";
		if(centroiding_data.image.empty()){
			 std::cerr << "Empty image" << "\n";
			 return 1;
		}
		if (centroiding_data.centroiding_enum != centroiding::no_errors) {
		     std::cerr << "Error while centroiding... last fg error: " << pcoGrabber->fgLastErrorDescription() << "\n";
		     //this is not correct!
		}

		//also we should check for possible errors in pco and fg later
		showImageAndCentroid(centroiding_data);
		auto end = std::chrono::high_resolution_clock::now();
		time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
		std::cout<< (imageCount / (time.count() * 1e-9))<<"\n";
		if (pcoConfiguration->isSaveCentroid()){
		    saveCentroid(centroiding_data,imageCount);
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
		}
	}

/*
	grabberTest::saveFPS(){

	}

	grabberTest::countFPS(){
		time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	}
*/

