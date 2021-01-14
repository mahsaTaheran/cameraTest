#ifndef FRAMEGRABBER_H_
#define FRAMEGRABBER_H_


#include <atomic>
#include <Cpco_com_cl_me4.h>
#include <fgrab_define.h>
#include <fgrab_prototyp.h>
#include <fgrab_struct.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <thread>

#include "FrameGrabberHelper.hpp"


/**
 * @brief This class represents the FrameGrabber interface which controls the internal framegrabber interface as well
 * as the serial communication to the PCO_Camera.
 */
class FrameGrabber final {
    //applet name
    const std::string applet_name_ = "/opt/siso/dll/mE5-MA-VCL/STUDIO_grab.hap";
    
    //serial
    CPco_com_cl_me4 pco_serial_com_;
    uint32_t last_pco_error_ = 0;
    std::string pco_last_error_description_;
    //framegrabber
    Fg_Struct* fg_ = nullptr;
    dma_mem* frame_buffer_ = nullptr;
    static constexpr unsigned image_width_ = 2048;
    static constexpr unsigned image_height_ = 2048;
    static constexpr unsigned buffer_cnt_ = 20;
    static constexpr unsigned image_size_ = image_width_ * (image_height_ + 1); // +1 for centroiding data
    int ROI_X_ID{0};
    int ROI_Y_ID{0};
    int ROI_WIDTH_ID{0};
    int ROI_HEIGHT_ID{0};
    int LOWER_THRESHOLD_ID{0};
    long long last_frame_num_{0};
    //common
    bool recording = false;
    std::string fg_last_error_description_;
    //centroiding
    cv::Mat current_frame_;
    ImageInformation current_image_info_;
    cv::Point2f centroiding_coords_;
    cv::Rect2i centroiding_roi_{cv::Point{0, 0}, cv::Size(100,100)};
    float maximum_mean_ = 800; //more than
    float minimum_mean_ = 100; //less than
    float minimum_snr_ = 5; //less than
    float maximum_mean_after_thresholding_ = 1; //more than //TODO check for good default
    uint16_t max_grey_value_ = 0;
    uint16_t maximum_grey_value_ = 50000; //more than
    //image save
    std::atomic_uint32_t thread_count_{0};
public:
    FrameGrabber();
    FrameGrabber(const FrameGrabber&) = delete;

    //centroiding interface
    PcoFgError_t init();
    FgError_t setCentroidingRoi(const cv::Rect2i &region_of_interest);
    cv::Rect2i getRoi();
    PcoError_t setDelayAndExposure(uint32_t delay_in_mu, uint32_t exposure_in_mu);
    PcoError_t setImageSize(uint16_t ROI_width, uint16_t ROI_height);
    FgError_t setStarThresholding(float percentage_of_maximum);
    PcoFgError_t startRecording();
    PcoFgError_t stopRecording();
    CentroidingResult grabImageAndCentroid();
    ImagingResult grabImage();

    void setMinimumSNR(float minimum_snr);
    void setMinimumMean(float minimum_mean);
    void setMaximumMean(float maximum_mean);
    void setMaximumMeanAfterThresholding(float mean_after_thresholding);
    void setMaximumGreyValue(uint16_t max_grey_value);
    //error interface
    std::string fgLastErrorDescription() const;
    std::string pcoLastErrorDescription() const;
    //image save interface
    bool write_image_async(const std::string &file_path, const cv::Mat &image);
    //camera health interface
    CameraStatus getCameraStatus();

    Fg_Struct* getFrameGrabberHandle();
    CPco_com_cl_me4& getSerialComHandle();

    ~FrameGrabber();
protected:
    PcoError_t init_serial_impl();
    FgError_t init_fg_impl();
    PcoError_t handle_pco_error_internal();
    FgError_t handle_fg_error_internal();
    int grab_image_impl();
};


#endif /* FRAMEGRABBER_H_ */
