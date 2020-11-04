//
// Created by joris on 2020-09-29.
//
#include <opencv2/core.hpp>
#include <PCO_err.h>
#include <fgrab_define.h>
/**
 * @file This file contains additional structs and enums which are use by the FrameGrabber implementation
 */

/**
 * @brief The grabImageAndCentroid function checks against the settable error conditions. And returns an enum with the CentroidingData
 * mean_above_threshold: The mean of the roi is above the set threshold (default 800). Either the image is overexposed or there is more than one guide star in the frame
 * mean_below_threshold: The mean of the roi is below the set threshold (default 100). Either the image is underexposed or there is no guide star in the frame
 * thresholded_mean_above_threshold: For centroiding all
 */
enum class centroiding{
    no_errors = 0, ///< centroiding should be successful
    mean_above_threshold, ///< image might be overexposed
    mean_below_threshold, ///< image might be underexposed
    thresholded_mean_above_threshold, ///< image contains too many stars or threshdolding parameters are not fitting
    max_grey_value_above_threshold, ///< image is overexposed or image contains hot pixel, which could not be removed
    snr_below_threshold, ///< guide star is too dim / image is underexposed
    framegrabber_error_occured ///< the frame grabber returned an error
};

/**
 * @brief This struct contains all Information which is calculated during the centroiding process
 */
struct ImageInformation{
    uint16_t max_grey_value; ///< The maximum grey value inside the region of interest
    float signal_to_noise_ratio; ///< The SNR (max_greyvalue/mean_before_threshold)
    float mean_before_threshold; ///< The mean of the region of interest before the thresholding operation
    float mean_after_treshold; ///< The mean of the region of interest after the thresholding operation
};

/**
 * @brief This struct represents an error either by the frame grabber or the serial interface
 * @details For frame grabber error codes see: https://docs.baslerweb.com/live_docu/RT5/en/documents/SDK/SDK.html#_8.4
 * For pco error codes see: https://www.pco.de/fileadmin/fileadmin/user_upload/pco-manuals/pco.sdk_manual.pdf#page=280
 *
 * The operator bool is overloaded usage is as follows:
 * @code
 * if(PcoFgError_t error = function_that_returns_PcoFgError_t()) {
 *     if(error.fgError == FG_ERROR){
 *     //handle error
 *     }
 *     if(error.pcoError & PCO_ERROR){
 *     //handle error
 *     }
 * }
 * @endcode
 */

struct PcoFgError_t{
    PcoFgError_t() = default;
    PcoFgError_t(uint32_t pco_error, int fg_error) : fgError(fg_error), pcoError(pco_error), hasError(true){}
    bool hasError = false; ///< Indicator if an error occurred
    uint32_t pcoError = PCO_NOERROR; ///< The pco error that occurred in case of no error PCO_NOERROR
    int fgError = FG_OK; ///< The fg error that occurred in case of no error FG_OK
    operator bool(){
        return hasError;
    }
};

/**
 * @brief This struct represents an error in the serial interface.
 * @details For error codes see: https://www.pco.de/fileadmin/fileadmin/user_upload/pco-manuals/pco.sdk_manual.pdf#page=280
 * please have also a look into the PCO_err.h
 *
 * PCO Error Codes are chained together via a logical OR operation. Warning format is 0xC0######, errorformat is
 * 0x80######.
 * Error numbers are not unique each layer and the common errors errors have it's own error codes.
 * I wouldn't advise on implementing all possible error conditions. Maybe you could downlink the error description and
 * handle the errors manually. There could be some handling for warnings like sensor overheating etc.
 *
 * You can find out if an error occurred, by masking the error with the error code e.g if(error & PCO_ERROR_DRIVER) -> Driver error occurred
 *
 * Usage:
 * @code
 * if(PcoError_t error = function_that_returns_PcoError_t()){
 *     if(error.pcoError & PCO_ERROR){
 *         //handle error
 *         }
 * }
 * @endcode
 */
struct PcoError_t{
    PcoError_t() = default;
    PcoError_t(uint32_t pco_error) : pcoError(pco_error), hasError(true) {}
    bool hasError = false; ///< Indicator if an error occurred
    uint32_t pcoError = PCO_NOERROR; ///< The pco error that occurred in case of no error PCO_NOERROR
    /**
     * @brief This overload can be used to simplify the error checking
     * @return true if an error occurred
     */
    operator bool(){
        return hasError;
    }
    /**
     * @brief This overload can be used to compare an frame grabber error directly to an DEFINE
     * @details Example:
     *
     * @code
     * if(PcoError_t error = grabber.setExposureAndDelay(10, 1000)){
     *     if(error & PCO_ERROR){
     *         //handle error
     *     }
     * }
     * @endcode
     */
    operator uint32_t(){
        return pcoError;
    }
};

/**
 * @brief This struct represents an error in the frame grabber interface.
 * For error codes see: https://docs.baslerweb.com/live_docu/RT5/en/documents/SDK/SDK.html#_8.4
 *
 * Usage:
 * @code
 * if(FgError_t error = function_that_returns_FgError_t()){
 *     if(error.fgError == FG_ERROR){
 *         //handle error
 *         }
 * }
 * @endcode
 */
struct FgError_t{
    FgError_t() = default;
    FgError_t(int fg_error) : fgError(fg_error) {};
    bool hasError = false; ///< Indicator if an error occurred
    int fgError = FG_OK; ///< The fg error that occurred in case of no error FG_OK
    /**
     * @brief This overload can be used to write a shorter error check
     * @return True if an frame grabber error occurred
     */
    operator bool(){
        return hasError;
    }
    /**
     * @brief This overload can be used to compare an frame grabber error directly to an DEFINE
     * @details Example:
     *
     * @code
     * if(FgError_t error = grabber.setCentroidingRoi(roi)){
     *     if(error == FG_VALUE_OUT_OF_RANGE){
     *         //handle error
     *     }
     * }
     * @endcode
     * @return
     */
    operator int(){
        return fgError;
    }
};

/**
 * @brief This struct contains information about the camera status
 * This includes sensor temperature, temperature inside the housing, and temperature of an external device (e. g. power supply)
 * The health status contains error, warning and status bits. For information on the bits see
 * https://www.pco.de/fileadmin/fileadmin/user_upload/pco-manuals/pco.sdk_manual.pdf#page=36
 */
struct CameraStatus{
    /**
     * @brief This struct contains the current temperatures
     */
    struct {
        short sensor; ///< sensor temperature in °C
        short camera; ///< temperature inside the housing in °C
        short external; ///< temperature of external device in °C
    } temperature;
    /**
     * @brief This struct contains the currently set error, warning and status bits
     */
    struct {
        unsigned error_bits; ///< https://www.pco.de/fileadmin/fileadmin/user_upload/pco-manuals/pco.sdk_manual.pdf#page=36
        unsigned warning_bits; ///< https://www.pco.de/fileadmin/fileadmin/user_upload/pco-manuals/pco.sdk_manual.pdf#page=36
        unsigned status_bits; ///< https://www.pco.de/fileadmin/fileadmin/user_upload/pco-manuals/pco.sdk_manual.pdf#page=36
    } health;
    PcoError_t pcoError; ///< Indication of possible error while polling the serial information
};

/**
 * @brief This struct contains the results of the centroiding operation
 */
struct CentroidingResult {
    cv::Mat image; ///< The full 2048x2048 - 16bit image
    cv::Point2f centroiding_coordinates; ///< The centroiding coordinates accurate to a quarter pixel
    FgError_t fg_error; ///< Indication of possible error while using the frame grabber interface
    centroiding centroiding_enum; ///< An indicator to show any centroiding errors
    ImageInformation image_info_roi; ///< A struct containing information on the data inside the region of interest
};

/**
 * @brief This struct contains the result of an image grab without centroids
 */
struct ImagingResult{
    cv::Mat image; ///< The full 2048x2048 - 16bit image
    FgError_t fg_error; ///< The error of the frame grabber interface if any occurred
};

#ifndef FRAMEGRABBER_HANDLER_FRAMEGRABBERHELPER_H
#define FRAMEGRABBER_HANDLER_FRAMEGRABBERHELPER_H

#endif //FRAMEGRABBER_HANDLER_FRAMEGRABBERHELPER_H
