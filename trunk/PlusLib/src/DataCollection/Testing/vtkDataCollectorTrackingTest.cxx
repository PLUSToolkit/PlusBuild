#include "PlusConfigure.h"
#include "vtksys/CommandLineArguments.hxx"
#include "vtkSmartPointer.h"
#include "vtkDataCollector.h"
#include "vtkTrackerBuffer.h"

int main(int argc, char **argv)
{
	std::string inputConfigFileName;
	double inputAcqTimeLength(60);
	std::string outputTrackerBufferSequenceFileName("TrackerBufferMetafile"); 
	std::string outputFolder("./");

	int verboseLevel=PlusLogger::LOG_LEVEL_INFO;

	vtksys::CommandLineArguments args;
	args.Initialize(argc, argv);

	args.AddArgument("--input-config-file-name", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &inputConfigFileName, "Name of the input configuration file.");
	args.AddArgument("--input-acq-time-length", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &inputAcqTimeLength, "Length of acquisition time in seconds (Default: 60s)");	
	args.AddArgument("--output-tracker-buffer-seq-file-name", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &outputTrackerBufferSequenceFileName, "Filename of the output tracker bufffer sequence metafile (Default: TrackerBufferMetafile)");
	args.AddArgument("--output-folder", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &outputFolder, "Output folder (Default: ./)");
	args.AddArgument("--verbose", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &verboseLevel, "Verbose level (1=error only, 2=warning, 3=info, 4=debug)");	

	PlusLogger::Instance()->SetLogLevel(verboseLevel);

	if ( !args.Parse() )
	{
		std::cerr << "Problem parsing arguments" << std::endl;
		std::cout << "Help: " << args.GetHelp() << std::endl;
		exit(EXIT_FAILURE);
	}

	if (inputConfigFileName.empty())
	{
		std::cerr << "input-config-file-name is required" << std::endl;
		exit(EXIT_FAILURE);
	}

	///////////////

	VTK_LOG_TO_CONSOLE_ON; 

	vtkSmartPointer<vtkDataCollector> dataCollector = vtkSmartPointer<vtkDataCollector>::New(); 
	dataCollector->ReadConfiguration(inputConfigFileName.c_str());
	dataCollector->Initialize(); 
	dataCollector->Start();

	const double acqStartTime = vtkTimerLog::GetUniversalTime(); 

	while ( acqStartTime + inputAcqTimeLength > vtkTimerLog::GetUniversalTime() )
	{
		LOG_INFO( acqStartTime + inputAcqTimeLength - vtkTimerLog::GetUniversalTime() << " seconds left..." ); 
		vtksys::SystemTools::Delay(1000); 
	}


	LOG_INFO("Copy tracker..."); 
	vtkTracker *tracker = vtkTracker::New(); 
	dataCollector->CopyTracker(tracker); 

	LOG_INFO("Dump tracker to " << outputTrackerBufferSequenceFileName);
	dataCollector->DumpTrackerToMetafile( tracker, outputFolder.c_str(), outputTrackerBufferSequenceFileName.c_str(), true); 

	tracker->Delete(); 

	VTK_LOG_TO_CONSOLE_OFF; 

	std::cout << "Test completed successfully!" << std::endl;
	return EXIT_SUCCESS; 

}

