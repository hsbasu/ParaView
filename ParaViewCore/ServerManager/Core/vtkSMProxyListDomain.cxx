/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyListDomain.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <string>
#include <vector>

namespace vtkSMProxyListDomainNS
{
//---------------------------------------------------------------------------
class vtkSMLinkObserver : public vtkCommand
{
public:
  vtkWeakPointer<vtkSMProperty> Output;
  typedef vtkCommand Superclass;
  virtual const char* GetClassNameInternal() const { return "vtkSMLinkObserver"; }
  static vtkSMLinkObserver* New() { return new vtkSMLinkObserver(); }
  virtual void Execute(vtkObject* caller, unsigned long event, void* calldata)
  {
    (void)event;
    (void)calldata;
    vtkSMProperty* input = vtkSMProperty::SafeDownCast(caller);
    if (input && this->Output)
    {
      // this will copy both checked and unchecked property values.
      this->Output->Copy(input);
    }
  }
};
}

//-----------------------------------------------------------------------------
class vtkSMProxyInfo
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;
  std::vector<unsigned long> ObserverIds;

  // Proxies in proxy-list domains can have hints that are used to setup
  // property-links to ensure that those proxies get appropriate domains.
  void ProcessProxyListProxyHints(vtkSMProxy* parent)
  {
    vtkSMProxy* plproxy = this->Proxy;
    vtkPVXMLElement* proxyListElement =
      plproxy->GetHints() ? plproxy->GetHints()->FindNestedElementByName("ProxyList") : NULL;
    if (!proxyListElement)
    {
      return;
    }
    for (unsigned int cc = 0, max = proxyListElement->GetNumberOfNestedElements(); cc < max; ++cc)
    {
      vtkPVXMLElement* child = proxyListElement->GetNestedElement(cc);
      if (child && child->GetName() && strcmp(child->GetName(), "Link") == 0)
      {
        const char* name = child->GetAttribute("name");
        const char* linked_with = child->GetAttribute("with_property");
        if (name && linked_with)
        {
          vtkSMProperty* input = parent->GetProperty(linked_with);
          vtkSMProperty* output = plproxy->GetProperty(name);
          if (input && output)
          {
            vtkSMProxyListDomainNS::vtkSMLinkObserver* observer =
              vtkSMProxyListDomainNS::vtkSMLinkObserver::New();
            observer->Output = output;
            this->ObserverIds.push_back(
              input->AddObserver(vtkCommand::PropertyModifiedEvent, observer));
            this->ObserverIds.push_back(
              input->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent, observer));
            observer->FastDelete();
            output->Copy(input);
          }
        }
      }
    }
  }
  void RemoveObservers()
  {
    for (size_t cc = 0; cc < this->ObserverIds.size(); cc++)
    {
      this->Proxy->RemoveObserver(this->ObserverIds[cc]);
    }
    this->ObserverIds.clear();
  }
};

class vtkSMProxyListDomainInternals
{
public:
  typedef std::vector<vtkSMProxyInfo> VectorOfProxies;
  const VectorOfProxies& GetProxies() const { return this->ProxyList; }

  // Add a proxy to the internal Proxies list. This will also process any "ProxyList"
  // hints specified on the \c proxy and setup property links with the parent-proxy of
  // self.
  void AddProxy(vtkSMProxy* proxy, vtkSMProxyListDomain* self)
  {
    vtkSMProxyInfo info;
    info.Proxy = proxy;

    vtkSMProxy* parent = self->GetProperty() ? self->GetProperty()->GetParent() : NULL;
    if (parent)
    {
      // Handle proxy-list hints.
      info.ProcessProxyListProxyHints(parent);
    }
    this->ProxyList.push_back(info);
  }

  // Removes a proxy from the Proxies list. Will also cleanup any "links" setup
  // during AddProxy().
  bool RemoveProxy(vtkSMProxy* proxy)
  {
    VectorOfProxies::iterator iter;
    for (iter = this->ProxyList.begin(); iter != this->ProxyList.end(); iter++)
    {
      if (iter->Proxy == proxy)
      {
        iter->RemoveObservers();
        this->ProxyList.erase(iter);
        return true;
      }
    }
    return false;
  }

  // Removes a proxy from the Proxies list. Will also cleanup any "links" setup
  // during AddProxy().
  bool RemoveProxy(unsigned int index)
  {
    if (index >= this->ProxyList.size())
    {
      return false;
    }

    VectorOfProxies::iterator iter;
    unsigned int cc = 0;
    for (iter = this->ProxyList.begin(); iter != this->ProxyList.end(); ++iter, ++cc)
    {
      if (cc == index)
      {
        iter->RemoveObservers();
        this->ProxyList.erase(iter);
        return true;
      }
    }
    return false;
  }

  // Removes all proxies from the Proxies list. Will also cleanup any "links" setup
  // during AddProxy() calls.
  void ClearProxies()
  {
    VectorOfProxies::iterator iter;
    for (iter = this->ProxyList.begin(); iter != this->ProxyList.end(); ++iter)
    {
      iter->RemoveObservers();
    }
    this->ProxyList.clear();
  }

  ~vtkSMProxyListDomainInternals() { this->ClearProxies(); }

public:
  struct ProxyInfo
  {
    std::string GroupName;
    std::string ProxyName;
  };
  typedef std::vector<ProxyInfo> VectorOfProxyInfo;
  VectorOfProxyInfo ProxyTypeList;

private:
  VectorOfProxies ProxyList;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMProxyListDomain);
//-----------------------------------------------------------------------------
vtkSMProxyListDomain::vtkSMProxyListDomain()
{
  this->Internals = new vtkSMProxyListDomainInternals;
}

//-----------------------------------------------------------------------------
vtkSMProxyListDomain::~vtkSMProxyListDomain()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::CreateProxies(vtkSMSessionProxyManager* pxm)
{
  assert(pxm);

  this->Internals->ClearProxies();

  for (vtkSMProxyListDomainInternals::VectorOfProxyInfo::iterator iter =
         this->Internals->ProxyTypeList.begin();
       iter != this->Internals->ProxyTypeList.end(); ++iter)
  {
    vtkSMProxy* proxy = pxm->NewProxy(iter->GroupName.c_str(), iter->ProxyName.c_str());
    if (proxy)
    {
      this->Internals->AddProxy(proxy, this);
      proxy->FastDelete();
    }
  }
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::IsInDomain(vtkSMProperty* vtkNotUsed(property))
{
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::AddProxy(const char* group, const char* name)
{
  vtkSMProxyListDomainInternals::ProxyInfo info;
  info.GroupName = group;
  info.ProxyName = name;
  this->Internals->ProxyTypeList.push_back(info);
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxyListDomain::GetNumberOfProxyTypes()
{
  return static_cast<unsigned int>(this->Internals->ProxyTypeList.size());
}

//-----------------------------------------------------------------------------
const char* vtkSMProxyListDomain::GetProxyGroup(unsigned int cc)
{
  if (this->GetNumberOfProxyTypes() <= cc)
  {
    vtkErrorMacro("Invalid index " << cc);
    return NULL;
  }

  return this->Internals->ProxyTypeList[cc].GroupName.c_str();
}

//-----------------------------------------------------------------------------
const char* vtkSMProxyListDomain::GetProxyName(unsigned int cc)
{
  if (this->GetNumberOfProxyTypes() <= cc)
  {
    vtkErrorMacro("Invalid index " << cc);
    return NULL;
  }

  return this->Internals->ProxyTypeList[cc].ProxyName.c_str();
}

//-----------------------------------------------------------------------------
const char* vtkSMProxyListDomain::GetProxyName(vtkSMProxy* proxy)
{
  return proxy ? proxy->GetXMLName() : NULL;
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(prop);
  if (pp && this->GetNumberOfProxies() > 0)
  {
    vtkSMPropertyHelper helper(prop);
    helper.SetUseUnchecked(use_unchecked_values);
    vtkSMProxy* values[1] = { this->GetProxy(0) };
    helper.Set(values, 1);
    return 1;
  }

  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  int found = 0;
  unsigned int max = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < max; ++cc)
  {
    vtkPVXMLElement* proxyElement = element->GetNestedElement(cc);
    if (strcmp(proxyElement->GetName(), "Proxy") == 0)
    {
      const char* name = proxyElement->GetAttribute("name");
      const char* group = proxyElement->GetAttribute("group");
      if (name && group)
      {
        this->AddProxy(group, name);
        found = 1;
      }
    }
    else if (strcmp(proxyElement->GetName(), "Group") == 0)
    {
      // Recover group name
      const char* name = proxyElement->GetAttribute("name");

      if (name)
      {
        found = 1;

        // Browse group and recover each proxy type
        vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
        vtkSMProxyDefinitionManager* pxdm = pxm->GetProxyDefinitionManager();
        if (!pxdm)
        {
          vtkErrorMacro("No vtkSMProxyDefinitionManager available in vtkSMSessionProxyManager, "
                        "cannot generate proxy list for groups");
          continue;
        }
        else
        {
          vtkPVProxyDefinitionIterator* iter = pxdm->NewSingleGroupIterator(name);
          for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
          {
            this->AddProxy(name, iter->GetProxyName());
          }
          iter->Delete();
        }
      }
    }
  }
  if (!found)
  {
    vtkErrorMacro("Required element \"Proxy\" (with a 'name' and 'group' attribute) "
                  " or \"Group\" ( with a name attribute ) was not found.");
    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::AddProxy(vtkSMProxy* proxy)
{
  this->Internals->AddProxy(proxy, this);
  this->DomainModified();
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxyListDomain::GetNumberOfProxies()
{
  return static_cast<unsigned int>(this->Internals->GetProxies().size());
}

//-----------------------------------------------------------------------------
bool vtkSMProxyListDomain::HasProxy(vtkSMProxy* proxy)
{
  const vtkSMProxyListDomainInternals::VectorOfProxies& proxies = this->Internals->GetProxies();
  vtkSMProxyListDomainInternals::VectorOfProxies::const_iterator iter;
  for (iter = proxies.begin(); iter != proxies.end(); iter++)
  {
    if (iter->Proxy == proxy)
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyListDomain::GetProxy(unsigned int index)
{
  const vtkSMProxyListDomainInternals::VectorOfProxies& proxies = this->Internals->GetProxies();
  if (index >= proxies.size())
  {
    vtkErrorMacro("Index " << index << " greater than max " << proxies.size());
    return 0;
  }
  return this->Internals->GetProxies()[index].Proxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyListDomain::FindProxy(const char* xmlgroup, const char* xmlname)
{
  vtkSMProxyListDomainInternals::VectorOfProxies::const_iterator iter;
  const vtkSMProxyListDomainInternals::VectorOfProxies& proxies = this->Internals->GetProxies();
  for (iter = proxies.begin(); xmlgroup != NULL && xmlname != NULL && iter != proxies.end(); iter++)
  {
    if (iter->Proxy && iter->Proxy->GetXMLGroup() && iter->Proxy->GetXMLName() &&
      strcmp(iter->Proxy->GetXMLGroup(), xmlgroup) == 0 &&
      strcmp(iter->Proxy->GetXMLName(), xmlname) == 0)
    {
      return iter->Proxy;
    }
  }
  return NULL;
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::RemoveProxy(vtkSMProxy* proxy)
{
  if (this->Internals->RemoveProxy(proxy))
  {
    this->DomainModified();
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::RemoveProxy(unsigned int index)
{
  if (this->Internals->RemoveProxy(index))
  {
    this->DomainModified();
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::ChildSaveState(vtkPVXMLElement* element)
{
  this->Superclass::ChildSaveState(element);

  vtkSMProxyListDomainInternals::VectorOfProxies::const_iterator iter;
  const vtkSMProxyListDomainInternals::VectorOfProxies& proxies = this->Internals->GetProxies();
  for (iter = proxies.begin(); iter != proxies.end(); ++iter)
  {
    vtkSMProxy* proxy = iter->Proxy.GetPointer();
    vtkPVXMLElement* proxyElem = vtkPVXMLElement::New();
    proxyElem->SetName("Proxy");
    proxyElem->AddAttribute("value", static_cast<int>(proxy->GetGlobalID()));
    element->AddNestedElement(proxyElem);
    proxyElem->Delete();
  }
}

//-----------------------------------------------------------------------------
int vtkSMProxyListDomain::LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader)
{
  this->Internals->ClearProxies();
  if (!this->Superclass::LoadState(element, loader))
  {
    return 0;
  }

  for (unsigned int cc = 0; cc < element->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* proxyElem = element->GetNestedElement(cc);
    if (strcmp(proxyElem->GetName(), "Proxy") == 0)
    {
      int id;
      if (proxyElem->GetScalarAttribute("value", &id))
      {
        vtkSMProxy* proxy = loader->LocateProxy(id);
        if (proxy)
        {
          this->Internals->AddProxy(proxy, this);
        }
      }
    }
  }
  this->DomainModified();
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::SetProxies(vtkSMProxy** proxies, unsigned int count)
{
  this->Internals->ClearProxies();
  for (unsigned int cc = 0; cc < count; ++cc)
  {
    this->Internals->AddProxy(proxies[cc], this);
  }
  this->DomainModified();
}

//-----------------------------------------------------------------------------
void vtkSMProxyListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
