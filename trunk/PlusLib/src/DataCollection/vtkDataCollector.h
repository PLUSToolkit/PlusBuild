// .NAME vtkDataCollector
// .SECTION Description

#ifndef __vtkDataCollector_h
#define __vtkDataCollector_h

#include "vtkMultiThreader.h"
#include "PlusConfigure.h"
#include "vtkImageAlgorithm.h" 
#include "vtkXMLDataElement.h"
#include "vtkImageData.h"
#include "vtkTimerLog.h"
#include "vtksys/SystemTools.hxx"

#include "vtkVideoSource2.h"
#include "vtkVideoBuffer2.h"
#include "vtkTracker.h"
#include "vtkTrackerBuffer.h"
#include "vtkTrackedFrameList.h"
#include "vtkDataCollectorSynchronizer.h"

// Acquisition types
enum ACQUISITION_TYPE 
{
	SYNCHRO_VIDEO_NONE=0, 
	SYNCHRO_VIDEO_SAVEDDATASET,
	SYNCHRO_VIDEO_NOISE,
	SYNCHRO_VIDEO_MIL,
	SYNCHRO_VIDEO_WIN32,
	SYNCHRO_VIDEO_LINUX,
	SYNCHRO_VIDEO_SONIX,
	SYNCHRO_VIDEO_ICCAPTURING
}; 

// Tracker types
enum TRACKER_TYPE
{
	TRACKER_NONE=0, 
	TRACKER_SAVEDDATASET,
	TRACKER_AMS, 
	TRACKER_CERTUS, 
	TRACKER_POLARIS, 
	TRACKER_AURORA, 
	TRACKER_FLOCK, 
	TRACKER_MICRON,
	TRACKER_FAKE,
	TRACKER_ASCENSION3DG
}; 

class VTK_EXPORT vtkDataCollector: public vtkImageAlgorithm
{
public:
	static vtkDataCollector *New();
	vtkTypeRevisionMacro(vtkDataCollector,vtkImageAlgorithm);
	virtual void PrintSelf(ostream& os, vtkIndent indent);

	// Description:
	// Initialize the data collector and connect to devices 
	virtual void Initialize(); 

	// Description:
	// Read the configuration file in XML format and set up the devices
	virtual void ReadConfiguration( const char* configFileName); 
	virtual void ReadConfiguration(); 

	// Description:
	// Disconnect from devices
	virtual void Disconnect(); 
	
	// Description:
	// Connect to devices
	virtual void Connect(); 

	// Description:
	// Stop data collection
	virtual void Stop(); 

	// Description:
	// Start data collection 
	virtual void Start(); 

	// Description:
	// Synchronize the connected devices
	virtual void Synchronize(const bool saveSyncData = false); 

	// Description:
	// Copy the current state of the tracker buffer 
	virtual void CopyTrackerBuffer( vtkTrackerBuffer* trackerBuffer, int toolNumber ); 

	// Description:
	// Copy the current state of the tracker (with each tools and buffers)
	virtual void CopyTracker( vtkTracker* tracker); 

	// Description:
	// Dump the current state of the tracker to metafile (with each tools and buffers)
	virtual void DumpTrackerToMetafile( vtkTracker* tracker, const char* outputFolder, const char* metaFileName, bool useCompression = false ); 
	
	// Description:
	// Copy the current state of the video buffer 
	virtual void CopyVideoBuffer( vtkVideoBuffer2* videoBuffer ); 

	// Description:
	// Dump the current state of the video buffer to metafile
	virtual void DumpVideoBufferToMetafile( vtkVideoBuffer2* videoBuffer, const char* outputFolder, const char* metaFileName, bool useCompression = false ); 

	// Description:
	// Return the most recent frame timestamp in the buffer
	virtual double GetMostRecentTimestamp(); 

	// Description:
	// Return the main tool status at a given time
	virtual long GetMainToolStatus( double time ); 

	// Description:
	// Return the main tool name
	virtual std::string GetMainToolName(); 

	// Description:
	// Get the tracked frame from devices 
	virtual void GetTrackedFrame(vtkImageData* frame, vtkMatrix4x4* toolTransMatrix, long& flags, double& synchronizedTime, int toolNumber = 0); 

	// Description:
	// Get the tracked frame from devices with each tool transforms
	virtual void GetTrackedFrame(vtkImageData* frame, std::vector<vtkMatrix4x4*> &toolTransforms, std::vector<std::string> &toolTransformNames, std::vector<long> &flags, double& synchronizedTime); 
	
	// Description:
	// Get the tracked frame from devices with each tool transforms
	virtual void GetTrackedFrame(TrackedFrame* trackedFrame); 

	// Description:
	// Get the tracked frame from devices by time with each tool transforms
	virtual void GetTrackedFrameByTime(const double time, TrackedFrame* trackedFrame); 

	// Description:
	// Get the tracked frame from devices by time with each tool transforms
	virtual void GetTrackedFrameByTime(const double time, vtkImageData* frame, std::vector<vtkMatrix4x4*> &toolTransforms, std::vector<std::string> &toolTransformNames, std::vector<long> &flags, double& synchronizedTime); 

	// Description:
	// Get the frame timestamp by time 
	virtual double GetFrameTimestampByTime(double time); 

	// Description:
	// Get transformation with timestamp from tracker 
	virtual void GetTransformWithTimestamp(vtkMatrix4x4* toolTransMatrix, double& transformTimestamp, long& flags, int toolNumber = 0); 

	// Description:
	// Get transformation by timestamp from tracker 
	virtual void GetTransformByTimestamp(vtkMatrix4x4* toolTransMatrix, long& flags, const double synchronizedTime, int toolNumber = 0); 

	// Description:
	// Get transformations by timestamp range from tracker. The first returned transform is the one after the startTime, except if startTime is -1, then it refers to the oldest one. '-1' for end time means the latest transform. Returns the timestamp of the requested transform (makes sense if endTime is -1)
	virtual double GetTransformsByTimeInterval(std::vector<vtkMatrix4x4*> &toolTransMatrixVector, std::vector<long> &flagsVector, const double startTime, const double endTime, int toolNumber = 0);

	// Description:
	// Get frame data with timestamp 
	virtual void GetFrameWithTimestamp(vtkImageData* frame, double& frameTimestamp); 

	// Description:
	// Get frame data by time 
	virtual void GetFrameByTime(const double time, vtkImageData* frame, double& frameTimestamp); 

	// Description:
	// Find the next active tracker tool number 
	virtual int GetNextActiveToolNumber(); 
	
	// Description:
	// Find the previous active tracker tool number 
	virtual int GetPreviousActiveToolNumber(); 

	// Description:
	// Set/Get the acquisition type 
	void SetAcquisitionType(ACQUISITION_TYPE type) { AcquisitionType = type; }
	ACQUISITION_TYPE GetAcquisitionType() { return this->AcquisitionType; }

	// Description:
	// Set/Get the tracker type 
	void SetTrackerType(TRACKER_TYPE type) { TrackerType = type; }
	TRACKER_TYPE GetTrackerType() { return this->TrackerType; }

	// Description:
	// Get the tool transformation matrix
	virtual vtkMatrix4x4* GetToolTransMatrix( unsigned int toolNumber = 0 ) ;
	
	// Description:
	// Get the tool flags (for more details see vtkTracker.h (e.g. TR_MISSING) 
	virtual int GetToolFlags( unsigned int toolNumber/* = 0 */); 

	// Description:
	// Set video and tracker local time offset 
	virtual void SetLocalTimeOffset(double videoOffset, double trackerOffset); 

	// Description:
	// Set/Get the configuration file name
	vtkSetStringMacro(ConfigFileName); 
	vtkGetStringMacro(ConfigFileName); 

	// Description:
	// Set/Get the video source of ultrasound
	vtkSetObjectMacro(VideoSource,vtkVideoSource2);
	vtkGetObjectMacro(VideoSource,vtkVideoSource2);

	// Description:
	// Set/Get the video source of ultrasound
	vtkSetObjectMacro(Synchronizer,vtkDataCollectorSynchronizer);
	vtkGetObjectMacro(Synchronizer,vtkDataCollectorSynchronizer);

	// Description:
	// Set/Get the tracker 
	vtkSetObjectMacro(Tracker,vtkTracker);
	vtkGetObjectMacro(Tracker,vtkTracker);

	// Description:	
	// Get/Set the main tool number 
	vtkSetMacro(MainToolNumber, int); 
	vtkGetMacro(MainToolNumber, int);

	// Description:	
	// Get/Set the maximum buffer size to dump
	vtkSetMacro(DumpBufferSize, int); 
	vtkGetMacro(DumpBufferSize, int);
	
	// Description:
	// Set/get the Initialized flag 
	vtkSetMacro(Initialized,bool);
	vtkGetMacro(Initialized,bool);
	vtkBooleanMacro(Initialized, bool); 

	// Description:
	// Set/get the Tracking only flag
	vtkGetMacro(TrackingOnly,bool);
	void SetTrackingOnly(bool);

	// Description:
	// Set/get the Video only flag
	vtkGetMacro(VideoOnly,bool);
	void SetVideoOnly(bool);

	// Description:
	// Set/get the cancel sync request flag to cancel the active sync job 
	vtkSetMacro(CancelSyncRequest, bool); 
	vtkGetMacro(CancelSyncRequest, bool); 
	vtkBooleanMacro(CancelSyncRequest, bool); 
	
	int GetNumberOfTools();

	//! Description 
	// Callback function for progress bar refreshing
	typedef void (*ProgressBarUpdatePtr)(int percent);
    void SetProgressBarUpdateCallbackFunction(ProgressBarUpdatePtr cb) { ProgressBarUpdateCallbackFunction = cb; } 
	
protected:
	vtkDataCollector();
	virtual ~vtkDataCollector();

	// This is called by the superclass.
	virtual int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

	// Description:
	// Read image acqusition properties from xml file 
	virtual void ReadImageAcqusitionProperties(vtkXMLDataElement* imageAcqusitionConfig); 

	// Description:
	// Read tracker properties from xml file 
	virtual void ReadTrackerProperties(vtkXMLDataElement* trackerConfig); 

	// Description:
	// Read synchronization properties from xml file 
	virtual void ReadSynchronizationProperties(vtkXMLDataElement* synchronizationConfig); 

	// Description:
	// Convert vtkImageData to itkImage (TrackedFrame::ImageType)
	virtual void ConvertVtkImageToItkImage(vtkImageData* inFrame, TrackedFrame::ImageType* outFrame); 

	//! Pointer to the progress bar update callback function 
	ProgressBarUpdatePtr ProgressBarUpdateCallbackFunction; 

	vtkDataCollectorSynchronizer* Synchronizer; 
	
	vtkVideoSource2*	VideoSource; 
	vtkTracker*			Tracker; 

	ACQUISITION_TYPE	AcquisitionType; 
	TRACKER_TYPE		TrackerType; 

	std::vector<vtkMatrix4x4*> ToolTransMatrices; 
	std::vector<int>	ToolFlags; 

	vtkXMLDataElement*	ConfigurationData;
	char*				ConfigFileName; 

	bool Initialized; 
	
	int MainToolNumber; 

	int MostRecentFrameBufferIndex; 
	
	int DumpBufferSize;

	bool TrackingOnly;
	bool VideoOnly;

	bool CancelSyncRequest; 

private:
	vtkDataCollector(const vtkDataCollector&);
	void operator=(const vtkDataCollector&);

};

#endif
