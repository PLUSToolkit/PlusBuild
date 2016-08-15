/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/ 

#include "PlusConfigure.h"
#include "PlusTrackedFrame.h"
#include "vtkPlus3DObjectVisualizer.h"
#include "vtkPlusDisplayableObject.h"
#include "vtkGlyph3D.h"
#include "vtkImageSliceMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPlusChannel.h"
#include "vtkPlusDevice.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlus3DObjectVisualizer);
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

vtkPlus3DObjectVisualizer::vtkPlus3DObjectVisualizer()
: CanvasRenderer(NULL)
, ImageActor(NULL)
, InputActor(NULL)
, InputGlyph(NULL)
, ResultActor(NULL)
, ResultGlyph(NULL)
, TransformRepository(NULL)
, WorldCoordinateFrame(NULL)
, VolumeID(NULL)
, SelectedChannel(NULL)
{
  // Set up canvas renderer
  vtkSmartPointer<vtkRenderer> canvasRenderer = vtkSmartPointer<vtkRenderer>::New(); 
  canvasRenderer->SetBackground(0.1, 0.1, 0.1);
  canvasRenderer->SetBackground2(0.4, 0.4, 0.4);
  canvasRenderer->SetGradientBackground(true);
  this->SetCanvasRenderer(canvasRenderer);

  // Input points actor
  vtkSmartPointer<vtkActor> inputActor = vtkSmartPointer<vtkActor>::New();
  this->SetInputActor(inputActor);
  vtkSmartPointer<vtkPolyDataMapper> inputMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkGlyph3D> inputGlyph = vtkSmartPointer<vtkGlyph3D>::New();
  this->SetInputGlyph(inputGlyph);
  vtkSmartPointer<vtkSphereSource> inputSphereSource = vtkSmartPointer<vtkSphereSource>::New();
  inputSphereSource->SetRadius(2.0); // mm

  // Connect all input items (except poly data) in chain
  this->InputGlyph->SetSourceConnection(inputSphereSource->GetOutputPort());  
  inputMapper->SetInputConnection(this->InputGlyph->GetOutputPort());
  this->InputActor->SetMapper(inputMapper);
  this->InputActor->GetProperty()->SetColor(0.0, 0.7, 1.0);

  // Result points actor
  vtkSmartPointer<vtkActor> resultActor = vtkSmartPointer<vtkActor>::New();
  this->SetResultActor(resultActor);
  vtkSmartPointer<vtkPolyDataMapper> resultMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkGlyph3D> resultGlyph = vtkSmartPointer<vtkGlyph3D>::New();
  this->SetResultGlyph(resultGlyph);
  vtkSmartPointer<vtkSphereSource> resultSphereSource = vtkSmartPointer<vtkSphereSource>::New();
  resultSphereSource->SetRadius(1.0); // mm

  // Connect all result items (except poly data) in chain
  this->ResultGlyph->SetSourceConnection(resultSphereSource->GetOutputPort());
  resultMapper->SetInputConnection(resultGlyph->GetOutputPort());
  this->ResultActor->SetMapper(resultMapper);
  this->ResultActor->GetProperty()->SetColor(0.0, 0.8, 0.0);

  // Create image actor
  vtkSmartPointer<vtkImageActor> imageActor = vtkSmartPointer<vtkImageActor>::New();
  this->SetImageActor(imageActor);

  this->ImageMapper = vtkImageSliceMapper::SafeDownCast(this->ImageActor->GetMapper());

  this->CanvasRenderer->AddActor(this->InputActor);
  this->CanvasRenderer->AddActor(this->ResultActor);
}

//-----------------------------------------------------------------------------

vtkPlus3DObjectVisualizer::~vtkPlus3DObjectVisualizer()
{
  ClearDisplayableObjects();

  this->SetResultActor(NULL);
  this->SetCanvasRenderer(NULL);
  this->SetImageActor(NULL);
  this->SetInputActor(NULL);
  this->SetWorldCoordinateFrame(NULL);
  this->SetTransformRepository(NULL);
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::Update()
{
  // If none of the objects are displayable then return with fail
  bool noObjectsToDisplay = true;
  for (std::vector<vtkPlusDisplayableObject*>::iterator it = this->DisplayableObjects.begin(); it != this->DisplayableObjects.end(); ++it)
  {
    vtkPlusDisplayableObject* displayableObject = *it;
    if ( displayableObject->IsDisplayable() && displayableObject->GetActor() && displayableObject->GetActor()->GetVisibility() > 0 )
    {
      noObjectsToDisplay = false;
      break;
    }
  }

  if (noObjectsToDisplay)
  {
    return PLUS_SUCCESS;
  }

  // Get tracked frame and set the transforms
  PlusTrackedFrame trackedFrame; 
  if ( this->SelectedChannel->GetTrackedFrame(trackedFrame) != PLUS_SUCCESS )
  {
    LOG_ERROR("Failed to get tracked frame!"); 
    return PLUS_FAIL; 
  }

  if( this->TransformRepository == NULL )
  {
    // With new member initialization flexibility, this is an acceptable fail... return PLUS_SUCCESS?
    //LOG_WARNING("Update called with no transform repository.");
    return PLUS_FAIL;
  }
  if ( this->TransformRepository->SetTransforms(trackedFrame) != PLUS_SUCCESS )
  {
    LOG_ERROR("Failed to set current transforms to transform repository!"); 
    return PLUS_FAIL; 
  }

  bool resetCameraNeeded = false;

  // Update actors of displayable objects
  for (std::vector<vtkPlusDisplayableObject*>::iterator it = this->DisplayableObjects.begin(); it != this->DisplayableObjects.end(); ++it)
  {
    vtkPlusDisplayableObject* displayableObject = *it;
    PlusTransformName objectCoordinateFrameToWorldTransformName(displayableObject->GetObjectCoordinateFrame(), this->WorldCoordinateFrame);
    vtkSmartPointer<vtkMatrix4x4> objectCoordinateFrameToWorldTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();

    // If not displayable or valid transform does not exist then hide
    if ( (displayableObject->IsDisplayable() == false)
      || (this->TransformRepository->IsExistingTransform(objectCoordinateFrameToWorldTransformName) != PLUS_SUCCESS) )
    {
      if (displayableObject->GetActor())
      {
        displayableObject->GetActor()->VisibilityOff();
      }
      continue;
    }

    // Get object to world transform
    bool valid = false;
    if ( this->TransformRepository->GetTransform(objectCoordinateFrameToWorldTransformName, objectCoordinateFrameToWorldTransformMatrix, &valid) != PLUS_SUCCESS )
    {
      LOG_ERROR("Failed to get transform from object (" << displayableObject->GetObjectCoordinateFrame() << ") to world! (" << this->WorldCoordinateFrame << ")");
      continue;
    }

    // If the transform is valid then display it normally
    if (valid)
    {
      // If opacity was 0.0, then this is the first visualization iteration after switching back from image mode - reset opacity and camera is needed
      // In case of 0.3 it was previously out of view, same opacity and camera reset is needed
      if (fabs(displayableObject->GetOpacity()) < 0.001 || fabs(displayableObject->GetOpacity() - 0.3) < 0.001)
      {
        displayableObject->SetOpacity( displayableObject->GetLastOpacity() );
        resetCameraNeeded = true;
      }

      // Assemble and set transform for visualization
      vtkSmartPointer<vtkTransform> objectModelToWorldTransform = vtkSmartPointer<vtkTransform>::New();
      objectModelToWorldTransform->Identity();
      objectModelToWorldTransform->Concatenate(objectCoordinateFrameToWorldTransformMatrix);

      vtkDisplayableModel* displayableModel = dynamic_cast<vtkDisplayableModel*>(displayableObject);
      if (displayableModel)
      {
        objectModelToWorldTransform->Concatenate(displayableModel->GetModelToObjectTransform());
      }
      objectModelToWorldTransform->Modified();

      displayableObject->GetActor()->SetUserTransform(objectModelToWorldTransform);
    }
    // If invalid then make it partially transparent and leave in place
    else
    {
      if (fabs(displayableObject->GetOpacity()) > 0.001 && fabs(displayableObject->GetOpacity() - 0.3) > 0.001)
      {
        displayableObject->SetLastOpacity( displayableObject->GetOpacity() );
      }
      displayableObject->SetOpacity( 0.3 );
    }
  } // for all displayable objects

  if (resetCameraNeeded)
  {
    this->CanvasRenderer->ResetCamera();
  }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::ClearDisplayableObjects()
{
  LOG_TRACE("vtkPerspectiveVisualizer::ClearDisplayableObjects");

  for (std::vector<vtkPlusDisplayableObject*>::iterator it = this->DisplayableObjects.begin(); it != this->DisplayableObjects.end(); ++it)
  {
    vtkPlusDisplayableObject* tool = *it;
    if (tool != NULL)
    {
      if (tool->GetActor() != NULL)
      {
        this->CanvasRenderer->RemoveActor(tool->GetActor());
      }

      tool->Delete();
      tool = NULL;
    }
  }

  this->DisplayableObjects.clear();

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::ShowAllObjects(bool aOn)
{
  LOG_TRACE("vtkPerspectiveVisualizer::ShowAllObjects(" << (aOn?"true":"false") << ")");

  for (std::vector<vtkPlusDisplayableObject*>::iterator it = this->DisplayableObjects.begin(); it != this->DisplayableObjects.end(); ++it)
  {
    vtkPlusDisplayableObject* displayableObject = *it;
    if ((displayableObject != NULL) && (displayableObject->GetActor() != NULL))
    {
      displayableObject->GetActor()->SetVisibility(aOn);
    }
  }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::ShowInput(bool aOn)
{
  this->InputActor->SetVisibility(aOn);
  this->CanvasRenderer->Modified();

  return PLUS_SUCCESS;
}
//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::SetInputColor(double r, double g, double b)
{
  this->InputActor->GetProperty()->SetColor(r, g, b);
  this->CanvasRenderer->Modified();

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::ShowResult(bool aOn)
{
  this->ResultActor->SetVisibility(aOn);
  this->CanvasRenderer->Modified();

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::HideAll()
{
  this->InputActor->VisibilityOff();
  this->ResultActor->VisibilityOff();
  this->ImageActor->VisibilityOff();

  ShowAllObjects(false);

  this->CanvasRenderer->Modified();

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------
void vtkPlus3DObjectVisualizer::SetInputPolyData(vtkPolyData* aPolyData)
{
  this->InputGlyph->SetInputData_vtk5compatible(aPolyData);
}

//-----------------------------------------------------------------------------
void vtkPlus3DObjectVisualizer::SetResultPolyData(vtkPolyData* aPolyData)
{
  this->ResultGlyph->SetInputData_vtk5compatible(aPolyData);
}


//-----------------------------------------------------------------------------
PlusStatus vtkPlus3DObjectVisualizer::SetChannel(vtkPlusChannel* channel)
{
  LOG_TRACE("vtkPlus3DObjectVisualizer::SetChannel");
  SetSelectedChannel(channel);

  if( this->SelectedChannel != NULL )
  {
    if (this->SelectedChannel->GetOwnerDevice()->GetConnected() == false)
    {
      LOG_ERROR("Data collection not initialized or device visualization cannot be initialized unless they are connected");
      return PLUS_FAIL;
    }

    // Connect data collector to image actor
    if (this->SelectedChannel->GetVideoDataAvailable())
    {
      this->ImageActor->VisibilityOn();
      this->ImageActor->SetInputData_vtk5compatible( this->SelectedChannel->GetBrightnessOutput() );
    }
    else
    {
      LOG_DEBUG("No video in the selected channel. Hiding image visualization.");
      this->ImageActor->VisibilityOff();
    }
  }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::ReadConfiguration(vtkXMLDataElement* aXMLElement)
{
  // Rendering section
  vtkXMLDataElement* renderingElement = aXMLElement->FindNestedElementWithName("Rendering"); 

  if (renderingElement == NULL)
  {
    LOG_ERROR("Unable to find Rendering element in XML tree!"); 
    return PLUS_FAIL;     
  }

  // World coordinate frame name
  const char* worldCoordinateFrame = renderingElement->GetAttribute("WorldCoordinateFrame");
  if (worldCoordinateFrame == NULL)
  {
    LOG_ERROR("WorldCoordinateFrame is not specified in DisplayableTool element of the configuration!");
    return PLUS_FAIL;     
  }

  this->SetWorldCoordinateFrame(worldCoordinateFrame);

  // Read displayable tool configurations
  bool imageFound = false;
  for ( int i = 0; i < renderingElement->GetNumberOfNestedElements(); ++i )
  {
    vtkXMLDataElement* displayableObjectElement = renderingElement->GetNestedElement(i); 
    if ( STRCASECMP(displayableObjectElement->GetName(), "DisplayableObject") != 0 )
    {
      // if this is not a DisplayableObject element, skip it
      continue; 
    }

    // Get type
    const char* type = displayableObjectElement->GetAttribute("Type");
    if (type == NULL)
    {
      LOG_ERROR("No type found for displayable object!");
      continue;
    }

    // Create displayable tool
    vtkPlusDisplayableObject* displayableObject = vtkPlusDisplayableObject::New(type);

    // Image has a special actor, set it now in the displayable object
    if (STRCASECMP(type, "Image") == 0)
    {
      displayableObject->SetActor(this->ImageActor);
      imageFound = true;
    }

    // Read configuration
    if (displayableObject->ReadConfiguration(displayableObjectElement) != PLUS_SUCCESS)
    {
      LOG_ERROR("Unable to read displayable tool configuration!");
      continue;
    }

    this->DisplayableObjects.push_back(displayableObject);
  }

  if (this->DisplayableObjects.size() == 0)
  {
    LOG_ERROR("No displayable objects found!");
    return PLUS_FAIL;
  }

  if (!imageFound)
  {
    LOG_INFO("No image found in the displayable object list, it will not be displayed!");
  }

  // Add displayable object actors to renderer
  for (std::vector<vtkPlusDisplayableObject*>::iterator it = this->DisplayableObjects.begin(); it != this->DisplayableObjects.end(); ++it)
  {
    vtkPlusDisplayableObject* displayableObject = *it;
    if (displayableObject == NULL)
    {
      LOG_ERROR("Invalid displayable object!");
      continue;
    }

    this->CanvasRenderer->AddActor(displayableObject->GetActor());
  }

  // Rendering section
  vtkXMLDataElement* fCalElement = aXMLElement->FindNestedElementWithName("fCal"); 

  if (fCalElement == NULL)
  {
    return PLUS_SUCCESS;     
  }

  const char* volumeId = fCalElement->GetAttribute("ReconstructedVolumeId");
  if (volumeId == NULL)
  {
    LOG_WARNING("Volume displayable object ID not defined in fCal. Unable to update volume object.");
    return PLUS_SUCCESS;     
  }

  this->SetVolumeID(volumeId);

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::ShowObjectById( const char* aModelId, bool aOn )
{
  LOG_TRACE("vtkPlus3DObjectVisualizer::ShowObjectById(" << aModelId << ", " << (aOn?"true":"false") << ")");

  for (std::vector<vtkPlusDisplayableObject*>::iterator it = this->DisplayableObjects.begin(); it != this->DisplayableObjects.end(); ++it)
  {
    vtkPlusDisplayableObject* displayableObject = *it;

    if( displayableObject->GetObjectId() != NULL && STRCASECMP(displayableObject->GetObjectId(), aModelId) == 0 )
    {
      displayableObject->GetActor()->SetVisibility(aOn);
    }
  }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

vtkPlusDisplayableObject* vtkPlus3DObjectVisualizer::GetObjectById( const char* aModelId )
{
  if (aModelId==NULL)
  {
    return NULL;
  }

  LOG_TRACE("vtkPlus3DObjectVisualizer::GetObjectById(" << aModelId << ")");

  for (std::vector<vtkPlusDisplayableObject*>::iterator it = this->DisplayableObjects.begin(); it != this->DisplayableObjects.end(); ++it)
  {
    vtkPlusDisplayableObject* displayableObject = *it;

    if( displayableObject->GetObjectId() != NULL && STRCASECMP(displayableObject->GetObjectId(), aModelId) == 0 )
    {
      return displayableObject;
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::AddObject( vtkPlusDisplayableObject* displayableObject )
{
  LOG_TRACE("vtkPlus3DObjectVisualizer::AddObject");

  if (displayableObject==NULL || displayableObject->GetObjectId()==NULL)
  {
    LOG_ERROR("vtkPlus3DObjectVisualizer::AddObject failed: invalid input object")
    return PLUS_FAIL;
  }
  if (GetObjectById(displayableObject->GetObjectId())!=NULL)
  {
    LOG_ERROR("vtkPlus3DObjectVisualizer::AddObject failed: object with name "<<displayableObject->GetObjectId()<<" already exists")
    return PLUS_FAIL;
  }
  if (displayableObject->GetActor()==NULL)
  {
    LOG_ERROR("vtkPlus3DObjectVisualizer::AddObject failed: object with name "<<displayableObject->GetObjectId()<<" does not have a valid actor")
    return PLUS_FAIL;
  }

  this->DisplayableObjects.push_back(displayableObject);
  displayableObject->Register(this);
  this->CanvasRenderer->AddActor(displayableObject->GetActor());

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::SetVolumeColor( double r, double g, double b )
{
  LOG_TRACE("vtkPlus3DObjectVisualizer::SetVolumeColor(" << r << ", " << g << ", " << b << ")");

  if( this->GetVolumeActor() )
  {
    this->GetVolumeActor()->GetProperty()->SetColor(r, g, b);
    return PLUS_SUCCESS;
  }

  return PLUS_FAIL;
}

//-----------------------------------------------------------------------------

PlusStatus vtkPlus3DObjectVisualizer::SetVolumeMapper( vtkPolyDataMapper* aContourMapper )
{
  LOG_TRACE("vtkPlus3DObjectVisualizer::SetVolumeMapper(...)");

  if( this->GetVolumeID() == NULL )
  {
    LOG_ERROR("Trying to update volume data without volume displayable object defined.");
    return PLUS_FAIL;
  }

  vtkPlusDisplayableObject* disObj = this->GetObjectById(this->GetVolumeID());
  if( disObj )
  {
    vtkDisplayablePolyData* polyObj = dynamic_cast<vtkDisplayablePolyData*>(disObj);

    if( polyObj )
    {
      polyObj->SetPolyDataMapper(aContourMapper);
      return PLUS_SUCCESS;
    }
  }

  return PLUS_FAIL;
}

//-----------------------------------------------------------------------------

vtkActor* vtkPlus3DObjectVisualizer::GetVolumeActor()
{
  LOG_TRACE("vtkPlus3DObjectVisualizer::SetVolumeMapper(...)");

  if( this->GetVolumeID() == NULL )
  {
    LOG_ERROR("Trying to retrieve volume actor without volume displayable object defined.");
    return NULL;
  }

  vtkPlusDisplayableObject* disObj = this->GetObjectById(this->GetVolumeID());
  if( disObj )
  {
    vtkActor* actor = dynamic_cast<vtkActor*>(disObj->GetActor());
    if(actor)
    {
      return actor;
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------
PlusStatus vtkPlus3DObjectVisualizer::SetSliceNumber(int number)
{
  if( number >= this->ImageMapper->GetSliceNumberMinValue() && number <= this->ImageMapper->GetSliceNumberMaxValue() )
  {
    this->ImageMapper->SetSliceNumber(number);
    return PLUS_SUCCESS;
  }
  return PLUS_FAIL;
}
