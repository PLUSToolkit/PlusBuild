/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/ 

/*=========================================================================
The following copyright notice is applicable to parts of this file:
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
Authors include: Danielle Pace
(Robarts Research Institute and The University of Western Ontario)
=========================================================================*/ 

#ifndef __vtkPlusMmfVideoSource_h
#define __vtkPlusMmfVideoSource_h

#include "vtkPlusDataCollectionExport.h"

#include "vtkPlusRecursiveCriticalSection.h"
#include "vtkSmartPointer.h"

#include "PlusVideoFrame.h"
#include "vtkPlusDevice.h"

class MmfVideoSourceReader;

/*!
  \class vtkPlusMmfVideoSource 
  \brief Microsoft media foundation video digitizer

  Media foundation require Microsoft Windows SDK 7.1 or later. Download <a href="http://www.microsoft.com/en-us/download/details.aspx?id=8279">here</a>

  \sa vtkPlusDevice
  \ingroup PlusLibDataCollection
*/ 
class vtkPlusDataCollectionExport vtkPlusMmfVideoSource : public vtkPlusDevice
{
  friend class MmfVideoSourceReader;

  struct VideoFormat
  {
    unsigned int DeviceId;
    unsigned int StreamIndex;
    int FrameSize[2];
    std::string PixelFormatName; // e.g., YUY2    
  };

public:
  static vtkPlusMmfVideoSource *New();
  vtkTypeMacro(vtkPlusMmfVideoSource, vtkPlusDevice);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetRequestedDeviceId(unsigned int deviceId);  
  virtual void SetRequestedVideoFormat(const std::string& pixelFormatName);
  virtual void SetRequestedStreamIndex(unsigned int streamIndex);
  virtual void SetRequestedFrameSize(int frameSize[2]);
  
  std::string GetRequestedDeviceName();
  std::string GetActiveDeviceName();
  void GetListOfCaptureVideoFormats(std::vector< std::string > &videoModes, unsigned int deviceId);
  void LogListOfCaptureVideoFormats(unsigned int deviceId);
  void GetListOfCaptureDevices(std::vector< std::string > &deviceNames);
  void LogListOfCaptureDevices();

  virtual PlusStatus ReadConfiguration(vtkXMLDataElement* xmlElement);
  virtual PlusStatus WriteConfiguration(vtkXMLDataElement* xmlElement);

  /*! Verify the device is correctly configured */
  virtual PlusStatus NotifyConfigured();

  virtual bool IsTracker() const { return false; }

protected:
  /*! Constructor */
  vtkPlusMmfVideoSource();
  /*! Destructor */
  ~vtkPlusMmfVideoSource();

  /*! Device-specific connect */
  virtual PlusStatus InternalConnect();

  /*! Device-specific disconnect */
  virtual PlusStatus InternalDisconnect();

  /*! Device-specific recording start */
  virtual PlusStatus InternalStartRecording();

  /*! Device-specific recording stop */
  virtual PlusStatus InternalStopRecording();

  PlusStatus UpdateFrameSize();

  PlusStatus AddFrame(unsigned char* bufferData, DWORD bufferSize);

  std::string GetCaptureDeviceName(unsigned int deviceId);

  int FrameIndex;

  vtkSmartPointer<vtkPlusRecursiveCriticalSection> Mutex;
  PlusVideoFrame UncompressedVideoFrame;
  VideoFormat RequestedVideoFormat;
  VideoFormat ActiveVideoFormat;

  MmfVideoSourceReader* MmfSourceReader;
private:
  vtkPlusMmfVideoSource(const vtkPlusMmfVideoSource&);  // Not implemented.
  void operator=(const vtkPlusMmfVideoSource&);  // Not implemented.
};

#endif
