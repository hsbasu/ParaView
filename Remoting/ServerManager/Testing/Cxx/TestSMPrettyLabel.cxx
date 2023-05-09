/*=========================================================================

Program:   ParaView
Module:    TestSMPrettyLabel.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMProperty.h"

#include <cstring>
#include <iostream>

bool CheckStringEquivalent(const std::string& tested, const std::string& reference)
{
  if (tested != reference)
  {
    cerr << "Expected '" << reference << "' but got '" << tested << "'" << std::endl;
    return false;
  }
  return true;
}

int TestSMPrettyLabel(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel("MYSpace"), "MY Space"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel("MYSPACE"), "MYSPACE"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel("My Space"), "My Space"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel("MySPACE"), "My SPACE"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel("MySPace"), "My S Pace"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel("MySPACE"), "My SPACE"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel("MySpACE"), "My Sp ACE"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel("MYSuperSpacer"), "MY Super Spacer"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel("MySpACe"), "My Sp A Ce"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel("MySpAcE"), "My Sp Ac E"))
  {
    return EXIT_FAILURE;
  }

  if (!CheckStringEquivalent(vtkSMObject::CreatePrettyLabel(""), ""))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
