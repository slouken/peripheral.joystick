/*
 *      Copyright (C) 2015 Garrett Brown
 *      Copyright (C) 2015 Team XBMC
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

#include "ButtonMap.h"
#include "Device.h"
#include "DeviceConfiguration.h"
#include "StorageManager.h"
#include "StorageUtils.h"
#include "log/Log.h"

#include "kodi_peripheral_utils.hpp"
#include "p8-platform/util/timeutils.h"

#include <algorithm>
#include <set>

using namespace JOYSTICK;

#define RESOURCE_LIFETIME_MS  2000 // 2 seconds

CButtonMap::CButtonMap(const std::string& strResourcePath) :
  m_strResourcePath(strResourcePath),
  m_device(std::move(std::make_shared<CDevice>())),
  m_timestamp(-1),
  m_bModified(false)
{
}

CButtonMap::CButtonMap(const std::string& strResourcePath, const DevicePtr& device) :
  m_strResourcePath(strResourcePath),
  m_device(device),
  m_timestamp(-1),
  m_bModified(false)
{
}

bool CButtonMap::IsValid(void) const
{
  return m_device->IsValid();
}

const ButtonMap& CButtonMap::GetButtonMap()
{
  if (!m_bModified)
    Refresh();

  return m_buttonMap;
}

void CButtonMap::MapFeatures(const std::string& controllerId, const FeatureVector& features)
{
  // Create a backup to allow revert
  if (m_originalButtonMap.empty())
    m_originalButtonMap = m_buttonMap;

  FeatureVector& myFeatures = m_buttonMap[controllerId];

  // Remove features with the same name
  for (auto& newFeature : features)
  {
    myFeatures.erase(std::remove_if(myFeatures.begin(), myFeatures.end(),
      [&newFeature, &controllerId](const ADDON::JoystickFeature& feature)
      {
        if (feature.Name() == newFeature.Name())
        {
          dsyslog("%s: Overwriting feature \"%s\"", controllerId.c_str(), feature.Name().c_str());
          return true;
        }
        return false;
      }), myFeatures.end());
  }

  // Update axis configurations
  for (auto& newFeature : features)
  {
    std::set<unsigned int> updatedAxes;
    for (auto& primitive : newFeature.Primitives())
    {
      if (primitive.Type() == JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS)
        updatedAxes.insert(primitive.DriverIndex());
    }

    // TODO
    //JOYSTICK_FEATURE_CATEGORY category = CStorageManager::Get().GetFeatureCategory(controllerId, newFeature.Name());

    for (unsigned int axis : updatedAxes)
      m_device->Configuration().LoadAxisFromAPI(axis, *m_device);
  }

  myFeatures.insert(myFeatures.begin(), features.begin(), features.end());

  Sanitize(controllerId, myFeatures);

  std::sort(myFeatures.begin(), myFeatures.end(),
    [](const ADDON::JoystickFeature& lhs, const ADDON::JoystickFeature& rhs)
    {
      return lhs.Name() < rhs.Name();
    });

  m_bModified = true;
}

bool CButtonMap::SaveButtonMap()
{
  if (Save())
  {
    m_timestamp = P8PLATFORM::GetTimeMs();
    m_originalButtonMap.clear();
    m_bModified = false;
    return true;
  }

  return false;
}

bool CButtonMap::RevertButtonMap()
{
  if (!m_originalButtonMap.empty())
  {
    m_buttonMap = m_originalButtonMap;
    return true;
  }

  return false;
}

bool CButtonMap::ResetButtonMap(const std::string& controllerId)
{
  FeatureVector& features = m_buttonMap[controllerId];

  if (!features.empty())
  {
    features.clear();
    return SaveButtonMap();
  }

  return false;
}

bool CButtonMap::Refresh(void)
{
  const int64_t expires = m_timestamp + RESOURCE_LIFETIME_MS;
  const int64_t now = P8PLATFORM::GetTimeMs();

  if (now >= expires)
  {
    if (!Load())
      return false;

    for (auto it = m_buttonMap.begin(); it != m_buttonMap.end(); ++it)
      Sanitize(it->first, it->second);

    m_timestamp = now;
    m_originalButtonMap.clear();
  }

  return true;
}

void CButtonMap::Sanitize(const std::string& controllerId, FeatureVector& features)
{
  // Loop through features
  for (unsigned int iFeature = 0; iFeature < features.size(); ++iFeature)
  {
    auto& feature = features[iFeature];

    // Loop through feature's primitives
    auto& primitives = feature.Primitives();
    for (unsigned int iPrimitive = 0; iPrimitive < primitives.size(); ++iPrimitive)
    {
      auto& primitive = primitives[iPrimitive];

      if (primitive.Type() == JOYSTICK_DRIVER_PRIMITIVE_TYPE_UNKNOWN)
        continue;

      bool bFound = false;

      // Search for prior feature with the primitive
      ADDON::JoystickFeature existingFeature;

      for (unsigned int iExistingFeature = 0; iExistingFeature < iFeature; ++iExistingFeature)
      {
        const auto& existingPrimitives = features[iExistingFeature].Primitives();
        if (std::find(existingPrimitives.begin(), existingPrimitives.end(), primitive) != existingPrimitives.end())
        {
          existingFeature = features[iExistingFeature];
          bFound = true;
          break;
        }
      }

      if (!bFound)
      {
        // Search for primitive in prior primitives
        for (unsigned int iExistingPrimitive = 0; iExistingPrimitive < iPrimitive; ++iExistingPrimitive)
        {
          if (primitives[iExistingPrimitive] == primitive)
          {
            bFound = true;
            break;
          }
        }
      }

      // Invalid the primitive if it has already been seen
      if (bFound)
      {
        esyslog("%s: %s (%s) conflicts with %s (%s)",
            controllerId.c_str(),
            CStorageUtils::PrimitiveToString(primitive).c_str(),
            existingFeature.Type() != JOYSTICK_FEATURE_TYPE_UNKNOWN ? existingFeature.Name().c_str() : feature.Name().c_str(),
            CStorageUtils::PrimitiveToString(primitive).c_str(),
            feature.Name().c_str());

        primitive = ADDON::DriverPrimitive();
      }
    }
  }

  // Erase invalid features
  features.erase(std::remove_if(features.begin(), features.end(),
    [&controllerId](const ADDON::JoystickFeature& feature)
    {
      auto& primitives = feature.Primitives();

      // Find valid primitive
      auto it = std::find_if(primitives.begin(), primitives.end(),
        [](const ADDON::DriverPrimitive& primitive)
        {
          return primitive.Type() != JOYSTICK_DRIVER_PRIMITIVE_TYPE_UNKNOWN;
        });

      const bool bIsValid = (it != primitives.end());

      if (!bIsValid)
      {
        dsyslog("%s: Removing %s from button map", controllerId.c_str(), feature.Name().c_str());
        return true;
      }

      return false;
    }), features.end());
}
