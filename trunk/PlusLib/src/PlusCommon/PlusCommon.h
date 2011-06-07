#ifndef __PlusCommon_h
#define __PlusCommon_h

#include "vtkOutputWindow.h"

enum PlusStatus
{   
  PLUS_FAIL=0,
  PLUS_SUCCESS=1
};

/* Define case insensitive string compare for Windows. */
#if defined( _WIN32 ) && !defined(__CYGWIN__)
#  if defined(__BORLANDC__)
#    define STRCASECMP stricmp
#  else
#    define STRCASECMP _stricmp
#  endif
#else
#  define STRCASECMP STRCASECMP
#endif

#define ROUND(x) (static_cast<int>(floor( x + 0.5 )))

///////////////////////////////////////////////////////////////////
// Logging

#define LOG_ERROR(msg) \
	{ \
	std::ostrstream msgStream; \
  msgStream << " " << msg << std::ends; \
	PlusLogger::Instance()->LogMessage(PlusLogger::LOG_LEVEL_ERROR, msgStream.str(), __FILE__, __LINE__); \
	msgStream.rdbuf()->freeze(); \
	}	

#define LOG_WARNING(msg) \
	{ \
	std::ostrstream msgStream; \
	msgStream << " " << msg << std::ends; \
  PlusLogger::Instance()->LogMessage(PlusLogger::LOG_LEVEL_WARNING, msgStream.str(), __FILE__, __LINE__); \
  msgStream.rdbuf()->freeze(); \
	}
		
#define LOG_INFO(msg) \
	{ \
	std::ostrstream msgStream; \
	msgStream << " " << msg << std::ends; \
	PlusLogger::Instance()->LogMessage(PlusLogger::LOG_LEVEL_INFO, msgStream.str(), __FILE__, __LINE__); \
	msgStream.rdbuf()->freeze(); \
	}
	
#define LOG_DEBUG(msg) \
	{ \
	std::ostrstream msgStream; \
	msgStream << " " << msg << std::ends; \
	PlusLogger::Instance()->LogMessage(PlusLogger::LOG_LEVEL_DEBUG, msgStream.str(), __FILE__, __LINE__); \
	msgStream.rdbuf()->freeze(); \
	}	
	
#define LOG_TRACE(msg) \
	{ \
	std::ostrstream msgStream; \
	msgStream << " " << msg << std::ends; \
	PlusLogger::Instance()->LogMessage(PlusLogger::LOG_LEVEL_TRACE, msgStream.str(), __FILE__, __LINE__); \
	msgStream.rdbuf()->freeze(); \
	}	
	
class vtkConsoleOutputWindow : public vtkOutputWindow 
{ 
public: 
	static vtkConsoleOutputWindow* New() 
	{ return new vtkConsoleOutputWindow; } 
};

#define VTK_LOG_TO_CONSOLE_ON \
	{  \
	vtkSmartPointer<vtkConsoleOutputWindow> console = vtkSmartPointer<vtkConsoleOutputWindow>::New();  \
	vtkOutputWindow::SetInstance(console); \
	}

#define VTK_LOG_TO_CONSOLE_OFF \
	{  \
	vtkOutputWindow::SetInstance(NULL); \
	}	

/////////////////////////////////////////////////////////////////// 


#endif //__PlusCommon_h