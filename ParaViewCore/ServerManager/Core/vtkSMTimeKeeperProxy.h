/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMTimeKeeperProxy
 *
 * We simply pass the TimestepValues and TimeRange properties to the client-side
 * vtkSMTimeKeeper instance so that it can keep those up-to-date.
*/

#ifndef vtkSMTimeKeeperProxy_h
#define vtkSMTimeKeeperProxy_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMTimeKeeperProxy : public vtkSMProxy
{
public:
  static vtkSMTimeKeeperProxy* New();
  vtkTypeMacro(vtkSMTimeKeeperProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Track timesteps provided by a source. If \c suppress_input is true, before
   * adding the proxy, if the \r proxy has producers those will be removed from
   * the time sources i.e. we'll ignore timesteps from the input.
   */
  virtual bool AddTimeSource(vtkSMProxy* proxy, bool suppress_input);
  static bool AddTimeSource(vtkSMProxy* timeKeeper, vtkSMProxy* proxy, bool suppress_input)
  {
    vtkSMTimeKeeperProxy* self = vtkSMTimeKeeperProxy::SafeDownCast(timeKeeper);
    return self ? self->AddTimeSource(proxy, suppress_input) : false;
  }
  //@}

  //@{
  /**
   * Remove a particular time source.
   */
  virtual bool RemoveTimeSource(vtkSMProxy* proxy, bool unsuppress_input);
  static bool RemoveTimeSource(vtkSMProxy* timeKeeper, vtkSMProxy* proxy, bool unsuppress_input)
  {
    vtkSMTimeKeeperProxy* self = vtkSMTimeKeeperProxy::SafeDownCast(timeKeeper);
    return self ? self->RemoveTimeSource(proxy, unsuppress_input) : false;
  }
  //@}

  //@{
  /**
   * Returns true if the proxy has been added to time sources and not
   * suppressed.
   */
  virtual bool IsTimeSourceTracked(vtkSMProxy* proxy);
  static bool IsTimeSourceTracked(vtkSMProxy* timeKeeper, vtkSMProxy* proxy)
  {
    vtkSMTimeKeeperProxy* self = vtkSMTimeKeeperProxy::SafeDownCast(timeKeeper);
    return self ? self->IsTimeSourceTracked(proxy) : false;
  }
  //@}

  //@{
  /**
   * Set whether to suppress a time source that has been added to the time
   * keeper. Suppressing a source results in its time being ignored by the time
   * keeper.
   */
  virtual bool SetSuppressTimeSource(vtkSMProxy* proxy, bool suppress);
  static bool SetSuppressTimeSource(vtkSMProxy* timeKeeper, vtkSMProxy* proxy, bool suppress)
  {
    vtkSMTimeKeeperProxy* self = vtkSMTimeKeeperProxy::SafeDownCast(timeKeeper);
    return self ? self->SetSuppressTimeSource(proxy, suppress) : false;
  }
  //@}

  //@{
  /**
   * Returns a time value after snapping to a lower-bound in the current
   * timesteps.
   */
  virtual double GetLowerBoundTimeStep(double value);
  static double GetLowerBoundTimeStep(vtkSMProxy* timeKeeper, double value)
  {
    vtkSMTimeKeeperProxy* self = vtkSMTimeKeeperProxy::SafeDownCast(timeKeeper);
    return self ? self->GetLowerBoundTimeStep(value) : value;
  }
  //@}

  //@{
  /**
   * Returns the index for the lower bound of the time specified in current
   * timestep values, if possible. If there are no timestep values, returns 0.
   */
  virtual int GetLowerBoundTimeStepIndex(double value);
  static int GetLowerBoundTimeStepIndex(vtkSMProxy* timeKeeper, double value)
  {
    vtkSMTimeKeeperProxy* self = vtkSMTimeKeeperProxy::SafeDownCast(timeKeeper);
    return self ? self->GetLowerBoundTimeStepIndex(value) : 0;
  }
  //@}

protected:
  vtkSMTimeKeeperProxy();
  ~vtkSMTimeKeeperProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMTimeKeeperProxy(const vtkSMTimeKeeperProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMTimeKeeperProxy&) VTK_DELETE_FUNCTION;
};

#endif
