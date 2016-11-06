/*
 *      Copyright (C) 2016 Garrett Brown
 *      Copyright (C) 2016 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ControllerTransformer.h"
#include "log/Log.h"
#include "storage/Device.h"
#include "utils/CommonMacros.h"

#include "kodi_peripheral_utils.hpp"

#include <algorithm>

using namespace JOYSTICK;

CControllerTransformer::CControllerTransformer(CJoystickFamilyManager& familyManager) :
  m_familyManager(familyManager)
{
}

void CControllerTransformer::OnAdd(const DevicePtr& driverInfo, const ButtonMap& buttonMap)
{
  // Santity check
  if (m_observedDevices.size() > 200)
    return;

  // Skip devices we've already encountered.
  if (m_observedDevices.find(driverInfo) != m_observedDevices.end())
    return;

  m_observedDevices.insert(driverInfo);

  for (auto itTo = buttonMap.begin(); itTo != buttonMap.end(); ++itTo)
  {
    // Only allow controller map items where "from" compares before "to"
    for (auto itFrom = buttonMap.begin(); itFrom->first < itTo->first; ++itFrom)
    {
      AddControllerMap(itFrom->first, itFrom->second, itTo->first, itTo->second);
    }
  }
}

DevicePtr CControllerTransformer::CreateDevice(const CDevice& deviceInfo)
{
  DevicePtr result = std::make_shared<CDevice>(deviceInfo);

  for (const auto& device : m_observedDevices)
  {
    if (*device == deviceInfo)
      result->Configuration() = device->Configuration();
  }

  return result;
}

bool CControllerTransformer::AddControllerMap(const std::string& controllerFrom, const FeatureVector& featuresFrom,
                                              const std::string& controllerTo, const FeatureVector& featuresTo)
{
  FeatureMap featureMap;

  ASSERT(controllerFrom < controllerTo);

  ControllerTranslation key = { controllerFrom, controllerTo };

  FeatureMap features;

  for (auto itFromFeature = featuresFrom.begin(); itFromFeature != featuresFrom.end(); ++itFromFeature)
  {
    const ADDON::JoystickFeature& fromFeature = *itFromFeature;

    auto itToFeature = std::find_if(featuresTo.begin(), featuresTo.end(),
      [&fromFeature](const ADDON::JoystickFeature& feature)
      {
        if (fromFeature.Type() == feature.Type())
        {
          switch (feature.Type())
          {
          case JOYSTICK_FEATURE_TYPE_SCALAR:
          case JOYSTICK_FEATURE_TYPE_MOTOR:
          {
            return fromFeature.Primitive(JOYSTICK_SCALAR_PRIMITIVE) == feature.Primitive(JOYSTICK_SCALAR_PRIMITIVE);
          }
          case JOYSTICK_FEATURE_TYPE_ANALOG_STICK:
          {
            return fromFeature.Primitive(JOYSTICK_ANALOG_STICK_UP)    == feature.Primitive(JOYSTICK_ANALOG_STICK_UP) &&
                   fromFeature.Primitive(JOYSTICK_ANALOG_STICK_DOWN)  == feature.Primitive(JOYSTICK_ANALOG_STICK_DOWN) &&
                   fromFeature.Primitive(JOYSTICK_ANALOG_STICK_RIGHT) == feature.Primitive(JOYSTICK_ANALOG_STICK_RIGHT) &&
                   fromFeature.Primitive(JOYSTICK_ANALOG_STICK_LEFT)  == feature.Primitive(JOYSTICK_ANALOG_STICK_LEFT);
          }
          case JOYSTICK_FEATURE_TYPE_ACCELEROMETER:
          {
            return fromFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_X) == feature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_X) &&
                   fromFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_Y) == feature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_Y) &&
                   fromFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_Z) == feature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_Z);
          }
          default:
            break;
          }
        }
        return false;
      });

    if (itToFeature != featuresTo.end())
    {
      FeatureTranslation featureEntry = { fromFeature.Name(), itToFeature->Name() };
      features.insert(std::move(featureEntry));
    }
  }

  if (!features.empty())
  {
    FeatureMaps& featureMaps = m_controllerMap[key];

    auto it = featureMaps.find(features);
    if (it != featureMaps.end())
      ++it->second;
    else
      featureMaps.insert(std::make_pair(std::move(features), 1));

    return true;
  }

  return false;
}

void CControllerTransformer::TransformFeatures(const ADDON::Joystick& driverInfo,
                                               const std::string& fromController,
                                               const std::string& toController,
                                               const FeatureVector& features,
                                               FeatureVector& transformedFeatures)
{
  bool bSwap = (fromController >= toController);

  ControllerTranslation needle = { bSwap ? toController : fromController,
                                   bSwap ? fromController : toController };

  FeatureMaps& featureMaps = m_controllerMap[needle];

  unsigned int maxCount = 0;
  const FeatureMap* bestFeatureMap = nullptr;

  for (const auto& featureMap : featureMaps)
  {
    const FeatureMap& features = featureMap.first;
    unsigned int count = featureMap.second;

    dsyslog("Found %u controller transformations from %s to %s with %u features:", count, fromController.c_str(), toController.c_str(), features.size());
    for (auto& featureTranslation : features)
      dsyslog("    %s -> %s", featureTranslation.fromFeature.c_str(), featureTranslation.toFeature.c_str());

    if (count > maxCount)
    {
      maxCount = count;
      bestFeatureMap = &features;
    }
  }

  if (bestFeatureMap != nullptr)
  {
    dsyslog("Best transformatio with %u translations:", bestFeatureMap->size());
    for (auto& featureTranslation : *bestFeatureMap)
      dsyslog("    %s -> %s", featureTranslation.fromFeature.c_str(), featureTranslation.toFeature.c_str());

    for (const auto& featurePair : *bestFeatureMap)
    {
      const std::string& fromFeature = bSwap ? featurePair.toFeature : featurePair.fromFeature;
      const std::string& toFeature = bSwap ? featurePair.fromFeature : featurePair.toFeature;

      auto itFrom = std::find_if(features.begin(), features.end(),
        [fromFeature](const ADDON::JoystickFeature& feature)
        {
          return feature.Name() == fromFeature;
        });

      if (itFrom != features.end())
      {
        ADDON::JoystickFeature transformedFeature(*itFrom);
        transformedFeature.SetName(toFeature);
        transformedFeatures.emplace_back(std::move(transformedFeature));
      }
    }
  }
}
