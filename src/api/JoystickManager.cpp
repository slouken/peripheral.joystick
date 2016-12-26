/*
 *      Copyright (C) 2014-2015 Garrett Brown
 *      Copyright (C) 2014-2015 Team XBMC
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "JoystickManager.h"
#include "IJoystickInterface.h"
#include "Joystick.h"

#if defined(HAVE_DIRECT_INPUT)
  #include "directinput/JoystickInterfaceDirectInput.h"
#endif
#if defined(HAVE_XINPUT)
  #include "xinput/JoystickInterfaceXInput.h"
#endif
#if defined(HAVE_LINUX_JOYSTICK)
  #include "linux/JoystickInterfaceLinux.h"
#endif
#if defined(HAVE_COCOA)
  #include "cocoa/JoystickInterfaceCocoa.h"
#endif
#if defined(HAVE_UDEV)
  #include "udev/JoystickInterfaceUdev.h"
#endif
#if defined(HAVE_SDL)
  #include "sdl/JoystickInterfaceSDL.h"
#endif

#include "log/Log.h"
#include "utils/CommonMacros.h"

#include <algorithm>

using namespace JOYSTICK;
using namespace P8PLATFORM;

// --- Utility functions -------------------------------------------------------

namespace JOYSTICK
{
  struct ScanResultEqual
  {
    ScanResultEqual(const JoystickPtr& needle) : m_needle(needle) { }

    bool operator()(const JoystickPtr& rhs)
    {
      if (m_needle)
        return m_needle->Equals(rhs.get());

      return m_needle == rhs;
    }

  private:
    JoystickPtr const m_needle;
  };

  template <class T>
  void safe_delete(T*& pVal)
  {
      delete pVal;
      pVal = NULL;
  }

  template <class T>
  void safe_delete_vector(std::vector<T*>& vec)
  {
    for (typename std::vector<T*>::iterator it = vec.begin(); it != vec.end(); ++it)
      delete *it;
    vec.clear();
  }
}

// --- CJoystickManager --------------------------------------------------------

CJoystickManager::CJoystickManager(void)
  : m_scanner(NULL),
    m_nextJoystickIndex(0)
{
}

CJoystickManager& CJoystickManager::Get(void)
{
  static CJoystickManager _instance;
  return _instance;
}

bool CJoystickManager::Initialize(IScannerCallback* scanner)
{
  CLockObject lock(m_interfacesMutex);

  m_scanner = scanner;

  // Windows
#if defined(HAVE_DIRECT_INPUT)
  m_interfaces.push_back(new CJoystickInterfaceDirectInput);
#endif
#if defined(HAVE_XINPUT)
  m_interfaces.push_back(new CJoystickInterfaceXInput);
#endif

  // Linux
#if defined(HAVE_SDL)
  m_interfaces.push_back(new CJoystickInterfaceSDL);
#elif defined(HAVE_LINUX_JOYSTICK)
  m_interfaces.push_back(new CJoystickInterfaceLinux);
#elif defined(HAVE_UDEV)
  m_interfaces.push_back(new CJoystickInterfaceUdev);
#endif

  // OSX
#if defined(HAVE_COCOA)
  m_interfaces.push_back(new CJoystickInterfaceCocoa);
#endif

  if (m_interfaces.empty())
    dsyslog("No joystick APIs in use");

  // Initialise all known interfaces
  for (int i = (int)m_interfaces.size() - 1; i >= 0; i--)
  {
    if (!m_interfaces.at(i)->Initialize())
    {
      esyslog("Failed to initialize interface %s", m_interfaces.at(i)->Name());
      delete m_interfaces.at(i);
      m_interfaces.erase(m_interfaces.begin() + i);
    }
  }

  return true;
}

void CJoystickManager::Deinitialize(void)
{
  {
    CLockObject lock(m_joystickMutex);
    m_joysticks.clear();
  }

  {
    CLockObject lock(m_interfacesMutex);
    safe_delete_vector(m_interfaces);
  }

  m_scanner = NULL;
}

bool CJoystickManager::PerformJoystickScan(JoystickVector& joysticks)
{
  JoystickVector scanResults;
  {
    CLockObject lock(m_interfacesMutex);
    // Scan for joysticks (this can take a while, don't block)
    for (std::vector<IJoystickInterface*>::iterator itInterface = m_interfaces.begin(); itInterface != m_interfaces.end(); ++itInterface)
      (*itInterface)->ScanForJoysticks(scanResults);
  }

  CLockObject lock(m_joystickMutex);

  // Unregister removed joysticks
  for (int i = (int)m_joysticks.size() - 1; i >= 0; i--)
  {
    if (std::find_if(scanResults.begin(), scanResults.end(), ScanResultEqual(m_joysticks.at(i))) == scanResults.end())
      m_joysticks.erase(m_joysticks.begin() + i);
  }

  // Register new joysticks
  for (JoystickVector::iterator itJoystick = scanResults.begin(); itJoystick != scanResults.end(); ++itJoystick)
  {
    if (std::find_if(m_joysticks.begin(), m_joysticks.end(), ScanResultEqual(*itJoystick)) == m_joysticks.end())
    {
      if ((*itJoystick)->Initialize())
      {
        (*itJoystick)->SetIndex(m_nextJoystickIndex++);

        isyslog("Initialized joystick %u: \"%s\", axes: %u, hats: %u, buttons: %u",
                (*itJoystick)->Index(), (*itJoystick)->Name().c_str(),
                (*itJoystick)->AxisCount(), (*itJoystick)->HatCount(), (*itJoystick)->ButtonCount());

        m_joysticks.push_back(*itJoystick);
      }
    }
  }

  joysticks = m_joysticks;

  // Work around bug on linux: Don't return disconnected Xbox 360 controllers
  joysticks.erase(std::remove_if(joysticks.begin(), joysticks.end(),
    [](const JoystickPtr& joystick)
    {
      return (joystick->Provider() == INTERFACE_LINUX ||
               joystick->Provider() == INTERFACE_UDEV) &&
             (joystick->Name() == "Xbox 360 Wireless Receiver" ||
               joystick->Name() == "Xbox 360 Wireless Receiver (XBOX)") &&
             joystick->ActivateTimeMs() < 0;
    }), joysticks.end());

  return true;
}

JoystickPtr CJoystickManager::GetJoystick(unsigned int index) const
{
  CLockObject lock(m_joystickMutex);

  for (JoystickVector::const_iterator it = m_joysticks.begin(); it != m_joysticks.end(); ++it)
  {
    if ((*it)->Index() == index)
      return *it;
  }

  return JoystickPtr();
}

JoystickVector CJoystickManager::GetJoysticks(const ADDON::Joystick& joystickInfo) const
{
  JoystickVector result;

  CLockObject lock(m_joystickMutex);

  for (const auto& joystick : m_joysticks)
  {
    if (joystick->Name() == joystickInfo.Name() &&
        joystick->Provider() == joystickInfo.Provider())
    {
      result.push_back(joystick);
    }
  }

  return result;
}

bool CJoystickManager::GetEvents(std::vector<ADDON::PeripheralEvent>& events)
{
  CLockObject lock(m_joystickMutex);

  for (JoystickVector::iterator it = m_joysticks.begin(); it != m_joysticks.end(); ++it)
    (*it)->GetEvents(events);

  return true;
}

bool CJoystickManager::SendEvent(const ADDON::PeripheralEvent& event)
{
  bool bHandled = false;

  CLockObject lock(m_joystickMutex);

  for (const JoystickPtr& joystick : m_joysticks)
  {
    if (joystick->Index() == event.PeripheralIndex())
    {
      bHandled = joystick->SendEvent(event);
      if (bHandled)
        break;
    }
  }

  return bHandled;
}

void CJoystickManager::ProcessEvents()
{
  CLockObject lock(m_joystickMutex);

  for (const JoystickPtr& joystick : m_joysticks)
    joystick->ProcessEvents();
}

void CJoystickManager::TriggerScan(void)
{
  if (m_scanner)
    m_scanner->TriggerScan();
}

const ButtonMap& CJoystickManager::GetButtonMap(const std::string& provider)
{
  static ButtonMap empty;

  CLockObject lock(m_interfacesMutex);

  // Scan for joysticks (this can take a while, don't block)
  for (std::vector<IJoystickInterface*>::iterator itInterface = m_interfaces.begin(); itInterface != m_interfaces.end(); ++itInterface)
  {
    if ((*itInterface)->Name() == provider)
      return (*itInterface)->GetButtonMap();
  }

  return empty;
}
