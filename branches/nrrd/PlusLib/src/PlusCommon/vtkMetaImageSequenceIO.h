/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/

#ifndef __vtkMetaImageSequenceIO_h
#define __vtkMetaImageSequenceIO_h

#include "vtkPlusCommonExport.h"

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif 

#include "vtkSequenceIOBase.h"

class vtkTrackedFrameList;

/*!
  \class vtkMetaImageSequenceIO
  \brief Read and write MetaImage file with a sequence of frames, with additional information for each frame
  \ingroup PlusLibCommon
*/
class vtkPlusCommonExport vtkMetaImageSequenceIO : public vtkSequenceIOBase
{
public:
  static vtkMetaImageSequenceIO *New();
  vtkTypeMacro(vtkMetaImageSequenceIO, vtkSequenceIOBase);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  /*! Accessors to control 2D Dims output */
  vtkSetMacro(Output2DDataWithZDimensionIncluded, bool);
  vtkGetMacro(Output2DDataWithZDimensionIncluded, bool);

  /*! Prepare the sequence for writing */
  virtual PlusStatus PrepareHeader();

  /*! Update the number of frames in the header 
      This is used primarly by vtkVirtualDiscCapture to update the final tally of frames, as it continually appends new frames to the file
      /param numberOfFrames the new number of frames to write
      /param addPadding this should only be true if this is the first time this function is called, which typically happens in OpenImageHeader
  */
  virtual PlusStatus OverwriteNumberOfFramesInHeader(int numberOfFrames, bool addPadding=false);

  /*! 
    Append the frames in tracked frame list to the header, if the onlyTrackerData flag is true it will not save
    in the header the image data related fields. 
  */
  virtual PlusStatus AppendImagesToHeader();

  /*! Finalize the header */
  virtual PlusStatus FinalizeHeader();

  /*! Write images to disc, compression allowed */
  virtual PlusStatus WriteImages();

  /*! Close the sequence */
  virtual PlusStatus Close();

  /*! Check if this class can read the specified file */
  static bool CanReadFile(const std::string& filename);

  /*! Check if this class can write the specified file */
  static bool CanWriteFile(const std::string& filename);

  /*! Update a field in the image header with its current value */
  virtual PlusStatus UpdateFieldInImageHeader(const char* fieldName);

  /*! Return the string that represents the dimensional sizes */
  virtual const char* GetDimensionSizeString();

  /*!
    Set input/output file name. The file contains only the image header in case of
    MHD images and the full image (including pixel data) in case of MHA images.
  */
  virtual PlusStatus SetFileName(const std::string& aFilename);

protected:
  vtkMetaImageSequenceIO();
  virtual ~vtkMetaImageSequenceIO();

  /*! Read all the fields in the metaimage file header */
  virtual PlusStatus ReadImageHeader();

  /*! Read pixel data from the metaimage */
  virtual PlusStatus ReadImagePixels();

  /*! Write all the fields to the metaimage file header */
  virtual PlusStatus OpenImageHeader();

  /*! Write pixel data to the metaimage */
  virtual PlusStatus WriteImagePixels(const std::string& aFilename, bool forceAppend = false);

  /*! Conversion between ITK and METAIO pixel types */
  PlusStatus ConvertMetaElementTypeToVtkPixelType(const std::string &elementTypeStr, PlusCommon::VTKScalarPixelType &vtkPixelType);
  /*! Conversion between ITK and METAIO pixel types */
  PlusStatus ConvertVtkPixelTypeToMetaElementType(PlusCommon::VTKScalarPixelType vtkPixelType, std::string &elementTypeStr);

  /*! 
    Writes the compressed pixel data directly into file. 
    The compression is performed in chunks, so no excessive memory is used for the compression.
    \param outputFileStream the file stream where the compressed pixel data will be written to
    \param compressedDataSize returns the size of the total compressed data that is written to the file.
  */
  virtual PlusStatus WriteCompressedImagePixelsToFile(FILE *outputFileStream, int &compressedDataSize);

private:
  /*! ASCII or binary */
  bool IsPixelDataBinary;
  /*! If 2D data, boolean to determine if we should write out in the form X Y Nfr (false) or X Y 1 Nfr (true) */
  bool Output2DDataWithZDimensionIncluded;

protected:
  vtkMetaImageSequenceIO(const vtkMetaImageSequenceIO&); //purposely not implemented
  void operator=(const vtkMetaImageSequenceIO&); //purposely not implemented
};

#endif // __vtkMetaImageSequenceIO_h 