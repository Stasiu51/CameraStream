/*****************************************************************************
 * EVT_AcquisitionControl.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_AcquisitionControl
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_AcquisitionControl example opens the device, sets exposure, tests  
 *   software multiframe trigger, tests multiframe software trigger read of 
 *   buffer, then closes the device and ends. 
 *   Thus it exercises functionality of the XML AcquisitionControl group.
 *   Other elements exercised elsewhere.
 ****************************************************************************/


// EVT_AquisitionControl.exe FRAMERATE WIDTH HEIGHT EXPOSURE GAIN FORMAT 

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <EmergentCameraAPIs.h>
#include <emergentframe.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>
#include <EmergentFrameSave.h>
#include <conio.h>
#include <chrono>

using namespace std;
using namespace Emergent;
using namespace std::chrono;

#define SUCCESS 0
//#define XML_FILE   "C:\\xml\\Emergent_HS-2000-M_1_0.xml"
#define FRAME_QUEUE 250
#define FRAMES_IN_MEMORY 5600

#define MAX_CAMERAS 10

#define EXPOSURE_VAL 1000 //us
#define WIDTH_VAL_DEF 5120 //INC is 16 typ
#define HEIGHT_VAL_DEF 4096
#define GAIN_DEF 2000

void configure_defaults(CEmergentCamera* camera);
char* next_token;

void safe(EVT_ERROR ReturnVal, CEmergentCamera* camera) {
    if (ReturnVal != EVT_SUCCESS) {
        printf("Error: %d\n", ReturnVal);
        EVT_CameraClose(camera);
        printf("Camera Closed!");
        exit(ReturnVal);
    }
}

void safe(EVT_ERROR ReturnVal, CEmergentCamera* camera, char* msg) {
    if (ReturnVal != EVT_SUCCESS) {
        printf(msg);
        printf(" Error: %d\n", ReturnVal);
        EVT_CameraClose(camera);
        printf("Camera Closed!");
        exit(ReturnVal);
    }
}

int main(int argc, char* argv[])
{

  CEmergentCamera camera;
  int ReturnVal = SUCCESS;
  unsigned int param_val_max, param_val_min, param_val_inc, height, width, framerate, gain, exposure;
  CEmergentFrame evtFrame[FRAMES_IN_MEMORY];
  char filename[100];
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int count, camera_index;
  EVT_ERROR err = EVT_SUCCESS;

  printf("-------------------------------------"); printf("\n");
  printf("AcquisitionControl : Custom program "); printf("\n");
  printf("-------------------------------------"); printf("\n");

  //Get Camera
  {
      //Find all cameras in system.
      unsigned int listcam_buf_size = MAX_CAMERAS;
      EVT_ListDevices(deviceInfo, &listcam_buf_size, &count);
      if (count == 0)
      {
          printf("Enumerate Cameras: \tNo cameras found. Exiting program.\n");
          return 0;
      }

      //Find and use first EVT camera.
      for (camera_index = 0; camera_index < MAX_CAMERAS; camera_index++)
      {
          char* EVT_models[] = { "HS", "HT", "HR", "HB", "LR", "LB", "HZ" };
          int EVT_models_count = sizeof(EVT_models) / sizeof(EVT_models[0]);
          bool is_EVT_camera = false;
          for (int i = 0; i < EVT_models_count; i++)
          {
              if (strncmp(deviceInfo[camera_index].modelName, EVT_models[i], 2) == 0)
              {
                  is_EVT_camera = true;
                  break; //it is an EVT camera
              }
          }

          if (is_EVT_camera)
          {
              break; //Found EVT camera so break. i carries index for open.
          }

          if (camera_index == (MAX_CAMERAS - 1))
          {
              printf("No EVT cameras found. Exiting program\n");
              return 0;
          }
      }

      //Open the camera. Example usage. Camera found needs to match XML.
#ifdef XML_FILE
      ReturnVal = EVT_CameraOpen(&camera, &deviceInfo[camera_index], XML_FILE);
#else
      ReturnVal = EVT_CameraOpen(&camera, &deviceInfo[camera_index]);
#endif
      if (ReturnVal != SUCCESS)
      {
          printf("Open Camera: \t\tError. Exiting program.\n");
          return ReturnVal;
      }
      else
      {
          printf("Open Camera: \t\tCamera Opened\n\n");
      }
  }
  try {

      //To avoid conflict with settings in other examples.
      configure_defaults(&camera);
      // Get CMD Settings
      {
          safe(EVT_CameraSetUInt32Param(&camera, "Width", WIDTH_VAL_DEF), &camera);
          if (argc >= 3)
          {
              //Set ROI width
              int input_width = atoi(argv[2]);
              unsigned int width_max, width_min, width_inc;
              safe(EVT_CameraGetUInt32ParamMax(&camera, "Width", &width_max), &camera);
              safe(EVT_CameraGetUInt32ParamMin(&camera, "Width", &width_min), &camera);
              safe(EVT_CameraGetUInt32ParamInc(&camera, "Width", &width_inc), &camera);

              if (input_width >= (int)width_min && input_width <= (int)width_max && (input_width % width_inc == 0))
              {
                  safe(EVT_CameraSetUInt32Param(&camera, "Width", input_width), &camera);
              }
              else {
                  printf("Invalid width! Min/Max/Inc %d/%d/%d\n", width_min, width_max, width_inc);
              }
          }
          else {
              printf("No width specified, using default.\n");
          }

          //Specifies ROI height
          //Could be overwritten below.
          safe(EVT_CameraSetUInt32Param(&camera, "Height", HEIGHT_VAL_DEF), &camera);
          if (argc >= 4)
          {
              //Set ROI height
              int input_height = atoi(argv[3]);
              unsigned int height_max, height_min, height_inc;
              safe(EVT_CameraGetUInt32ParamMax(&camera, "Height", &height_max), &camera);
              safe(EVT_CameraGetUInt32ParamMin(&camera, "Height", &height_min), &camera);
              safe(EVT_CameraGetUInt32ParamInc(&camera, "Height", &height_inc), &camera);

              if (input_height >= (int)height_min && input_height <= (int)height_max && (input_height % height_inc == 0))
              {
                  safe(EVT_CameraSetUInt32Param(&camera, "Height", input_height), &camera);
              }
              else {
                  printf("Invalid height! Min/Max/Inc %d/%d/%d\n", height_min, height_max, height_inc);
              }
          }
          else {
              printf("No height specified, using default.\n");
          }

          //Specifies pixel format
          if (argc >= 7)
          {
              ReturnVal = EVT_CameraSetEnumParam(&camera, "PixelFormat", argv[4]);
              if (ReturnVal != SUCCESS)
              {
                  printf("EVT_CameraSetEnumParam: PixelFormat Error %d\n", ReturnVal);
                  EVT_CameraClose(&camera);
                  printf("\nClose Camera: \t\tCamera Closed\n");
                  return ReturnVal;
              }
          }
          else {
              printf("No pixel format specified, using default.\n");
          }

          //Specifies frame rate
          //Could be overwritten below.
          unsigned int frame_rate_max, frame_rate_min;
          safe(EVT_CameraGetUInt32ParamMax(&camera, "FrameRate", &frame_rate_max), &camera);
          safe(EVT_CameraGetUInt32ParamMin(&camera, "FrameRate", &frame_rate_min), &camera);
          printf("Framreate Min/Max: \t\t%d/%d\n", frame_rate_min, frame_rate_max);
          if (argc >= 2)
          {
              //Set ROI width
              unsigned int input_rate = atoi(argv[1]);



              if (input_rate >= (int)frame_rate_min && input_rate <= (int)frame_rate_max)
              {
                  safe(EVT_CameraSetUInt32Param(&camera, "FrameRate", input_rate), &camera);
              }

              if (input_rate > (int)frame_rate_max)
              {
                  safe(EVT_CameraSetUInt32Param(&camera, "FrameRate", frame_rate_max), &camera);
                  printf("FrameRate Set to Max: \t\t%d\n", frame_rate_max);
              }
          }
          else {
              printf("No framerate specified, setting max: %d.\n", frame_rate_max);
              safe(EVT_CameraSetUInt32Param(&camera, "FrameRate", frame_rate_max), &camera);
          }

          //Specifies exposure
          //Could be overwritten below.
          unsigned int exposure_max, exposure_min, exposure_inc;
          safe(EVT_CameraGetUInt32ParamMax(&camera, "Exposure", &exposure_max), &camera);
          safe(EVT_CameraGetUInt32ParamMin(&camera, "Exposure", &exposure_min), &camera);
          safe(EVT_CameraGetUInt32ParamInc(&camera, "Exposure", &exposure_inc), &camera);
          printf("Exposure Min/Max/Inc: \t\t%d/%d/%d\n", exposure_min, exposure_max, exposure_inc);
          if (argc >= 5)
          {
              //Set ROI width
              unsigned int input_ex = atoi(argv[4]);

              if (input_ex >= (int)exposure_min && input_ex <= (int)exposure_max && input_ex % exposure_inc == 0)
              {
                  safe(EVT_CameraSetUInt32Param(&camera, "Exposure", input_ex), &camera);
              }
              else
              {
                  safe(EVT_CameraSetUInt32Param(&camera, "Exposure", exposure_max), &camera);
                  printf("Invalid exposure setting! \nExposure Set to Max: \t\t%d\n", exposure_max);
              }
          }
          else {
              printf("No exposure specified, setting max: %d.\n", exposure_max);
              safe(EVT_CameraSetUInt32Param(&camera, "Exposure", exposure_max), &camera);
          }

          //Set Gain
          unsigned int gain_max, gain_min;
          safe(EVT_CameraGetUInt32ParamMax(&camera, "Gain", &gain_max), &camera);
          safe(EVT_CameraGetUInt32ParamMin(&camera, "Gain", &gain_min), &camera);
          printf("Gain Min/Max: \t\t%d/%d\n", gain_min, gain_max);
          if (argc >= 6)
          {
              //Set ROI width
              unsigned int input_gain = atoi(argv[5]);

              if (input_gain >= (int)gain_min && input_gain <= (int)gain_max)
              {
                  safe(EVT_CameraSetUInt32Param(&camera, "Gain", input_gain), &camera);
              }
              else
              {
                  printf("Invalid gain setting, setting default: %d.\n", GAIN_DEF);
                  safe(EVT_CameraSetUInt32Param(&camera, "Gain", GAIN_DEF), &camera);
              }
          }
          else {
              printf("No gain specified, setting default: %d.\n", GAIN_DEF);
              safe(EVT_CameraSetUInt32Param(&camera, "Gain", GAIN_DEF), &camera);
          }
      }

      // Show Settings
      printf("\n---- Final Settings ----\n");
      {
          printf("No Buffers: \t\t%d\n", FRAMES_IN_MEMORY);
          //Get format.
          {
              const int buffersize = 100;
              unsigned long valuesize = 0;
              char format[buffersize];
              safe(EVT_CameraGetEnumParam(&camera, "PixelFormat", format, buffersize, &valuesize), &camera);
              printf("PixelFormat: \t\t%s\n", format);
          }
          //Get framerate
          safe(EVT_CameraGetUInt32Param(&camera, "FrameRate", &framerate), &camera);
          printf("Frame Rate: \t\t%d\n", framerate);

          //Get exposure
          safe(EVT_CameraGetUInt32Param(&camera, "Exposure", &exposure), &camera);
          printf("Exposure: \t\t%d\n", exposure);

          //Get gain
          safe(EVT_CameraGetUInt32Param(&camera, "Gain", &gain), &camera);
          printf("Gain:      \t\t%d\n", gain);

          //Get resolution.
          safe(EVT_CameraGetUInt32Param(&camera, "Height", &height), &camera);
          safe(EVT_CameraGetUInt32Param(&camera, "Width", &width), &camera);
          printf("Resolution: \t\t%d x %d\n", width, height);
      }


      //Setup multiframe software trigger for MAX_FRAMES frame. Error check omitted for clarity.
      //printf("Trigger Mode: \t\tMulti Frame, SoftwareTrigger, %d Frames\n", FRAMES_TO_RECORD);
      //safe(EVT_CameraSetEnumParam(&camera, "AcquisitionMode", "MultiFrame"), &camera);
      //printf("1");
      //safe(EVT_CameraSetUInt32Param(&camera, "AcquisitionFrameCount", FRAMES_TO_RECORD), &camera);
      //printf("2");
      //safe(EVT_CameraSetEnumParam(&camera, "TriggerSelector", "FrameStart"), &camera); printf("3");
      //safe(EVT_CameraSetEnumParam(&camera, "TriggerMode", "On"), &camera); printf("4");
      //safe(EVT_CameraSetEnumParam(&camera, "TriggerSource", "Software"), &camera); printf("5");
      // 
      //safe(EVT_CameraSetEnumParam(&camera,   "BufferMode",             "Off"), &camera); printf("6");
      //safe(EVT_CameraSetUInt32Param(&camera, "BufferNum",              0), &camera); printf("7");

        //Prepare host side for streaming.
      printf("Opening stream...\n");
      safe(EVT_CameraOpenStream(&camera), &camera);



      //Allocate buffers and queue up frames before entering grab loop.
      printf("Allocate buffers...\n");
      for (int frame_count = 0; frame_count < FRAMES_IN_MEMORY; frame_count++)
      {
          //Three params used for memory allocation. Worst case covers all models so no recompilation required.
   
          evtFrame[frame_count].size_x = width;
          evtFrame[frame_count].size_y = height;
          evtFrame[frame_count].pixel_type = GVSP_PIX_MONO8; //Covers color model using BayerGB8 also.
          safe(EVT_AllocateFrameBuffer(&camera, &evtFrame[frame_count], EVT_FRAME_BUFFER_ZERO_COPY),&camera,"AllocateFrame");
      }
      for (int frame_count = 0; frame_count < FRAME_QUEUE; frame_count++)
      {
          safe(EVT_CameraQueueFrame(&camera, &evtFrame[frame_count + 1]),&camera,"QueueFrame");
      }


      printf("Ready. C to cancel otherwise any key to capture.\n");
      if (getch() == 'c') {
          EVT_CameraClose(&camera);
          printf("Camera closed.");
      }


      //Now, do multiframe software trigger grab.
      auto start = high_resolution_clock::now();
      printf("Acquisition Start!\n");
      safe(EVT_CameraExecuteCommand(&camera, "AcquisitionStart"), &camera);
      //safe(EVT_CameraExecuteCommand(&camera, "TriggerSoftware"), &camera);
      //Now read frames back.
      unsigned int frame_buffer = 0;
      unsigned int dropped_frames = 0;
      unsigned short id_prev = 0;
      unsigned int frame_count;
      for (frame_count = 0; frame_count < 10000; frame_count++)
      {
          frame_buffer = frame_count % FRAMES_IN_MEMORY;
          safe(EVT_CameraGetFrame(&camera, &evtFrame[frame_buffer],3000), &camera,"GetFrame");

          //Counting dropped frames through frame_id as redundant check. 
          if (frame_count != 0 && evtFrame[frame_buffer].frame_id != (evtFrame[(frame_count-1)%FRAMES_IN_MEMORY].frame_id % 65535) + 1) //Ignore very first frame as id is unknown.
          {
              dropped_frames++;
              printf("\nDropped Frame!\n");
          }
          safe(EVT_CameraQueueFrame(&camera, &evtFrame[(frame_buffer + FRAME_QUEUE)%FRAMES_IN_MEMORY]), &camera,"QueueFrame");
          printf("\rCapturing: %d/%d   ", frame_count+1, 3000);
          
          
#ifndef _MSC_VER 
          fflush(stdout);
#endif
      }
      auto stop = high_resolution_clock::now();
      auto duration = duration_cast<microseconds>(stop - start);
      float perframe = (float)duration.count() / (float) frame_count;
      printf("\n%d dropped frames. %.3f microseconds, %.3f per frame, %.3f fps.\n",dropped_frames,(float) duration.count(), perframe, (float) 1000000 / perframe);
      printf("Disarm stream...\n");

      //Tell camera to disarm streaming.
      safe(EVT_CameraExecuteCommand(&camera, "AcquisitionStop"), &camera);

      //Release frame buffers

      printf("Captured. C to discard otherwise any key to save.\n");
      if (getch() != 'c') {
          printf("Saving...\n");
          for (int i = 0; i < FRAMES_IN_MEMORY; i++)
          {
              
              int frame_index = (frame_count + i) % FRAMES_IN_MEMORY;
              //sprintf_s(filename, "R:\\Cache\\output\\myimage_%d.tif", i);
              //sprintf_s(filename, "V:\\output\\myimage_%d.tif", i);
              sprintf_s(filename, "C:\\Users\\sw902\\Desktop\\Output\\myimage_%d.tif", i);
              safe(EVT_FrameSave(&evtFrame[frame_index], filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE,camera.linesReorderHandle), &camera, "Frame Save");
              printf("\rSaving: %d/%d   ", i, FRAMES_IN_MEMORY);
          }
      }

      printf("Release buffers...\n");
      for (int frame_count = 0; frame_count < FRAMES_IN_MEMORY; frame_count++)
          safe(EVT_ReleaseFrameBuffer(&camera, &evtFrame[frame_count]), &camera);

      //Unconfigure acquisition.
      printf("Close stream...");
      safe(EVT_CameraCloseStream(&camera), &camera);

      //To avoid conflict with settings in other examples.
      configure_defaults(&camera);

      EVT_CameraClose(&camera);
      printf("\nClose Camera: \t\tCamera Closed\n");

      return(0);
  }
  catch (...) {
      printf("Caught Error!\n");
      EVT_CameraClose(&camera);
      printf("Camera closed.\n");
      return 1;
  }
} 

//A function to set all appropriate defaults so 
//running other examples does not require reconfiguration.
void configure_defaults(CEmergentCamera* camera)
{
  unsigned int width_max, height_max, param_val_max;
  const unsigned long enumBufferSize = 1000;
  unsigned long enumBufferSizeReturn = 0;
  char enumBuffer[enumBufferSize];

  //Order is important as param max/mins get updated.
  EVT_CameraGetEnumParamRange(camera, "PixelFormat", enumBuffer, enumBufferSize, &enumBufferSizeReturn);
  char* enumMember = strtok_s(enumBuffer, ",", &next_token);
  EVT_CameraSetEnumParam(camera,      "PixelFormat", enumMember);

  EVT_CameraSetUInt32Param(camera,    "FrameRate", 30);

  EVT_CameraSetUInt32Param(camera,    "OffsetX", 0);
  EVT_CameraSetUInt32Param(camera,    "OffsetY", 0);

  EVT_CameraGetUInt32ParamMax(camera, "Width", &width_max);
  EVT_CameraSetUInt32Param(camera,    "Width", width_max);

  EVT_CameraGetUInt32ParamMax(camera, "Height", &height_max);
  EVT_CameraSetUInt32Param(camera,    "Height", height_max);

  EVT_CameraSetEnumParam(camera,      "AcquisitionMode",        "Continuous");
  EVT_CameraSetUInt32Param(camera,    "AcquisitionFrameCount",  1);
  EVT_CameraSetEnumParam(camera,      "TriggerSelector",        "AcquisitionStart");
  EVT_CameraSetEnumParam(camera,      "TriggerMode",            "Off");
  EVT_CameraSetEnumParam(camera,      "TriggerSource",          "Software");
  //EVT_CameraSetEnumParam(camera,      "BufferMode",             "Off");
  //EVT_CameraSetUInt32Param(camera,    "BufferNum",              0);

  EVT_CameraGetUInt32ParamMax(camera, "GevSCPSPacketSize", &param_val_max);
  EVT_CameraSetUInt32Param(camera,    "GevSCPSPacketSize", param_val_max);

  EVT_CameraSetUInt32Param(camera,    "Gain", 256);
  EVT_CameraSetUInt32Param(camera, "Exposure", 1);
  EVT_CameraSetUInt32Param(camera,    "Offset", 0);

  EVT_CameraSetBoolParam(camera,      "LUTEnable", false);
  EVT_CameraSetBoolParam(camera,      "AutoGain", false);
}
