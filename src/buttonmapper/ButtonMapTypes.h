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
 */
#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ADDON
{
  struct DriverPrimitive;
  class JoystickFeature;
};

namespace JOYSTICK
{
  /*!
   * \brief A list of driver primitives
   */
  typedef std::vector<ADDON::DriverPrimitive> PrimitiveVector;

  /*!
   * \brief A list of joystick features
   *
   * Each feature is connected to one or more driver primitives.
   */
  typedef std::vector<ADDON::JoystickFeature> FeatureVector;

  /*!
   * \brief Controller ID type
   */
  typedef std::string ControllerID;

  /*!
   * \brief Entire set of controller profiles for a device
   */
  typedef std::map<ControllerID, FeatureVector> ButtonMap;

  /*!
   * \brief Feature translation entry
   */
  struct FeatureTranslation
  {
    std::string fromFeature;
    std::string toFeature;

    bool operator<(const FeatureTranslation& other) const
    {
      if (fromFeature < other.fromFeature) return true;
      if (fromFeature > other.fromFeature) return false;

      if (toFeature < other.toFeature) return true;
      if (toFeature > other.toFeature) return false;

      return false;
    }
  };

  typedef std::set<FeatureTranslation> FeatureMap;

  typedef std::map<FeatureMap, unsigned int> FeatureMaps; // Feature map -> occurrences

  /*!
   * \brief Feature translation entry
   */
  struct ControllerTranslation
  {
    std::string fromController;
    std::string toController;

    bool operator<(const ControllerTranslation& other) const
    {
      if (fromController < other.fromController) return true;
      if (fromController > other.fromController) return false;

      if (toController < other.toController) return true;
      if (toController > other.toController) return false;

      return false;
    }
  };

  typedef std::map<ControllerTranslation, FeatureMaps> ControllerMap;

  typedef std::string FamilyName;
  typedef std::string JoystickName;

  typedef std::map<FamilyName, std::set<JoystickName>> JoystickFamilyMap;
}
