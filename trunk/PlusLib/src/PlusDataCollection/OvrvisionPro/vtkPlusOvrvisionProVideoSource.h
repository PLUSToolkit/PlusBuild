/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/

#ifndef __vtkPlusOvrvisionProVideoSource_h
#define __vtkPlusOvrvisionProVideoSource_h

#include "vtkPlusDataCollectionExport.h"
#include "vtkPlusDevice.h"

// OvrvisionPro SDK includes
#include <ovrvision_pro.h>

/*!
  \class __vtkPlusOvrvisionProVideoSource_h
  \brief Class for providing video input from the OvrvisionPro stereo camera device
  \ingroup PlusLibDataCollection
*/
class vtkPlusDataCollectionExport vtkPlusOvrvisionProVideoSource : public vtkPlusDevice
{
public:
  static vtkPlusOvrvisionProVideoSource* New();
  vtkTypeMacro( vtkPlusOvrvisionProVideoSource, vtkPlusDevice );
  void PrintSelf( ostream& os, vtkIndent indent );

  virtual bool IsTracker() const
  {
    return false;
  }

  /// Read configuration from xml data
  virtual PlusStatus ReadConfiguration( vtkXMLDataElement* config );
  /// Write configuration to xml data
  virtual PlusStatus WriteConfiguration( vtkXMLDataElement* config );

  /// Perform any completion tasks once configured
  virtual PlusStatus NotifyConfigured();

  vtkGetMacro( DirectShowFilterID, int );
  vtkGetMacro( USB3, bool );

protected:
  vtkSetMacro( DirectShowFilterID, int );
  vtkSetMacro( USB3, bool );

  /// Device-specific connect
  virtual PlusStatus InternalConnect();

  /// Device-specific disconnect
  virtual PlusStatus InternalDisconnect();

  /// Given a requested resolution and framerate
  bool ConfigureRequestedFormat(int resolution[2], int fps);

protected:
  vtkPlusOvrvisionProVideoSource();
  ~vtkPlusOvrvisionProVideoSource();

  // Callback when the SDK tells us a new frame is available
  void OnNewFrameAvailable();

protected:
  OVR::OvrvisionPro OvrvisionProHandle;

  // The filter ID to pass to the SDK
  int DirectShowFilterID;

  // Requested capture format
  OVR::Camprop RequestedFormat;

private:
  vtkPlusOvrvisionProVideoSource( const vtkPlusOvrvisionProVideoSource& ); // Not implemented.
  void operator=( const vtkPlusOvrvisionProVideoSource& ); // Not implemented.
};

#endif