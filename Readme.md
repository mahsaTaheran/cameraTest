# Framegrabber Module

This API provides a common interface for the programmed SiliconSoftware
Framegrabber and the cameralink serial communication for the PCO Edge 4.2.

# Installation:
* in this project (ESBO_OBSW)
* pathway: ESBO_OBSW/STUDIO/Devices/FrameGrabber
* leads to the FrameGrabber software. To install it, some prequisites are required:
## Silicon Software libraries setup
Get the runtime:

https://dev.silicon-software.info/file-download/runtime-v5-7-0-for-linux-x64/ 
* download the zip and unpack it
* copy the contents with _sudo_ of Silicon Software Runtime folder into _/opt/siso_ (so the include-path is _/opt/siso/include_)
  change the folder name `mv siso... siso`
  move it _sudo mv siso /opt/_
* the symlink from lib to lib64 will be broken, you can fix this issue by running
_rm /opt/siso/lib_ and _ln -s /opt/siso/lib64 /opt/siso/lib_ in the siso directory
## Silicon Software drivers
Get the drivers: 

https://cloud.pco.de/index.php/s/28peFRJ3issCz3w/download?path=%2F&files=menable_linuxdrv_src_4.2.6.tar.bz2
* Follow install instructions in the INSTALL file.
* you can install it whereever you like.
* you need to add yourself to the video group as described in INSTALL
## PCO libraries setup
Get the pco libs: 

https://cloud.pco.de/index.php/s/21sfbf9bCNLmHMH
* copy contents of the PCO SDK folder into */opt/pco_camera* 
(so the include-path can be */opt/pco_camera/pco_common/pco_include*)
* cd into */opt/pco_camera/pco_me4*
* set the siso runtime install directory by running _export SISODIR5=/opt/siso_
* run _make_
* there will be errors, "undefined ..." which can be ignored. TBD
* all required object files should be generated now
## Install cmake-gui
_sudo apt  install cmake-qt-gui_
## FrameGrabber Install
* go back to the _framegrabber_ directory ( where _Readme.md_ lies) 
* create a folder one directory above (we recommend out of source build so the build folder is not located in the source folder)
_mkdir build_ 
* and open cmake gui in terminal, type  _cmake -gui_ 
* select pathway to project (e.g. **ESBO_OBSW/STUDIO/Devices/FrameGrabber**)
* where to build binaries (_/build_)
* press ***configure*** and press ***generate***

* _cd build_ into build directory and run _make_ . If it succeded you can execute the built executable by typing _sudo ./FrameGrabberTest_

## OpenCV setup
* run _sudo apt install libopencv-dev_ in the console
## Flash the Framegrabber
To use the hap file on the mE5 framegrabber it needs to be flashed first.

* To do this open the _sudo /opt/siso/bin/microDiagnostics_ application. (all executables in opt only work properly with sudo in front) 
* To flash the applet navigate to Tools -> Manage Firmware/Applet(s)
* You can select an applet by clicking the folder icon.
* Choose the second _FrameGrabberTest_ by clicking on the folder symbol
* Select **ESBO_OBSW/STUDIO/Devices/FrameGrabber/Hardware\ Applet/STUDIO_grab.hap**
* Press **yes**
* Right-click the entry **STUDIO_grab.hap** and activate this partition and set it as boot-partition
* Flashing the framegrabber and activating the partition is only needed the first time.
## HAP File must be able to be found
The file **STUDIO_grab.hap** should be copied to _/opt/siso/dll/mE5-MA-VCL_
as alternative the file path can be changed in the implementation:
* in _FrameGrabber.h_ the path has to be updated
* you need to build with _make_ again

## Almost there
* turn on the PCO hardware 
* and execute **sudo build/PCO_test**.
* It won't work properly, without sudo!
## Additional notes:
Currently the API is built with cmake. You should be able to create your own makefile if you take 
the current CMakeLists.txt as a basis.

The applet and camera communication were tested on a limited basis with a mE4VD4-CL framegrabber.


# Usage
## Errors
All functions return errors. You can check if a function returned an error by encapsulating it in an if condition: 
The errors are the forwarded SiliconSoftware framegrabber errors or the PCO serial com errors.
The type of the returned error gives a hint on where it could come from 
(either the serial interface or the framegrabber interface).
Example:
```
if(PcoError_t error = function_that_returns_pco_error()){
      //handle error
}
if(FgError_t error = function_that_returns_fg_error()){
     //handle error
}
```
## Basic Grabbing:
First step is to create a FrameGrabber object.
Then you have to initialize the grabber.
```
FrameGrabber grabber;
if(PcoFgError_t error = grabber.init()){
//handle errors
}
```
To get a more detailed description of the error you can call the fgLastErrorDescription and
pcoLastErrorDescription function.
```
std::string last_grabber_error = grabber.fgLastErrorDescription();
std::cerr << "Last framegrabber error: " << last_grabber_error << "\n";
```
Before you grab the first image you should set the delay and exposure as well as start the recording mode.
```
if(PcoError_t error = grabber.setDelayAndExposure(10, 10000){
//handle error
}
if(FgError_t error = grabber.startRecording()){
//handle error
}
```
If you want to grab an image you can do so with the grabImage function.
```
ImagingResult result = grabber.grabImage();
if(result.fgError){
//handle error
}
cv::Mat image = result.image;
```
You can write the image to disk with the write_image_async function. This functions opens a new 
thread to write the image. When the grabber object is destroyed, it will wait for all writing threads 
to end.
Note: The write_image_async function only writes the image if the hardware thread limit is not exhausted.
If the thread limit is reached the function returns false and no image is written.
```
grabber.write_image_async("file_path/image.tif", image);
```
You can get the camera status by calling the getCameraStatus function
```
CameraStatus status = grabber.getCameraStatus();
if(status.pcoError){
//handle error
}
short sensor_temperature = status.temperature.sensor;
unsigned warning_bits = status.health.warning_bits;
if(warning_bits){
//handle warnings
}
```
## Usage Of The Centroiding API
To get the center of gravity of the guide-star you should first set the region of interest.
```
int x, y = 0;
int width, height = 500;
cv::Rect2i roi{{x, y}, cv::size{width, height}};
if(FgError_t error = grabber.setCentroidingRoi(roi)){
//handle error
}
```
The algorithm fails on certain rejection conditions. You can set those manually by calling the 
appropriate functions.
```
grabber.setMinimumSNR(8);
grabber.setMaximumMean(500);
```
You should also set the thresholding condition. The condition is a percentage of the maximum of the image. 
For example 60 %.
```
grabber.setStarThresholding(0.6);
```
To get an image with centroids call the grabImageAndCentroid function.
```
CentroidingResult result = grabber.grabImageAndCentroid();
if(result.centroiding_enum != centroiding::no_errors){
    if(result.fgError){
        //handle error
    }
//handle error
}
cv::Mat image = result.image;
float x = result.centroiding_coordinates.x; 
float y = result.centroiding_coordinates.y; 
```
You can get additional image information from the centroiding result.
```
float image_mean = result.image_info_roi.mean_before_threshold;
```
To write just the region of interest to disk you can select it from the cv::Mat with the 
rect from the getRoi function.
```
cv::Rect2i roi = grabber.getRoi();
grabber.write_image_async("file_path/image_name.tif", image(roi));
``` 
