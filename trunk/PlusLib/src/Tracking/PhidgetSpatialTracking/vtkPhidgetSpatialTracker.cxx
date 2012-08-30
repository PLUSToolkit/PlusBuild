/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

#include "PlusConfigure.h"

#include "vtkPhidgetSpatialTracker.h"

#include <sstream>

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtksys/SystemTools.hxx"
#include "vtkTransform.h"
#include "vtkXMLDataElement.h"

#include "PlusConfigure.h"
#include "vtkTracker.h"
#include "vtkTrackerTool.h"
#include "vtkTrackerBuffer.h"
 
#include <math.h>

vtkStandardNewMacro(vtkPhidgetSpatialTracker);

//-------------------------------------------------------------------------
vtkPhidgetSpatialTracker::vtkPhidgetSpatialTracker()
{ 
  this->SpatialDeviceHandle = 0;
  this->TrackerTimeToSystemTimeSec = 0;
  this->TrackerTimeToSystemTimeComputed = false;

  this->AccelerometerTool = NULL;
  this->GyroscopeTool = NULL;
  this->MagnetometerTool = NULL;
  this->OrientationSensorTool = NULL;

  this->LastAccelerometerToTrackerTransform=vtkMatrix4x4::New();
  this->LastGyroscopeToTrackerTransform=vtkMatrix4x4::New();
  this->LastMagnetometerToTrackerTransform=vtkMatrix4x4::New();
  this->LastOrientationSensorToTrackerTransform=vtkMatrix4x4::New();

  this->AhrsAlgorithmGain[0]=0.5; // proportional
  this->AhrsAlgorithmGain[1]=0.0; // integral

  this->LastAhrsUpdateTime=-1;
}

//-------------------------------------------------------------------------
vtkPhidgetSpatialTracker::~vtkPhidgetSpatialTracker() 
{
  if ( this->Recording )
  {
    this->StopTracking();
  }
  this->LastAccelerometerToTrackerTransform->Delete();
  this->LastAccelerometerToTrackerTransform=NULL;
  this->LastGyroscopeToTrackerTransform->Delete();
  this->LastGyroscopeToTrackerTransform=NULL;
  this->LastMagnetometerToTrackerTransform->Delete();
  this->LastMagnetometerToTrackerTransform=NULL;
  this->LastOrientationSensorToTrackerTransform->Delete();
  this->LastOrientationSensorToTrackerTransform=NULL;
}

//-------------------------------------------------------------------------
void vtkPhidgetSpatialTracker::PrintSelf( ostream& os, vtkIndent indent )
{
  vtkTracker::PrintSelf( os, indent );

  if (this->SpatialDeviceHandle!=NULL)
  {
    const char* deviceType=NULL;
    CPhidget_getDeviceType((CPhidgetHandle)this->SpatialDeviceHandle, &deviceType);
    os << "Device type: " << deviceType << std::endl;
    int serialNo=0;
    CPhidget_getSerialNumber((CPhidgetHandle)this->SpatialDeviceHandle, &serialNo);
    os << "Serial Number: " << serialNo << std::endl;
    int version=0;
    CPhidget_getDeviceVersion((CPhidgetHandle)this->SpatialDeviceHandle, &version);
    os << "Version: " << version << std::endl;
    int numAccelAxes=0;
    CPhidgetSpatial_getAccelerationAxisCount(this->SpatialDeviceHandle, &numAccelAxes);
    os << "Number of Accel Axes: " << numAccelAxes << std::endl;
    int numGyroAxes=0;
    CPhidgetSpatial_getGyroAxisCount(this->SpatialDeviceHandle, &numGyroAxes);
    os << "Number of Gyro Axes: " << numGyroAxes << std::endl;
    int numCompassAxes=0;
    CPhidgetSpatial_getCompassAxisCount(this->SpatialDeviceHandle, &numCompassAxes);
    os << "Number of Compass Axes: " << numCompassAxes << std::endl;
    int dataRateMax=0;
    CPhidgetSpatial_getDataRateMax(this->SpatialDeviceHandle, &dataRateMax);
    os << "Maximum data rate: " << dataRateMax << std::endl;
    int dataRateMin=0;    
    CPhidgetSpatial_getDataRateMin(this->SpatialDeviceHandle, &dataRateMin);
    os << "Minimum data rate: " << dataRateMin << std::endl;
    int dataRate=0;
    CPhidgetSpatial_getDataRate(this->SpatialDeviceHandle, &dataRate);    
    os << "Current data rate: " << dataRate << std::endl;
  }
  else
  {
    os << "Spatial device is not available" << std::endl;
  }

}

//callback that will run if the Spatial is attached to the computer
int CCONV AttachHandler(CPhidgetHandle spatial, void *trackerPtr)
{
	int serialNo;
	CPhidget_getSerialNumber(spatial, &serialNo);
  LOG_DEBUG("Phidget spatial sensor attached: " << serialNo);
	return 0;
}

//callback that will run if the Spatial is detached from the computer
int CCONV DetachHandler(CPhidgetHandle spatial, void *trackerPtr)
{
	int serialNo;
	CPhidget_getSerialNumber(spatial, &serialNo);
	LOG_DEBUG("Phidget spatial sensor detached: " << serialNo);
	return 0;
}

//callback that will run if the Spatial generates an error
int CCONV ErrorHandler(CPhidgetHandle spatial, void *trackerPtr, int ErrorCode, const char *unknown)
{
  LOG_ERROR("Phidget spatial sensor error: "<<ErrorCode<<" ("<<unknown<<")");
	return 0;
}

//callback that will run at datarate
//data - array of spatial event data structures that holds the spatial data packets that were sent in this event
//count - the number of spatial data event packets included in this event
int CCONV vtkPhidgetSpatialTracker::SpatialDataHandler(CPhidgetSpatialHandle spatial, void *trackerPtr, CPhidgetSpatial_SpatialEventDataHandle *data, int count)
{
  vtkPhidgetSpatialTracker* tracker=(vtkPhidgetSpatialTracker*)trackerPtr;
  if ( ! tracker->IsTracking() )
  {
    // Received phidget tracking data when not tracking
    return 0;
  }

  if ( count<1 )
  {
    LOG_WARNING("No phidget data received in data handler" );
    return 0;
  }

  if (!tracker->TrackerTimeToSystemTimeComputed)
  {
    const double timeSystemSec = vtkAccurateTimer::GetSystemTime();
    const double timeTrackerSec = data[count-1]->timestamp.seconds+data[count-1]->timestamp.microseconds*1e-6;
    tracker->TrackerTimeToSystemTimeSec = timeSystemSec-timeTrackerSec;
    tracker->TrackerTimeToSystemTimeComputed = true;
  }

  LOG_TRACE("Number of phidget data packets received in the current event: " << count);

  for(int i = 0; i < count; i++)
  { 
    const double timeTrackerSec = data[i]->timestamp.seconds+data[i]->timestamp.microseconds*1e-6;
    const double timeSystemSec = timeTrackerSec + tracker->TrackerTimeToSystemTimeSec;        

    if (tracker->AccelerometerTool!=NULL)
    {
      ConvertVectorToTransformationMatrix(data[i]->acceleration, tracker->LastAccelerometerToTrackerTransform);
      tracker->ToolTimeStampedUpdateWithoutFiltering( tracker->AccelerometerTool->GetToolName(), tracker->LastAccelerometerToTrackerTransform, TOOL_OK, timeSystemSec, timeSystemSec);
    }  
    if (tracker->GyroscopeTool!=NULL)
    {
      vtkSmartPointer<vtkTransform> transform=vtkSmartPointer<vtkTransform>::New();
      transform->RotateX(data[i]->angularRate[0]/10.0);
      transform->RotateY(data[i]->angularRate[1]/10.0);  
      transform->RotateY(data[i]->angularRate[2]/10.0);  
      transform->GetMatrix(tracker->LastGyroscopeToTrackerTransform);
      tracker->ToolTimeStampedUpdateWithoutFiltering( tracker->GyroscopeTool->GetToolName(), tracker->LastGyroscopeToTrackerTransform, TOOL_OK, timeSystemSec, timeSystemSec);
    }  
    if (tracker->MagnetometerTool!=NULL)
    {      
      if (data[i]->magneticField[0]>1e100)
      {
        // magnetometer data is not available, use the last transform with an invalid status to not have any missing transform
        tracker->ToolTimeStampedUpdateWithoutFiltering( tracker->MagnetometerTool->GetToolName(), tracker->LastMagnetometerToTrackerTransform, TOOL_INVALID, timeSystemSec, timeSystemSec);
      }
      else
      {
        // magnetometer data is valid
        ConvertVectorToTransformationMatrix(data[i]->magneticField, tracker->LastMagnetometerToTrackerTransform);        
        tracker->ToolTimeStampedUpdateWithoutFiltering( tracker->MagnetometerTool->GetToolName(), tracker->LastMagnetometerToTrackerTransform, TOOL_OK, timeSystemSec, timeSystemSec);
      }
    }     

    if (tracker->OrientationSensorTool!=NULL)
    {
      if (data[i]->magneticField[0]>1e100)
      {
        // magnetometer data is not available, use the last transform with an invalid status to not have any missing transform
        tracker->ToolTimeStampedUpdateWithoutFiltering( tracker->OrientationSensorTool->GetToolName(), tracker->LastOrientationSensorToTrackerTransform, TOOL_INVALID, timeSystemSec, timeSystemSec);        
      }
      else
      {
        // magnetometer data is valid

        if (tracker->LastAhrsUpdateTime<0)
        {
          // this is the first update
          // just use it as a reference
          tracker->LastAhrsUpdateTime=timeSystemSec;
          continue;
        }

        // Settings
        // which AHRS algo to use
        const bool useMadgwick=true; 
        // enable/disable input for debugging
        const bool useGyroscope=true;
        const bool useAccelerometer=true;
        const bool useMagnetometer=true;       
       
        if (!useMagnetometer)
        {
          data[i]->magneticField[0]=0;
          data[i]->magneticField[1]=0;
          data[i]->magneticField[2]=0;
        }
        if (!useAccelerometer)
        {
          data[i]->acceleration[0]=0;
          data[i]->acceleration[1]=0;
          data[i]->acceleration[2]=0;
        }
        if (!useGyroscope)
        {
          data[i]->angularRate[0]=0;
          data[i]->angularRate[1]=0;
          data[i]->angularRate[2]=0;
        }
        
        double timeSinceLastAhrsUpdateSec=timeSystemSec-tracker->LastAhrsUpdateTime;
        tracker->LastAhrsUpdateTime=timeSystemSec;

        tracker->MadgwickAhrsAlgo.SetSampleFreqHz(1.0/timeSinceLastAhrsUpdateSec);
        tracker->MahonyAhrsAlgo.SetSampleFreqHz(1.0/timeSinceLastAhrsUpdateSec);

        //LOG_INFO("samplingTime(msec)="<<1000.0*timeSinceLastAhrsUpdateSec<<", packetCount="<<count);
        //LOG_INFO("gyroX="<<std::fixed<<std::setprecision(2)<<std::setw(6)<<data[i]->angularRate[0]<<", gyroY="<<data[i]->angularRate[1]<<", gyroZ="<<data[i]->angularRate[2]);               
        //LOG_INFO("magX="<<std::fixed<<std::setprecision(2)<<std::setw(6)<<data[i]->magneticField[0]<<", magY="<<data[i]->magneticField[1]<<", magZ="<<data[i]->magneticField[2]);               

        if (useMadgwick)
        {
          tracker->MadgwickAhrsAlgo.Update(          
            vtkMath::RadiansFromDegrees(data[i]->angularRate[0]), vtkMath::RadiansFromDegrees(data[i]->angularRate[1]), vtkMath::RadiansFromDegrees(data[i]->angularRate[2]),
            data[i]->acceleration[0], data[i]->acceleration[1], data[i]->acceleration[2],
            data[i]->magneticField[0], data[i]->magneticField[1], data[i]->magneticField[2]);
        }
        else
        {
          tracker->MahonyAhrsAlgo.Update(
            vtkMath::RadiansFromDegrees(data[i]->angularRate[0]), vtkMath::RadiansFromDegrees(data[i]->angularRate[1]), vtkMath::RadiansFromDegrees(data[i]->angularRate[2]),
            data[i]->acceleration[0], data[i]->acceleration[1], data[i]->acceleration[2],
            data[i]->magneticField[0], data[i]->magneticField[1], data[i]->magneticField[2]);
        }

        double rotQuat[4]={0};
        if (useMadgwick)
        {
          tracker->MadgwickAhrsAlgo.GetOrientation(rotQuat[0],rotQuat[1],rotQuat[2],rotQuat[3]);
        }
        else
        {
          tracker->MahonyAhrsAlgo.GetOrientation(rotQuat[0],rotQuat[1],rotQuat[2],rotQuat[3]);
        }
        double rotMatrix[3][3]={0};
        vtkMath::QuaternionToMatrix3x3(rotQuat, rotMatrix); 

        for (int c=0;c<3; c++)
        {
          for (int r=0;r<3; r++)
          {
            tracker->LastOrientationSensorToTrackerTransform->SetElement(r,c,rotMatrix[r][c]);
          }
        }
        tracker->ToolTimeStampedUpdateWithoutFiltering( tracker->OrientationSensorTool->GetToolName(), tracker->LastOrientationSensorToTrackerTransform, TOOL_OK, timeSystemSec, timeSystemSec);    
      }            

    }    
  }

  return 0;
}

void vtkPhidgetSpatialTracker::ConvertVectorToTransformationMatrix(double *inputVector, vtkMatrix4x4* outputMatrix)
{
  // Compose matrix that transforms the x axis to the input vector by rotations around two orthogonal axes
  double primaryRotationAngleDeg = atan2( inputVector[2],sqrt(inputVector[1]*inputVector[1]+inputVector[0]*inputVector[0]) )*180.0/vtkMath::Pi();
  double secondaryRotationAngleDeg = atan2( inputVector[1],sqrt( inputVector[2]*inputVector[2] + inputVector[0]*inputVector[0]) )*180.0/vtkMath::Pi();
  //LOG_TRACE("Pri="<<primaryRotationAngleDeg<<"  Sec="<<secondaryRotationAngleDeg<<"    x="<<inputVector[0]<<"    y="<<inputVector[1]<<"    z="<<inputVector[2]);

  vtkSmartPointer<vtkTransform> transform=vtkSmartPointer<vtkTransform>::New();
  transform->RotateX(primaryRotationAngleDeg);
  transform->RotateY(secondaryRotationAngleDeg);  
  transform->GetMatrix(outputMatrix);
}

//-------------------------------------------------------------------------
PlusStatus vtkPhidgetSpatialTracker::Connect()
{
  LOG_TRACE( "vtkPhidgetSpatialTracker::Connect" ); 

  this->AccelerometerTool = NULL;
  GetToolByPortName("Accelerometer", this->AccelerometerTool);

  this->GyroscopeTool = NULL;
  GetToolByPortName("Gyroscope", this->GyroscopeTool);

  this->MagnetometerTool = NULL;
  GetToolByPortName("Magnetometer", this->MagnetometerTool);

  this->OrientationSensorTool = NULL;
  GetToolByPortName("OrientationSensor", this->OrientationSensorTool);

	//Create the spatial object  
	CPhidgetSpatial_create(&this->SpatialDeviceHandle);

	//Set the handlers to be run when the device is plugged in or opened from software, unplugged or closed from software, or generates an error.
	CPhidget_set_OnAttach_Handler((CPhidgetHandle)this->SpatialDeviceHandle, AttachHandler, this);
	CPhidget_set_OnDetach_Handler((CPhidgetHandle)this->SpatialDeviceHandle, DetachHandler, this);
	CPhidget_set_OnError_Handler((CPhidgetHandle)this->SpatialDeviceHandle, ErrorHandler, this);

	//Registers a callback that will run according to the set data rate that will return the spatial data changes
	//Requires the handle for the Spatial, the callback handler function that will be called, 
	//and an arbitrary pointer that will be supplied to the callback function (may be NULL)

	CPhidgetSpatial_set_OnSpatialData_Handler(this->SpatialDeviceHandle, vtkPhidgetSpatialTracker::SpatialDataHandler, this);

  // TODO: verify tool definition

  this->TrackerTimeToSystemTimeSec = 0;
  this->TrackerTimeToSystemTimeComputed = false;

  //open the spatial object for device connections
	CPhidget_open((CPhidgetHandle)this->SpatialDeviceHandle, -1);

	//get the program to wait for a spatial device to be attached
	LOG_DEBUG("Waiting for phidget spatial device to be attached...");
  int result=0;
	if((result = CPhidget_waitForAttachment((CPhidgetHandle)this->SpatialDeviceHandle, 10000)))
	{
    const char *err=NULL;
		CPhidget_getErrorDescription(result, &err);
    LOG_ERROR( "Couldn't initialize vtkPhidgetSpatialTracker: Problem waiting for attachment (" << err << ")");
    return PLUS_FAIL;
	}

	//Set the data rate for the spatial events
  int userDataRateMsec=1000/this->GetAcquisitionRate();
  // Allowed userDataRateMsec values: 8, 16, 32, 64, 128, 256, 512, 1000, set the closest one
  if (userDataRateMsec<12) { userDataRateMsec=8; }
  else if (userDataRateMsec<24) { userDataRateMsec=16; }
  else if (userDataRateMsec<48) { userDataRateMsec=32; }
  else if (userDataRateMsec<96) { userDataRateMsec=64; }
  else if (userDataRateMsec<192) { userDataRateMsec=128; }
  else if (userDataRateMsec<384) { userDataRateMsec=256; }
  else if (userDataRateMsec<756) { userDataRateMsec=512; }
  else { userDataRateMsec=1000; }
	CPhidgetSpatial_setDataRate(this->SpatialDeviceHandle, userDataRateMsec);
	LOG_DEBUG("DataRate (msec):" << userDataRateMsec);

  // Initialize both AHRS algorithm implementations, we will decide in the update step which one to actually use

  // Set some default frequency here, more accurate value will be set at each update step anyway
  this->MadgwickAhrsAlgo.SetSampleFreqHz(1000.0/userDataRateMsec);
  this->MahonyAhrsAlgo.SetSampleFreqHz(1000.0/userDataRateMsec);

  this->MadgwickAhrsAlgo.SetGain(this->AhrsAlgorithmGain[0]);
  this->MahonyAhrsAlgo.SetGain(this->AhrsAlgorithmGain[0], this->AhrsAlgorithmGain[1]);

  // If magnetic fields nearby the sensor have non-negligible effect then compass correction may be needed (see http://www.phidgets.com/docs/Compass_Primer)
  // To set compass correction parameters:
  //  CPhidgetSpatial_setCompassCorrectionParameters(this->SpatialDeviceHandle, 0.648435, 0.002954, -0.024140, 0.002182, 1.520509, 1.530625, 1.575390, -0.002039, 0.003182, -0.001966, -0.013848, 0.003168, -0.014385);
  // To reset compass correction parameters:
  //  CPhidgetSpatial_resetCompassCorrectionParameters(this->SpatialDeviceHandle);
  
  // Determine the gyroscope sensors offset by integrating the gyroscope values for 2 seconds while the sensor is stationary.
  CPhidgetSpatial_zeroGyro(this->SpatialDeviceHandle);

  return PLUS_SUCCESS; 
}

//-------------------------------------------------------------------------
PlusStatus vtkPhidgetSpatialTracker::Disconnect()
{
  LOG_TRACE( "vtkPhidgetSpatialTracker::Disconnect" ); 
  this->StopTracking();
  CPhidget_close((CPhidgetHandle)this->SpatialDeviceHandle);
  CPhidget_delete((CPhidgetHandle)this->SpatialDeviceHandle);
  this->SpatialDeviceHandle = NULL;
  this->AccelerometerTool = NULL;
  this->GyroscopeTool = NULL;
  this->MagnetometerTool = NULL;
  this->OrientationSensorTool = NULL;
  return PLUS_SUCCESS;
}

//-------------------------------------------------------------------------
PlusStatus vtkPhidgetSpatialTracker::Probe()
{
  LOG_TRACE( "vtkPhidgetSpatialTracker::Probe" ); 

  return PLUS_SUCCESS; 
} 

//-------------------------------------------------------------------------
PlusStatus vtkPhidgetSpatialTracker::InternalStartTracking()
{
  LOG_TRACE( "vtkPhidgetSpatialTracker::InternalStartTracking" ); 
  if ( this->Recording )
  {
    return PLUS_SUCCESS;
  }  

  return PLUS_SUCCESS;
}

//-------------------------------------------------------------------------
PlusStatus vtkPhidgetSpatialTracker::InternalStopTracking()
{
  LOG_TRACE( "vtkPhidgetSpatialTracker::InternalStopTracking" );   

  return PLUS_SUCCESS;
}

//-------------------------------------------------------------------------
PlusStatus vtkPhidgetSpatialTracker::InternalUpdate()
{
  LOG_TRACE( "vtkPhidgetSpatialTracker::InternalUpdate" ); 
  return PLUS_SUCCESS;
}

//-------------------------------------------------------------------------
PlusStatus vtkPhidgetSpatialTracker::InitPhidgetSpatialTracker()
{
  LOG_TRACE( "vtkPhidgetSpatialTracker::InitPhidgetSpatialTracker" ); 
  return this->Connect(); 
}

//----------------------------------------------------------------------------
PlusStatus vtkPhidgetSpatialTracker::ReadConfiguration(vtkXMLDataElement* config)
{
  // Read superclass configuration first
  Superclass::ReadConfiguration(config); 

  if ( config == NULL ) 
  {
    LOG_WARNING("Unable to find BrachyTracker XML data element");
    return PLUS_FAIL; 
  }

  vtkXMLDataElement* dataCollectionConfig = config->FindNestedElementWithName("DataCollection");
  if (dataCollectionConfig == NULL)
  {
    LOG_ERROR("Cannot find DataCollection element in XML tree!");
    return PLUS_FAIL;
  }

  vtkXMLDataElement* trackerConfig = dataCollectionConfig->FindNestedElementWithName("Tracker"); 
  if (trackerConfig == NULL) 
  {
    LOG_ERROR("Cannot find Tracker element in XML tree!");
    return PLUS_FAIL;
  }

  double ahrsAlgorithmGain[2]={0}; 
  if ( trackerConfig->GetVectorAttribute("AhrsAlgorithmGain", 2, ahrsAlgorithmGain ) ) 
  {
    this->AhrsAlgorithmGain[0]=ahrsAlgorithmGain[0]; 
    this->AhrsAlgorithmGain[1]=ahrsAlgorithmGain[1]; 
  }

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPhidgetSpatialTracker::WriteConfiguration(vtkXMLDataElement* rootConfigElement)
{
  if ( rootConfigElement == NULL )
  {
    LOG_ERROR("Configuration is invalid");
    return PLUS_FAIL;
  }

  // Write configuration 
  Superclass::WriteConfiguration(rootConfigElement); 

  // Get data collection and then Tracker configuration element
  vtkXMLDataElement* dataCollectionConfig = rootConfigElement->FindNestedElementWithName("DataCollection");
  if (dataCollectionConfig == NULL)
  {
    LOG_ERROR("Cannot find DataCollection element in XML tree!");
    return PLUS_FAIL;
  }

  vtkSmartPointer<vtkXMLDataElement> trackerConfig = dataCollectionConfig->FindNestedElementWithName("Tracker"); 
  if ( trackerConfig == NULL) 
  {
    LOG_ERROR("Cannot find Tracker element in XML tree!");
    return PLUS_FAIL;
  }

  if (this->AhrsAlgorithmGain[1]==0.0)
  {
    // if the second gain parameter is zero then just write the first value
    trackerConfig->SetDoubleAttribute( "AhrsAlgorithmGain", this->AhrsAlgorithmGain[0] ); 
  }
  else
  {
    trackerConfig->SetVectorAttribute( "AhrsAlgorithmGain", 2, this->AhrsAlgorithmGain );
  }

  return PLUS_SUCCESS;
}