#include "FEAdaptor.h"
#include <iostream>

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

namespace
{
vtkCPProcessor* Processor = NULL;
vtkUnstructuredGrid* VTKGrid;

void BuildVTKGrid(unsigned int numberOfPoints, double* pointsData, unsigned int numberOfCells,
  unsigned int* cellsData)
{
  // create the points information
  vtkNew<vtkDoubleArray> pointArray;
  pointArray->SetNumberOfComponents(3);
  pointArray->SetArray(pointsData, static_cast<vtkIdType>(numberOfPoints * 3), 1);
  vtkNew<vtkPoints> points;
  points->SetData(pointArray.GetPointer());
  VTKGrid->SetPoints(points.GetPointer());

  // create the cells
  VTKGrid->Allocate(static_cast<vtkIdType>(numberOfCells * 9));
  for (unsigned int cell = 0; cell < numberOfCells; cell++)
  {
    unsigned int* cellPoints = cellsData + 8 * cell;
    vtkIdType tmp[8] = { cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3], cellPoints[4],
      cellPoints[5], cellPoints[6], cellPoints[7] };
    VTKGrid->InsertNextCell(VTK_HEXAHEDRON, 8, tmp);
  }
}

void UpdateVTKAttributes(unsigned int numberOfPoints, double* velocityData,
  unsigned int numberOfCells, float* pressureData)
{
  if (VTKGrid->GetPointData()->GetNumberOfArrays() == 0)
  {
    // velocity array
    vtkNew<vtkDoubleArray> velocity;
    velocity->SetName("velocity");
    velocity->SetNumberOfComponents(3);
    velocity->SetNumberOfTuples(static_cast<vtkIdType>(numberOfPoints));
    VTKGrid->GetPointData()->AddArray(velocity.GetPointer());
  }
  if (VTKGrid->GetCellData()->GetNumberOfArrays() == 0)
  {
    // pressure array
    vtkNew<vtkFloatArray> pressure;
    pressure->SetName("pressure");
    pressure->SetNumberOfComponents(1);
    VTKGrid->GetCellData()->AddArray(pressure.GetPointer());
  }
  vtkDoubleArray* velocity =
    vtkDoubleArray::SafeDownCast(VTKGrid->GetPointData()->GetArray("velocity"));
  // The velocity array is ordered as vx0,vx1,vx2,..,vy0,vy1,vy2,..,vz0,vz1,vz2,..
  // so we need to create a full copy of it with VTK's ordering of
  // vx0,vy0,vz0,vx1,vy1,vz1,..
  for (unsigned int i = 0; i < numberOfPoints; i++)
  {
    double values[3] = { velocityData[i], velocityData[i + numberOfPoints],
      velocityData[i + 2 * numberOfPoints] };
    velocity->SetTypedTuple(i, values);
  }

  vtkFloatArray* pressure =
    vtkFloatArray::SafeDownCast(VTKGrid->GetCellData()->GetArray("pressure"));
  // The pressure array is a scalar array so we can reuse
  // memory as long as we ordered the points properly.
  pressure->SetArray(pressureData, static_cast<vtkIdType>(numberOfCells), 1);
}

void BuildVTKDataStructures(unsigned int numberOfPoints, double* points, double* velocity,
  unsigned int numberOfCells, unsigned int* cells, float* pressure)
{
  if (VTKGrid == NULL)
  {
    // The grid structure isn't changing so we only build it
    // the first time it's needed. If we needed the memory
    // we could delete it and rebuild as necessary.
    VTKGrid = vtkUnstructuredGrid::New();
    BuildVTKGrid(numberOfPoints, points, numberOfCells, cells);
  }
  UpdateVTKAttributes(numberOfPoints, velocity, numberOfCells, pressure);
}
}

void CatalystInitialize(int numScripts, char* scripts[])
{
  if (Processor == NULL)
  {
    Processor = vtkCPProcessor::New();
    Processor->Initialize();
  }
  else
  {
    Processor->RemoveAllPipelines();
  }
  for (int i = 1; i < numScripts; i++)
  {
    vtkNew<vtkCPPythonScriptPipeline> pipeline;
    pipeline->Initialize(scripts[i]);
    Processor->AddPipeline(pipeline.GetPointer());
  }
}

void CatalystFinalize()
{
  if (Processor)
  {
    Processor->Delete();
    Processor = NULL;
  }
  if (VTKGrid)
  {
    VTKGrid->Delete();
    VTKGrid = NULL;
  }
}

void CatalystCoProcess(unsigned int numberOfPoints, double* pointsData, unsigned int numberOfCells,
  unsigned int* cellsData, double* velocityData, float* pressureData, double time,
  unsigned int timeStep, int lastTimeStep)
{
  vtkNew<vtkCPDataDescription> dataDescription;
  dataDescription->AddInput("input");
  dataDescription->SetTimeData(time, timeStep);
  if (lastTimeStep == true)
  {
    // assume that we want to all the pipelines to execute if it
    // is the last time step.
    dataDescription->ForceOutputOn();
  }
  if (Processor->RequestDataDescription(dataDescription.GetPointer()) != 0)
  {
    BuildVTKDataStructures(
      numberOfPoints, pointsData, velocityData, numberOfCells, cellsData, pressureData);
    dataDescription->GetInputDescriptionByName("input")->SetGrid(VTKGrid);
    Processor->CoProcess(dataDescription.GetPointer());
  }
}
