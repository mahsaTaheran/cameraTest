/*
 * Configuration.h
 *
 *  Created on: Oct 28, 2020
 *      Author: mala
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <boost/program_options.hpp>

#include <iostream>
#include <fstream>
#include <iterator>

using namespace std;
namespace po = boost::program_options;

template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    copy(v.begin(), v.end(), std::ostream_iterator<T>(os, " "));
    return os;
}


class Configuration {
public:


	/**
	 * @brief
	 * @details
	 * @param
	 * @return
	 */
	Configuration(int ac, char* av[]);
	virtual ~Configuration();
	void copyInputValues(int ac, char *av[]);
	/**
	 * @brief should show help and generic options, gets the config file path
	 * @details right now I put void as out put , but can be an error value of some sort in future
	 * @param
	 * @return
	 */
	void initialize();
	/**
	 * @brief adds the different options to program
	 * @details This method defines generic and config options. right now generic
	 * includes help and config (--help, --config or -c), the latter gets the path to
	 * a configuration file (should be a .ini file). config option includes all the
	 * key-value pairs in config file.
	 * At this point these options should be edited here, as this is a temporary code to use for tests
	 * @param
	 * @return
	 */
	void addOptions();
	void parseCommandLine(int argc, char* argv[]);

	 /**
	  * @brief
	  * @details
	  * @param
	  * @return
	  */
	void parseConfigFile();

//getters used by grabberTest class to get the settings

float getMinimumMean();
float getMaximumMean();
float getMinimumSNR();
uint16_t getMaximumGreyValue();
float getMeanAfterTresholding();
bool isGrabImage();
bool isGrabCentroid();
bool isSaveImage();
bool isSaveCentroid();
uint32_t getImageCount();
string getImagePath();
string getCentroidPath();
uint16_t *getRoi();
std::string getSettingID();


private:

//input from commandline
    	int inputCount;
    	char **inputString;

		//properties used by grabberTest to set the imaging properties
		uint32_t exposureValueMu;
		uint32_t delayValueMu;//should be changed to cv__Rect2i later

		//properties used by grabberTest to set the centroiding algorithm
		uint16_t Roi_Width;
		uint16_t Roi_Length;
		float minimumSNR;
		float minimumMean;
		float maximumMean;
		float meanAfterThresholding;
		uint16_t maximumGreyValue;

		//properties used by the grabberTest class to perform the operations
		bool grabImage;
		bool grabCentroid;
		bool saveImage;
		bool saveCentroid;
		uint32_t imageCount;

		//paths to reading config file and saving data
		std::string imagePath;
		std::string centroidPath;
		std::string config_file;
		std::string settingId;

		po::options_description *config;
		po::options_description *generic;
		po::options_description *visible;

        po::variables_map genericMap;
        po::variables_map configMap;


};
#endif /* CONFIGURATION_H_ */
