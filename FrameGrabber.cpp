#include "FrameGrabber.h"

// define to create error text functions
#define PCO_ERRT_H_CREATE_OBJECT

// fix pco error lib compatibility
#define sprintf_s snprintf
#include <iostream>
#include <PCO_err.h>
#include <PCO_errt.h>
constexpr unsigned TIME_BASE_MU = 1;
constexpr unsigned TRIGGER_AUTO = 0;
constexpr unsigned DMA_INDEX = PORT_A;
constexpr int DIV_BY_ZERO = -2048;
//TODO check with applet implementation
/**
 * @brief This struct is used internally to get the image information from the image buffer.
 */
struct internal_parse{
    uint64_t max_gv; ///< Maximum grey value of the roi.
    uint64_t mean; ///< Mean of the roi before thresholding.
    uint64_t mean_after_threshold; ///< Mean of the roi after thresholding.
    uint64_t x; ///< The center of gravity x.
    uint64_t y; ///< The center of gravity y.
};

/**
 * @brief To initialize the FrameGrabber device call init()
 * @details This constructor does no initialization of the interfaces.
 */
FrameGrabber::FrameGrabber() : pco_serial_com_() {}

/**
 * @brief Cleanup of memory and camera serial handle.
 * @details This is the destructor. There should be no need to call it manually.
 */
FrameGrabber::~FrameGrabber() {
    Fg_stopAcquireEx(fg_, PORT_A, frame_buffer_, STOP_ASYNC_FALLBACK);
    Fg_FreeMemEx(fg_, frame_buffer_);
    Fg_FreeGrabber(fg_);
    pco_serial_com_.Close_Cam();
    while(thread_count_ > 0){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

/**
 * @brief This function initializes the FrameGrabber and the PCO Serial Connection. Call this after construction.
 * @details The default recording mode is OFF (false)
 * @return A struct containing any Serial or FrameGrabber errors. In case of no error the hasError member is set to false.
 */
PcoFgError_t FrameGrabber::init() {

    if(init_serial_impl().hasError) return {handle_pco_error_internal().pcoError, FG_OK};

    if(init_fg_impl().hasError) return {PCO_NOERROR, handle_fg_error_internal().fgError};
    return {};
}

/**
 * @brief Set the centroiding roi
 * @details You can set the centroiding roi any time. Ideally the width of the roi is divisible by 8.
 * There generally should not be any problems if it isn't. But be aware that this can be a source of errors. The frame grabber
 * fills the excess roi width undefined values.
 * Be aware that the bigger the region of interest the better the stability of the centroiding
 * We do not recommend using a region of interest smaller than 256x256
 * @param region_of_interest the region of interest for the centroiding operation as cv::Rect
 * example cv::Rect{cv::Point{x, y}, cv::Size{width, height}}
 * maximum roi size is 512x512 (if you set a bigger size it will be clamped to 512x512)
 * @return The corresponding FrameGrabber error in case of no error this evaluates to false
 */
FgError_t FrameGrabber::setCentroidingRoi(const cv::Rect2i &region_of_interest) {
    centroiding_roi_ = region_of_interest;
    centroiding_roi_.width = std::min(centroiding_roi_.width, 512);
    centroiding_roi_.height = std::min(centroiding_roi_.height, 512);
    if(Fg_setParameterWithType(fg_, ROI_X_ID, static_cast<int32_t>(centroiding_roi_.x), DMA_INDEX) != FG_OK) return handle_fg_error_internal();
    if(Fg_setParameterWithType(fg_, ROI_Y_ID, static_cast<int32_t>(centroiding_roi_.y), DMA_INDEX) != FG_OK) return handle_fg_error_internal();
    if(Fg_setParameterWithType(fg_, ROI_WIDTH_ID, static_cast<int32_t>(centroiding_roi_.width), DMA_INDEX) != FG_OK) return handle_fg_error_internal();
    if(Fg_setParameterWithType(fg_, ROI_HEIGHT_ID, static_cast<int32_t>(centroiding_roi_.height), DMA_INDEX) != FG_OK) return handle_fg_error_internal();
    return {};
}

/**
 * @brief Returns the currently set region of interest, which is used for centroiding.
 * @return The currently set region of interest as cv::Rect2i.
 */
cv::Rect2i FrameGrabber::getRoi() {
    return centroiding_roi_;
}

/**
 * @brief Set the delay and exposure in mu
 * @details Be aware, that if you set the delay or exposure so that the maximum framerate of the camera is slower than the maximum
 * framerate of the frame grabber (ca. 120 fps) and you are polling images at that rate, it is possible, that you grab duplicate images
 * This means it is possible to grab n times and get n identical images.
 * @param delay_in_mu the delay in mu
 * @param exposure_in_mu the exposure in mu
 * @return The PCO Error/Warning in case of no error this evaluates to false
 */
PcoError_t FrameGrabber::setDelayAndExposure(uint32_t delay_in_mu, uint32_t exposure_in_mu) {
    if((last_pco_error_ = pco_serial_com_.PCO_SetDelayExposureTime(delay_in_mu, exposure_in_mu, TIME_BASE_MU, TIME_BASE_MU)) != PCO_NOERROR) return handle_pco_error_internal();
    if(!recording){
        if((last_pco_error_ = pco_serial_com_.PCO_ArmCamera() != PCO_NOERROR)){
            return handle_pco_error_internal();
        }
    }
    return {};
}

/**
 * @brief Set the thresholds for the guide star segmentation.
 * @details Pixels below the percentage times the maximum of the image will be set to zero
 * This is internal to the FPGA, the returned images will be originals.
 * @param percentage_of_maximum
 * @return The corresponding FrameGrabber errors in case of no errors this evaluates to false
 */
FgError_t FrameGrabber::setStarThresholding(float percentage_of_maximum) {
    int percentage_shifted = percentage_of_maximum*255;
    if(Fg_setParameterWithType(fg_, LOWER_THRESHOLD_ID, percentage_shifted, DMA_INDEX) != FG_OK) return handle_fg_error_internal();
    return {};
}


/**
 * @brief Start the recording. This function has to be called before grabbing an image
 * @details Before an image can be grabbed, the camera has to be set to the recording state and the FrameGrabber has to
 * start the acquisition, this function does both.
 * @return A struct containing both PCO and FG Error in case of no error this evaluates to false
 */
PcoFgError_t FrameGrabber::startRecording() {
    if(recording) return {};
    if((last_pco_error_ = pco_serial_com_.PCO_SetRecordingState(1)) != PCO_NOERROR) return {handle_pco_error_internal().pcoError, FG_OK};
    if((Fg_AcquireEx(fg_, DMA_INDEX, GRAB_INFINITE, ACQ_STANDARD, frame_buffer_)) != FG_OK) return {PCO_NOERROR, handle_fg_error_internal().fgError};
    recording = true;
    return {};
}
/**
 * @brief Stops the recording and the image acquisition
 * @return The PCO Error/Warning in case of no error this evaluates to false
 */
PcoFgError_t FrameGrabber::stopRecording() {
    if(!recording) return {};
    if(Fg_stopAcquireEx(fg_, DMA_INDEX, frame_buffer_, STOP_ASYNC_FALLBACK) != FG_OK) return {0, handle_fg_error_internal().fgError};
    if((last_pco_error_ = pco_serial_com_.PCO_SetRecordingState(0)) != PCO_NOERROR) return {handle_pco_error_internal().pcoError, 0};
    recording = false;
    last_frame_num_ = 1;
    return {};
}

/**
 * @brief This function is used to acquire a full image and the centroid coordinates relative to the
 * roi and errors which occurred while centroiding
 * @details This function also checks against the error thresholds, for mean centroiding coordinate delta to last coordinate and noise threshold (to be implemented)
 * @return A struct containing the image as cv::Mat, centroiding coordinates as cv::Point, the fg errors, in case of no error FG_OK,
 * a boolean signaling if the centroiding was successful
 */

//TODO check if cv::Mat::clone is really necessary
CentroidingResult FrameGrabber::grabImageAndCentroid() {
    int error = grab_image_impl();
    if(error == DIV_BY_ZERO) {
    	  std::cout<<"error1"<<'\n';
        return {current_frame_.clone(), centroiding_coords_, {}, centroiding::mean_below_threshold, current_image_info_};
    }
    else if(error != FG_OK){
    	  std::cout<<"error2"<<'\n';
        return {current_frame_.clone(), centroiding_coords_, handle_fg_error_internal(), centroiding::framegrabber_error_occured, current_image_info_};
    }
    if(current_image_info_.mean_before_threshold > maximum_mean_) {
    	  std::cout<<"error3"<<'\n';
    	return {current_frame_, centroiding_coords_, {}, centroiding::mean_above_threshold, current_image_info_};
    }
    if(current_image_info_.mean_before_threshold < minimum_mean_){
    	 std::cout<<"error4"<<'\n';
    	return {current_frame_, centroiding_coords_, {}, centroiding::mean_below_threshold, current_image_info_};
    }
    if(current_image_info_.mean_after_treshold > maximum_mean_after_thresholding_){
    	 std::cout<<"error5"<<'\n';
    	return {current_frame_.clone(), centroiding_coords_, {}, centroiding::thresholded_mean_above_threshold, current_image_info_};
    }
    if(current_image_info_.max_grey_value > maximum_grey_value_){
    	 std::cout<<"error6"<<'\n';
    	return  {current_frame_, centroiding_coords_, {}, centroiding::max_grey_value_above_threshold, current_image_info_};
    }
    if(current_image_info_.signal_to_noise_ratio < minimum_snr_){
    	 std::cout<<"error7"<<'\n';
    	return {current_frame_, centroiding_coords_, {}, centroiding::snr_below_threshold, current_image_info_};
    }
    return {current_frame_, centroiding_coords_, {}, centroiding::no_errors, current_image_info_};
}

/**
 * @brief Grabs a full image | the recording has to be started first
 * @return An ImagingResult struct containing the image as cv::Mat and the error if any
 * in case of no error this evaluates to false
 */
ImagingResult FrameGrabber::grabImage() {
    if(grab_image_impl() != FG_OK) {

    	return {current_frame_, handle_fg_error_internal()};
    	}
    return {current_frame_, {}};
}

/**
 * @brief Set the rejection threshold for the signal to noise ratio. SNR is internally calculated via max_grey_value divided by mean
 * @details If the snr is too low, the centroiding might fail.
 * @param minimum_snr The minimal acceptable signal to noise ratio inside the region of interest
 */
void FrameGrabber::setMinimumSNR(float minimum_snr) {
    minimum_snr_ = minimum_snr;
}

/**
 * @brief Set the lower rejection threshold for the mean of the roi.
 * @details If the mean is too low, the image is either
 * underexposed or there is no guide star in the frame
 * @param minimum_mean The minimal acceptable mean before thresholding inside the region of interest
 */
void FrameGrabber::setMinimumMean(float minimum_mean) {
    minimum_mean_ = minimum_mean;
}

/**
 * @brief Set the upper rejection threshold for the mean of the roi.
 * @details If the mean is too high, the image is either
 * overexposed or there is more than one guide star in the frame
 * @param maximum_mean The maximal acceptable mean before thresholding inside the region of interest
 */
void FrameGrabber::setMaximumMean(float maximum_mean) {
    maximum_mean_ = maximum_mean;
}

/**
 * @brief Set the upper rejection threshold for the maximum grey value in the image after median filtering.
 * @details If the maxim grey value is greater than the threshold the image
 * is either overexposed or there are hotpixels, which
 * could not be removed
 * @param max_grey_value The maximum acceptable grey value in the image region of interest
 */
void FrameGrabber::setMaximumGreyValue(uint16_t max_grey_value) {
    maximum_grey_value_ = max_grey_value;
}

/**
 * @brief Set the rejection threshold for the mean of the image after thresholding.
 * @details If the mean is too high the
 * centroiding will fail.
 * @param mean_after_thresholding Maximum acceptable image mean after thresholding inside the region of interest
 */
void FrameGrabber::setMaximumMeanAfterThresholding(float mean_after_thresholding) {
    maximum_mean_after_thresholding_ = mean_after_thresholding;
}

/**
 * @brief This function is used to get a human readable output for the FrameGrabber errorcodes.
 * @details The errortable can be found on https://docs.baslerweb.com/live_docu/RT5/en/documents/SDK/SDK.html#8.4
 * @return A std::string containing the last FgError in human readable format.
 */
std::string FrameGrabber::fgLastErrorDescription() const {
    return fg_last_error_description_;
}
/**
 * @brief This function is used to get a human readable output for the PCO Serial error/warning codes.
 * @details The error/warning table can be found on https://www.pco.de/fileadmin/fileadmin/user_upload/pco-manuals/pco.sdk_manual.pdf#page=280
 * @return The last PCO-Serial Error/Warning in human readable format.
 */
std::string FrameGrabber::pcoLastErrorDescription() const {
    return pco_last_error_description_;
}

/**
 * @brief This functions starts a new thread to write the image to disk.
 * @details The file extensions is determined by the ending of the
 * filename. Note: FITS is unfortunately not supported by the opencv libs.
 * Recommended format is tif
 * @param file_path The full file path including extension recommended extension: .tif
 * @param image The image you want to save
 * @return A boolean signaling whether the image was written or not. This is determined by the number of threads supported by the hardware.
 * If more threads would be opened than the hardware can support the image is not written and false is returned.
 */
bool FrameGrabber::write_image_async(const std::string &file_path, const cv::Mat &image) {
    if(thread_count_ > std::thread::hardware_concurrency()) return false; //You can remove this line, but be warned, that if too many threads are started the ram will be used up quite fast
    std::thread([file_path, image, this](){
        thread_count_++;
        cv::imwrite(file_path, image);
        thread_count_--;}).detach();
    return true;
}

/**
 * @brief Queries information about the camera status
 * @return A CameraStatus struct containing the sensor temperature, the internal temperature of the camera inside the
 * housing, the temperature of an external device, e.g. the power supply. The struct also contains the error, warning and
 * health bits, which are currently set in the camera. For detailed information on the possible bits see:
 * https://www.pco.de/fileadmin/fileadmin/user_upload/pco-manuals/pco.sdk_manual.pdf#page=36
 */
CameraStatus FrameGrabber::getCameraStatus() {
    short ccd_temp;
    short cam_temp;
    short ext_temp;
    if((last_pco_error_  = pco_serial_com_.PCO_GetTemperature(&ccd_temp, &cam_temp, &ext_temp)) != PCO_NOERROR)
        return {{0, 0, 0}, {0, 0, 0}, handle_pco_error_internal()};
    unsigned warning_bits;
    unsigned error_bits;
    unsigned status_bits;
    if((last_pco_error_ =  pco_serial_com_.PCO_GetHealthStatus(&warning_bits, &error_bits, &status_bits) != PCO_NOERROR))
        return {{0, 0, 0}, {0, 0, 0}, handle_pco_error_internal()};
    return {{ccd_temp, cam_temp, ext_temp}, {warning_bits, error_bits, status_bits}, {}};
}

/**
 * @brief Get a handle to the internal frame grabber object to use with the SiliconSoftware SDK.
 * @details This function should only be called in rare circumstances.
 * @return The internal Fg_Struct handle.
 */
Fg_Struct *FrameGrabber::getFrameGrabberHandle() {
    return fg_;
}
/**
 * @brief Get a handle to the internal pco serial com handle to use with the PCO SDK.
 * @details This function should only be called in rare circumstances.
 * @return The internal serial com handle (CPco_com_cl_me4).
 */
CPco_com_cl_me4& FrameGrabber::getSerialComHandle() {
    return pco_serial_com_;
}

/**
 * @brief Initializes the frame grabber with the centroiding applet and allocates the frame buffer
 * @details This function is internal use init() to initialize the framegrabber and serial interface.
 * @return In case of errors the frame grabber error number (see https://docs.baslerweb.com/live_docu/RT5/en/documents/SDK/SDK.html#_8.4)
 * in case of no error the return will evaluate to false
 */
FgError_t FrameGrabber::init_fg_impl() {
    if((fg_ = Fg_InitEx(applet_name_.c_str(),0, 0)) == nullptr) return handle_fg_error_internal();
    //Maybe load config Fg_loadConfig(fg_, "CONFIG_PATH/fg_conf.mcf");
    if((ROI_X_ID = Fg_getParameterIdByName(fg_, "Device1_Process0_SetROI_x_offset_Value")) == 0) return handle_fg_error_internal();
    if((ROI_Y_ID = Fg_getParameterIdByName(fg_, "Device1_Process0_SetROI_y_offset_Value")) == 0) return handle_fg_error_internal();
    if((ROI_WIDTH_ID = Fg_getParameterIdByName(fg_,"Device1_Process0_SetROI_x_length_Value")) == 0) return handle_fg_error_internal();
    if((ROI_HEIGHT_ID = Fg_getParameterIdByName(fg_,"Device1_Process0_SetROI_y_length_Value")) == 0) return handle_fg_error_internal();
    if((LOWER_THRESHOLD_ID = Fg_getParameterIdByName(fg_, "Device1_Process0_Preprocessing_Thresholding_lower_thresh_Value")) == 0) return handle_fg_error_internal();
    if(Fg_setParameterWithType(fg_, ROI_X_ID, static_cast<int32_t>(centroiding_roi_.x), DMA_INDEX) != FG_OK) return handle_fg_error_internal();
    if(Fg_setParameterWithType(fg_, ROI_Y_ID, static_cast<int32_t>(centroiding_roi_.y), DMA_INDEX) != FG_OK) return handle_fg_error_internal();
    if(Fg_setParameterWithType(fg_, ROI_WIDTH_ID, static_cast<int32_t>(centroiding_roi_.width), DMA_INDEX) != FG_OK) return handle_fg_error_internal();
    if(Fg_setParameterWithType(fg_, ROI_HEIGHT_ID, static_cast<int32_t>(centroiding_roi_.height), DMA_INDEX) != FG_OK) return handle_fg_error_internal();
    frame_buffer_ = Fg_AllocMemEx(fg_, image_size_*buffer_cnt_*sizeof(uint16_t), buffer_cnt_);
    return {};
}
/**
 * @brief Opens the serial connection to the PCO Edge 4.2
 * @details Opens a serial connection to the camera, stops the recording, sets baudrate, CL clock frequency and format,
 * Sets the pixelrate to 273Mhz.
 * Make sure, that the camera is in rolling shutter mode else
 * an invalid acquire mode is returned.
 * This function is internal use init() to initialize the frame grabber and serial interface.
 * @return An PcoError_t this will evaluate to false in case of no errors
 */
PcoError_t FrameGrabber::init_serial_impl() {

    unsigned camera_setup[4];
    short unsigned setup_type;
    short unsigned length = 4;
    PCO_SC2_CL_TRANSFER_PARAM transfer_param{11520, 85000000, 0, PCO_CL_DATAFORMAT_5x16, 1};
    if((last_pco_error_ = pco_serial_com_.Open_Cam(0)) != PCO_NOERROR) return handle_pco_error_internal();
    if((last_pco_error_ = pco_serial_com_.PCO_SetRecordingState(0)) != PCO_NOERROR) return handle_pco_error_internal();
    if((last_pco_error_ = pco_serial_com_.PCO_GetCameraSetup(&setup_type, camera_setup, &length))) handle_pco_error_internal();
    if(camera_setup[0] != 1){
        last_pco_error_ = PCO_ERROR_FIRMWARE_ACQUIRE_MODE_INVALID;
        return handle_pco_error_internal();
    }
    if((last_pco_error_ = pco_serial_com_.PCO_SetTransferParameter(&transfer_param, sizeof(transfer_param))) != PCO_NOERROR) return handle_pco_error_internal();
    if((last_pco_error_ = pco_serial_com_.PCO_SetPixelRate(272250000))) return handle_pco_error_internal();
    if((last_pco_error_ = pco_serial_com_.PCO_SetLut(0, 0))) return handle_pco_error_internal();
    if((last_pco_error_ = pco_serial_com_.PCO_SetTriggerMode(TRIGGER_AUTO)) != PCO_NOERROR) return handle_pco_error_internal();
    if((last_pco_error_ = pco_serial_com_.PCO_ArmCamera()) != PCO_NOERROR) return handle_pco_error_internal();
    return {};
}

/**
 * @brief Writes the error description to the internal member.
 * @details This function is internal and should not be called manually.
 * Error codes: https://www.pco.de/fileadmin/fileadmin/user_upload/pco-manuals/pco.sdk_manual.pdf#page=280
 * PCO Error Codes are chained together via a logical OR operation. Warning format is 0xC0######, errorformat is
 * 0x80######. Error numbers are not unique each layer and the common errors errors have it's own error codes.
 * I wouldn't advise on implementing all possible error conditions. Maybe you could downlink the error description and
 * handle the errors manually. There could be some handling for warnings like sensor overheating etc.
 * @return the last PCO Serial error as an unsigned int
 */
PcoError_t FrameGrabber::handle_pco_error_internal() {
  PcoError_t error;
  char error_desc[256];
  PCO_GetErrorText(last_pco_error_, error_desc, sizeof(error_desc));
  pco_last_error_description_ = std::string(error_desc);
  error.hasError = true;
  error.pcoError = last_pco_error_;
  return error;
}

/**
* @brief Writes the error description to the internal member
* @details This function is internal and should not be called manually
* Error codes: https://docs.baslerweb.com/live_docu/RT4/en/documents/SDK/SDK.html#_8.4
* @return a struct containing the last pco_error as uint31_t and a bool signaling if there was an error
*/
FgError_t FrameGrabber::handle_fg_error_internal() {
    FgError_t error;
    const char* error_string = Fg_getLastErrorDescription(fg_);
    const int last_fg_error = Fg_getLastErrorNumber(fg_);
    fg_last_error_description_ = error_string;
    error.hasError = true;
    error.fgError = last_fg_error;
    std::cerr << "Error is"<<last_fg_error<<error_string<<"\n";
    return error;
}

/**
 * @brief Internal implementation of the grab image function
 * @details Extracts all data from the framebuffer
 */
int FrameGrabber::grab_image_impl() {
    last_frame_num_ = Fg_getLastPicNumberBlockingEx(fg_, 1, DMA_INDEX, 100, frame_buffer_);
    if(last_frame_num_ < 0) 
    {
    	return FG_ERROR;
    	}
    if(last_frame_num_ == 0)  {

    	return FG_ERROR;
    	}
    uint16_t* image_ptr;
    if((image_ptr = static_cast<uint16_t*>(Fg_getImagePtrEx(fg_, last_frame_num_, DMA_INDEX, frame_buffer_))) == nullptr){

        return FG_ERROR;
    }
    //Maybe we don't need to clone here if the buffer is large enough
    current_frame_ = cv::Mat(image_height_, image_width_, CV_16UC1, image_ptr).clone();
    internal_parse image_information = *reinterpret_cast<internal_parse*>(image_ptr + (image_width_*image_height_));
    centroiding_coords_ = {image_information.x/256.0f , image_information.y/256.0f};
    current_image_info_.mean_before_threshold = static_cast<float>(image_information.mean) / 256.0f;
    current_image_info_.mean_after_treshold = static_cast<float>(image_information.mean_after_threshold) / 256.0f;
    current_image_info_.max_grey_value = static_cast<float>(image_information.max_gv);
    current_image_info_.signal_to_noise_ratio = max_grey_value_ / current_image_info_.mean_before_threshold;
    if((image_information.x | image_information.y) & 0x10000)  {
    	return DIV_BY_ZERO;
    }
    return FG_OK;
}
