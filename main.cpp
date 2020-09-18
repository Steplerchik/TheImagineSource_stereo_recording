//////////////////////////////////////////////////////////////////
/*
Tcam Software Trigger
This sample shows, how to trigger two cameras by software and use a callback for image handling.

Prerequisits
It uses the the examples/cpp/common/tcamcamera.cpp and .h files of the *tiscamera* repository as wrapper around the
GStreamer code and property handling. Adapt the CMakeList.txt accordingly.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "tcamcamera.h"
#include <unistd.h>

#include "opencv2/opencv.hpp"

using namespace gsttcam;

// Create a custom data structure to be passed to the callback function. 
typedef struct
{
    int ImageCounter;
    bool ReceivedAnImage;
    bool busy;
    char imageprefix[55]; // Prefix for the image file names.
   	cv::Mat frame; 
} CUSTOMDATA;


////////////////////////////////////////////////////////////////////
// Simple implementation of "getch()"

void waitforkey(char *text)
{
    printf("%s\n",text);
    char dummyvalue[10];
    scanf("%c",dummyvalue);
}
////////////////////////////////////////////////////////////////////
// List available properties helper function.
void ListProperties(TcamCamera &cam)
{
    // Get a list of all supported properties and print it out
    auto properties = cam.get_camera_property_list();
    std::cout << "Properties:" << std::endl;
    for(auto &prop : properties)
    {
        std::cout << prop->to_string() << std::endl;
    }
}

////////////////////////////////////////////////////////////////////
// Callback called for new images by the internal appsink
GstFlowReturn new_frame_cb(GstAppSink *appsink, gpointer data)
{
    int width, height ;
    const GstStructure *str;

    // Cast gpointer to CUSTOMDATA*
    CUSTOMDATA *pCustomData = (CUSTOMDATA*)data;

    if( pCustomData->busy) // Return, if will are busy. Will result in frame drops
       return GST_FLOW_OK;

    pCustomData->busy = true;

    // The following lines demonstrate, how to acces the image
    // data in the GstSample.
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstCaps *caps = gst_sample_get_caps(sample);

    str = gst_caps_get_structure (caps, 0);    
    
    if( strcmp( gst_structure_get_string (str, "format"),"BGR") == 0)  
    {
        gst_structure_get_int (str, "width", &width);
        gst_structure_get_int (str, "height", &height);

        // Get the image data
        GstMapInfo info;
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        gst_buffer_map(buffer, &info, GST_MAP_READ);
        if (info.data != NULL) 
        {
            // Create the cv::Mat
            pCustomData->frame.create(height,width,CV_8UC(3));
            // Copy the image data from GstBuffer intot the cv::Mat
            memcpy( pCustomData->frame.data, info.data, width*height*3);
            // Set the flag for received and handled an image.
            pCustomData->ReceivedAnImage = true;
        }
        gst_buffer_unmap (buffer, &info);
    }    
    // Calling Unref is important!
    gst_sample_unref(sample);

    // Set our flag of new image to true, so our main thread knows about a new image.
    pCustomData->busy = false;
    return GST_FLOW_OK;
}

////////////////////////////////////////////////////////////
// Enable trigger mode for the passed camera object
//
int setTriggerMode(TcamCamera &cam, int onoff)
{
    // Query the trigger mode property. Attention: The "Trigger Mode" name
    // can differ on USB and GigE camera. If you do not know the names of these
    // properties, uncomment the above mentions "ListProperties(cam1)" line in order
    // to get a list of available properties.
    std::shared_ptr<Property> TriggerMode;
    try
    {
        TriggerMode = cam.get_property("Trigger Mode");
        // TriggerMode->set(cam,"Off"); // Use this line for GigE cameras
        TriggerMode->set(cam,onoff); // Use this line for USB cameras
        return 1;
    }
    catch(...)
    {
        printf("Your camera does not support triggering.\n");
        return(0);
    }
}

////////////////////////////////////////////////////////////////////
// Increase the frame count and save the image in CUSTOMDATA
void SaveImage(CUSTOMDATA *pCustomData)
{
    char ImageFileName[256];
    pCustomData->ImageCounter++;

    sprintf(ImageFileName,"%s%05d.jpg", pCustomData->imageprefix,  pCustomData->ImageCounter);
    cv::imwrite(ImageFileName,pCustomData->frame);

}
///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    gst_init(&argc, &argv);
    // Create and initialize the custom data for the callback.
    CUSTOMDATA CustomData1;  // Customdata for camera 1
    CUSTOMDATA CustomData2;  // Customdata for camera 2

    CustomData1.ImageCounter = 0;
    CustomData1.ReceivedAnImage = false;
    CustomData1.busy = true; // Avoid saving of images when the stream is started
    strcpy(CustomData1.imageprefix,"left-"); // Specify the image prefix

    CustomData2.ImageCounter = 0;
    CustomData2.ReceivedAnImage = false;
    CustomData2.busy = true; // Avoid saving of images when the stream is started
    strcpy(CustomData2.imageprefix,"right-"); // Specify the image prefix

    // Open cameras by serial number. Use "tcam-ctrl -l" to query a list of cameras
    TcamCamera cam1("06710488");    
    TcamCamera cam2("36710103");    
    
    // Set video format, resolution and frame rate
    cam1.set_capture_format("BGR", FrameSize{1920, 1080}, FrameRate{30,1});
    cam2.set_capture_format("BGR", FrameSize{1920, 1080}, FrameRate{30,1});

    //Uncomment following line, if properties shall be listed
    //ListProperties(cam1);
    

    std::shared_ptr<Property> TriggerMode;
    try
    {
        TriggerMode = cam1.get_property("Trigger Mode");
    }
    catch(...)
    {
        printf("Your camera does not support triggering.\n");
        return(1);
    }
      // Query the software trigger property. Attention: The "Software Triggere" name
    // can be different on USB and GigE camera. 
    std::shared_ptr<Property> Softwaretrigger;
    try
    {
        Softwaretrigger = cam1.get_property("Software Trigger");
    }
    catch(...)
    {
        printf("Your camera does not support software triggering.\n");
        return(2);
    }

    std::shared_ptr<Property> ReverseImageX;
    try
    {
        ReverseImageX = cam1.get_property("Reverse X");
    }
    catch(...)
    {
        printf("Your camera does not support reversing image X.\n");
        return(3);
    }

    std::shared_ptr<Property> ReverseImageY;
    try
    {
        ReverseImageY = cam1.get_property("Reverse Y");
    }
    catch(...)
    {
        printf("Your camera does not support reversing image Y.\n");
        return(4);
    }

    std::shared_ptr<Property> GainAuto;
    try
    {
        GainAuto = cam1.get_property("Gain Auto");
    }
    catch(...)
    {
        printf("Your camera does not support gain auto.\n");
        return(5);
    }

    std::shared_ptr<Property> Gain;
    try
    {
        Gain = cam1.get_property("Gain");
    }
    catch(...)
    {
        printf("Your camera does not support gain.\n");
        return(6);
    }

    std::shared_ptr<Property> ExposureAuto;
    try
    {
        ExposureAuto = cam1.get_property("Exposure Auto");
    }
    catch(...)
    {
        printf("Your camera does not support exposure auto.\n");
        return(7);
    }

    std::shared_ptr<Property> Exposure;
    try
    {
        Exposure = cam1.get_property("Exposure");
    }
    catch(...)
    {
        printf("Your camera does not support exposure.\n");
        return(8);
    }


    // Comment following line, if no live video display is wanted.
    cam1.enable_video_display(gst_element_factory_make("ximagesink", NULL));
    cam2.enable_video_display(gst_element_factory_make("ximagesink", NULL));

    // Register a callback to be called for each new frame
    cam1.set_new_frame_callback(new_frame_cb, &CustomData1);
    cam2.set_new_frame_callback(new_frame_cb, &CustomData2);

    // Disable trigger mode fist
    // TriggerMode->set(cam,"Off"); // Use this line for GigE cameras
    TriggerMode->set(cam1,0); // Use this line for USB cameras
    TriggerMode->set(cam2,0); // Use this line for USB cameras
    
    // Reversing camera images
    ReverseImageX->set(cam1,1);
    ReverseImageX->set(cam2,1);
    ReverseImageY->set(cam1,1);
    ReverseImageY->set(cam2,1);
    
    // Exposure setup
    ExposureAuto->set(cam1,0);
    Exposure->set(cam1,7000);
    ExposureAuto->set(cam2,0);
    Exposure->set(cam2,7000);

    // Gain Setup
    GainAuto->set(cam1,1);
    Gain->set(cam1,0);
    GainAuto->set(cam2,1);
    Gain->set(cam2,0);

    // Start the camera
    cam1.start();
    cam2.start();


    usleep(5000000);
    waitforkey("Press Enter for start of triggering and image saving.");

    // Enable trigger mode
    TriggerMode->set(cam1,1); // Use this line for USB cameras
    TriggerMode->set(cam2,1); // Use this line for USB cameras

    CustomData1.busy = false; // allow saving of images when the stream is started
    CustomData1.ImageCounter = 0; // Reset the image counter
    CustomData2.busy = false; // allow saving of images when the stream is started
    CustomData2.ImageCounter = 0; // Reset the image counter
    
    printf("Starting triggers now\n");

    // Main loop for software trigger. Image saving is done in the callback.
    // for( int i = 0; i < 50; i++)
    while(true)
    {
        // printf("%d.Trigger \n",i);
	printf("Trigger \n");
	
        CustomData1.ReceivedAnImage = false;
        CustomData2.ReceivedAnImage = false;

        Softwaretrigger->set(cam1,1);
        Softwaretrigger->set(cam2,1);

        // Wait with timeout until we got images from both cameras.
        int tries = 50;
        while( !( CustomData1.ReceivedAnImage || CustomData2.ReceivedAnImage) && tries >= 0)
        {
            usleep(100000);  // 10ms
            tries--;
        }

        // If there are images received from both cameras, save them.
        if(CustomData1.ReceivedAnImage && CustomData1.ReceivedAnImage)
        {
            SaveImage(&CustomData1);
            SaveImage(&CustomData2);
	    // printf("May be saved");
        }
        else
        {
            // Check, from which camera we may did not receive an image. 
            // It is for convinience only and could be deleted.
            if(!CustomData1.ReceivedAnImage)
                printf("Did not receive an image from camera 1.\n");
                
            if(!CustomData2.ReceivedAnImage)
                printf("Did not receive an image from camera 2.\n");
        }
    }
    
    // waitforkey("Press Enter to end the program");

    CustomData1.busy = true; // deny saving of images 
    CustomData2.busy = true; // deny saving of images 

    // Disable the trigger mode, so other programs will see a live video.
    TriggerMode->set(cam1,0); // Use this line for USB cameras
    TriggerMode->set(cam2,0); // Use this line for USB cameras

    Softwaretrigger->set(cam1,1); // Needed for cam1.stop() not waiting for an image.
    Softwaretrigger->set(cam2,1); // Needed for cam2.stop() not waiting for an image.

    cam1.stop();
    cam2.stop();
    return 0;
}
