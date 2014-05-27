/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/ 

#include "PlusConfigure.h"
#include "vtkPlusStartDataCollectionCommand.h"
#include "igtlStringMessage1.h"

vtkCxxRevisionMacro( vtkPlusStartDataCollectionCommand, "$Revision: 1.0 $" );
vtkStandardNewMacro( vtkPlusStartDataCollectionCommand ); 

//----------------------------------------------------------------------------
void vtkPlusStartDataCollectionCommand::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//----------------------------------------------------------------------------
bool vtkPlusStartDataCollectionCommand::CanExecute( std::string str )
{
  if ( str.compare( this->StringRepresentation ) == 0 )
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------  
bool vtkPlusStartDataCollectionCommand::Execute()
{
  PlusStatus status = this->DataCollector->Start();
  if ( status == PLUS_FAIL )
  {
    LOG_WARNING( "Data collector could not be started." );
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
std::string vtkPlusStartDataCollectionCommand::GetStringRepresentation()
{
  return this->StringRepresentation;
}

//----------------------------------------------------------------------------
vtkPlusStartDataCollectionCommand::vtkPlusStartDataCollectionCommand()
{
  this->StringRepresentation = "StartDataCollection";
  this->AddSupportedCommandString( this->StringRepresentation );
}

//----------------------------------------------------------------------------
vtkPlusStartDataCollectionCommand::~vtkPlusStartDataCollectionCommand()
{

}
