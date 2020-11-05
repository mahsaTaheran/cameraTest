/*
 * grabberTest.cpp
 *
 *  Created on: Oct 29, 2020
 *      Author: mala
 */

//should initializes the setting within the constructor or in initialization maybe?

#include "grabberTest.h"

grabberTest::grabberTest(FrameGrabber *thisGrabber, Configuration* thisConfiguration){

	pcoGrabber = thisGrabber;
	pcoConfiguration = thisConfiguration;
}

grabberTest::~grabberTest(){

	//delete framegrabber
	//delete configuration
}

//TODO return value
int grabberTest::initialize(){
	PcoFgError_t error = pcoGrabber->init();
	cout<<"framegrabber initialized"<<std::endl;
	if (error) {
	        std::cerr << "FrameGrabber error: " << error.fgError << "PCO Error: " << error.pcoError;
	        std::cerr << pcoGrabber->pcoLastErrorDescription() << " " << pcoGrabber->fgLastErrorDescription() << '\n';
	        return error.fgError;
	}

	 if (PcoFgError_t error = pcoGrabber->startRecording()) {
	        std::cerr << pcoGrabber->pcoLastErrorDescription() << " " << pcoGrabber->fgLastErrorDescription() << '\n';
	        return error;
	    }
	settingProfile = pcoConfiguration->getSettingID();
	if(int result = setCentroidingSettings()){
		 return result;
	 }
return 0;
}

int grabberTest::setCentroidingSettings(){
	 if (pcoConfiguration->isSaveCentroid()){
		 path_to_centroid =pcoConfiguration->getCentroidPath();

	 }

	 if (pcoConfiguration->isSaveImage()){
		 path_to_image = pcoConfiguration->getImagePath();
	 }


	uint16_t RoI[2]= {100,100};
	//RoI = pcoConfiguration->getRoi();
	cv::Rect2i roi{{250, 100}, cv::Size{RoI[0], RoI[1]}};
	    if (FgError_t error = pcoGrabber->setCentroidingRoi(roi)) {
	        std::cerr << pcoGrabber->fgLastErrorDescription() << '\n';
	        return error;
	    }


	pcoGrabber->setMinimumMean(pcoConfiguration->getMinimumMean());
	pcoGrabber->setMaximumMean(pcoConfiguration->getMaximumMean());
	pcoGrabber->setMaximumGreyValue(pcoConfiguration->getMaximumGreyValue());
	pcoGrabber->setMinimumSNR(pcoConfiguration->getMinimumSNR());
	pcoGrabber->setMaximumMeanAfterThresholding(pcoConfiguration->getMeanAfterTresholding());


	if(FgError_t error = pcoGrabber->setStarThresholding(0.6)){
	            std::cerr << pcoGrabber->fgLastErrorDescription() << error.fgError;
	return error;
	}
	return 0;
}
int grabberTest::runGrab(int imageCount){
		/*if (countFPS){
			auto start = std::chrono::high_resolution_clock::now();
		}*/
		if (pcoConfiguration->isGrabCentroid()){
			auto centroiding_data = pcoGrabber->grabImageAndCentroid();
		    if (centroiding_data.centroiding_enum != centroiding::no_errors) {
		        std::cerr << "Error while centroiding... last fg error: " << pcoGrabber->fgLastErrorDescription() << "\n";
		        return 1;
		    }

		    if (pcoConfiguration->isShowImage()){
		   		showImageAndCentroid(centroiding_data);
		   	}


		    if (pcoConfiguration->isSaveCentroid()){

		    	int result = saveCentroid(centroiding_data,imageCount);
		    	if (result !=0){
		    		std::cerr << "Error while saving centroid" << "\n";
		    		return result;
		    	}
		    }

		    //save image
		    if (pcoConfiguration->isSaveImage()){
		    			//saveImage
		    	int result = saveImage(centroiding_data.image, imageCount);
		    	if (result !=0){
		    		std::cerr << "Error while saving image" << "\n";
		    			    		return result;
		    			    }
		    }

			//show image

		}else if (pcoConfiguration->isGrabImage()){
			//only grab image
			auto image_data = pcoGrabber->grabImage();
			if (image_data.fg_error) {
				std::cerr << "Error while imaging... last  error: " << pcoGrabber->fgLastErrorDescription() << "\n";
				return 2;
			}

			if (pcoConfiguration->isSaveImage()){
				//saveImage
				int result = saveImage(image_data.image, imageCount);
				if (result !=0){
					std::cerr << "Error while saving image" << "\n";
				    	return result;
				}
			}

			if (pcoConfiguration->isShowImage()){
				showImage(image_data);
			}


		}else {

			std::cout<<"nothing to do!"<<std::endl;
			return 0;
		}

	/*	if (countFPS){
			auto end = std::chrono::high_resolution_clock::now();
			countFPS(start, end);
			saveFPS(duration, imageCount)
		}*/
return 0;
}



	int grabberTest::saveImage(const cv::Mat &image, int image_Count){

		//const std::string & create the file path string = path to image+"pcoTest"+settingProfile+imageCount+"tif";
		//image is centroidingData.image;
		std::string filepath= path_to_image+"pcoTest"+settingProfile+std::to_string(image_Count)+".tif";
		if (!pcoGrabber->write_image_async(filepath, image)){
						return 4;
		}
return 0;
}

	void grabberTest::showImageAndCentroid(const CentroidingResult centroiding_data){

				cv::Mat image = centroiding_data.image;
		        cv::circle(image, {(int) centroiding_data.centroiding_coordinates.x + pcoGrabber->getRoi().x,
		                           (int) centroiding_data.centroiding_coordinates.y + pcoGrabber->getRoi().y}, 20, {50000, 50000, 50000});
		        cv::rectangle(image, pcoGrabber->getRoi(), {500000, 50000, 50000});
		        cv::putText(image, "Max grey value: " + std::to_string(centroiding_data.image_info_roi.max_grey_value),
		                    {10, 10}, cv::FONT_HERSHEY_PLAIN, 1, {50000});
		        cv::putText(image, "SNR: " + std::to_string(centroiding_data.image_info_roi.signal_to_noise_ratio),
		                    {10, 30}, cv::FONT_HERSHEY_PLAIN, 1, {50000});
		        cv::putText(image, "Mean: " + std::to_string(centroiding_data.image_info_roi.mean_before_threshold),
		                    {10, 50}, cv::FONT_HERSHEY_PLAIN, 1, {50000});
		        cv::putText(image,
		                    "Mean after threshold: " + std::to_string(centroiding_data.image_info_roi.mean_after_treshold),
		                    {10, 70}, cv::FONT_HERSHEY_PLAIN, 1, {50000});
/*		        cv::putText(image, "Image count: " + std::to_string(image_count), {10, 90}, cv::FONT_HERSHEY_PLAIN, 1,
		                    {50000});*/
		        cv::imshow("CentroidView", image);
	}

	void grabberTest::showImage(const ImagingResult image_data){

		cv::Mat image = image_data.image;
		cv::imshow("LastImage", image);

	}

	int grabberTest::saveCentroid(const CentroidingResult centroiding_data, int image_Count){
		if (!centroidFile.is_open()){
			centroidFile.open(path_to_centroid);
			centroidFile<<"imageCount,centroidX,centroidY\n";
		}
		centroidFile<<image_Count<<","<<centroiding_data.centroiding_coordinates.x<<","<<centroiding_data.centroiding_coordinates.y<<"\n";

		//if error happens return 3
	return 0;
	}

	void grabberTest::close(){
		if (pcoConfiguration->isSaveImage()){
			centroidFile.close();
		}
		//close the centroid file
	}

/*
	grabberTest::saveFPS(){

	}

	grabberTest::countFPS(){
		time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	}
*/

