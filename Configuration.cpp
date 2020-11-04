/*
 * configBoost.cpp
 *
 *  Created on: Oct 28, 2020
 *      Author: mala
 */

#include"Configuration.h"
#include <iostream>
#include <fstream>
#include <iterator>


Configuration::Configuration(int ac, char* av[]){

	config = new po::options_description("configuration Options");
	generic = new po::options_description("generic Options");
	visible = new po::options_description("alloowed Options");
	copyInputValues(ac,av);
}
Configuration::~Configuration(){
	//need to delete sth?

}

void Configuration::copyInputValues(int ac, char* av[]){

	inputCount = ac;
	if (ac > 0)
	{
		inputString = new char *[ac];
	}
	for (int n = 0; n < ac; n++)
	{
	  inputString[n] = new char[strlen(av[n])+1];
	  strcpy(inputString[n], av[n]);
	}


/*	std::cout << "name of program: " << av[0] << '\n' ;
	std::cout << "name of program: " << inputString[0] << '\n' ;

	   if( ac> 1 )
	   {
	       std::cout << "there are " << ac-1 << " (more) arguments, they are:\n" ;

	       std::copy( av+1, av+ac, std::ostream_iterator<const char*>( std::cout, "\n" ) ) ;
	   }*/
}


void Configuration::initialize(){

	addOptions();
	parseCommandLine(inputCount, inputString);
	cout << "config file: " << config_file << "\n";
}

void Configuration::addOptions(){
	generic->add_options()
				("help", "help message")//TODO: edit help message later
				("config,c", po::value <string> (&config_file)->default_value("pcoConfig.ini"),"Path to config file, variables to pass are explained in help.")
//we can make it is_required() , so he code does not go forward if it is not given

				;
	config->add_options()
		("exposure", po::value<uint32_t>(&exposureValueMu)->default_value(10),"camera exposure, default value is...")
		("delay", po::value<uint32_t> (&delayValueMu)->default_value(0), "camera delay value, default value is...")
		("Roi_Width", po::value<uint16_t>(&Roi_Width)->default_value(250), "Centroiding RoI width,default value is ...")
		("Roi_Length", po::value<uint16_t>(&Roi_Length)->default_value(100), "Centroiding RoI length,default value is ...")
		("minimum_SNR", po::value<float>(&minimumSNR)->default_value(0), "Minimum SNR required for centroiding,default value is ...")
		("minimum_Mean", po::value<float>(&minimumMean)->default_value(0), "Minimum mean required for centroiding,default value is ...")
		("maximum_Mean", po::value<float>(&maximumMean)->default_value(50000), "Maximum mean required for centroiding,default value is ...")
		("meanAfterThreshlding", po::value<float>(&meanAfterThresholding)->default_value(100000), "meanAfterThresholding required for centroiding,default value is ...")
		("maximumGreyValue", po::value<uint16_t>(&maximumGreyValue)->default_value(1000), "maximum grey value width,default value is ...")
		("saveImage", po::value<bool>(&saveImage)->default_value(true), "If you want the images saved,default value is true. also provide a path to save")
		("saveCentroid", po::value<bool>(&saveImage)->default_value(true), "If you want the centroids saved,default value is true. also provide a path to save")
		("grabImage", po::value<bool>(&grabImage)->default_value(true), "if you want to grab image,default value is true")
		("grabCentroid", po::value<bool>(&grabCentroid)->default_value(true), "if you want to grab centroid,default value is true")
		("imagePath", po::value<std::string>(&imagePath)->default_value("/images/"), "path to the directory to save images,default value is ...")
		("centroidPath", po::value<std::string>(&centroidPath)->default_value("/centroids/"), "path to the file to save centroids,default value is ...")
		("imageCount", po::value<uint32_t>(&imageCount)->default_value(10),"Number of Images to take, default value is...")
		("settingID", po::value<std::string>(&settingId),"settingID for the configuration, required")
		;
	  visible->add(*generic).add(*config);
}

void Configuration::parseCommandLine(int argc, char* argv[]){

	store(po::command_line_parser(argc,argv).options(*generic).run(),genericMap);
	notify(genericMap);
	if (genericMap.count("help")) {
		std::cout << *visible << "\n";

	}

	if(!genericMap.count ("config")){
		std::cout<<"no config file is provided, add a config file and run again"<<std::endl;
		//should break or sth?
	}else{
		std::cout<<"looking in to"<<config_file<<std::endl;
	}

}
void Configuration::parseConfigFile(){
	        ifstream ifs(config_file.c_str());
	        std::ifstream f;
	            f.open(config_file);

	            // after open, check f and throw std::system_error with the errno
	            if (!f){
	                throw std::system_error(errno, std::system_category(), "failed to open "+config_file);
	            }
	            std::clog << "opened " << config_file << std::endl;
	        if(!ifs){
	        	std::cout<<"can not open config file:"<< config_file<<"run the program again with a correct file"<<"\n";
	        	return;
	        }else {

	        	store(po::parse_config_file(ifs,*config),configMap);
	        	notify(configMap);
	        	std::cout<<"reading ended with"<<exposureValueMu<<std::endl;
	        }

}

float Configuration::getMinimumMean(){
	return minimumMean;
}
float Configuration::getMaximumMean(){
	return maximumMean;
}
float Configuration::getMinimumSNR(){
	return minimumSNR;
}
uint16_t Configuration::getMaximumGreyValue(){
	return maximumGreyValue;
}
float Configuration::getMeanAfterTresholding(){
	return meanAfterThresholding;
}
bool Configuration::isGrabImage(){
	return grabImage;
}
bool Configuration::isGrabCentroid(){
	return grabCentroid;
}
bool Configuration::isSaveImage(){
	return saveImage;
}
bool Configuration::isSaveCentroid(){
	return saveCentroid;
}
uint32_t Configuration::getImageCount(){
	return imageCount;
}


string Configuration::getImagePath(){
	return imagePath;
}
string Configuration::getCentroidPath(){
	return centroidPath;
}


string Configuration::getSettingID(){
	return settingId;
}
/*
uint16_t *Configuration::getRoi(){
	uint16_t *Roi[2]={0,0};
	Roi[0]= Roi_Width;
	Roi[1]= Roi_Length;
	return Roi;
}
*/

bool isShowImage(){
	return true;
}


