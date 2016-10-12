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
 */

#include "AnomalousTrigger.h"
#include "log/Log.h"
#include "settings/Settings.h"

using namespace JOYSTICK;

#define ANOMOLOUS_MAGNITUDE  0.5f

CAnomalousTrigger::CAnomalousTrigger(unsigned int axisIndex)
  : m_axisIndex(axisIndex),
    m_state(STATE_UNKNOWN),
    m_center(CENTER_ZERO),
    m_range(TRIGGER_RANGE_HALF),
    m_bCenterSeen(false),
    m_bPositiveOneSeen(false),
    m_bNegativeOneSeen(false)
{
}

float CAnomalousTrigger::Filter(float value)
{
  // First, check for discrete D-pad
  if (m_state == STATE_UNKNOWN)
  {
    // Is value a discrete integer?
    if (value == -1.0f || value == 0.0f || value == 1.0f)
    {
      // Might be a discrete dpad
      if      (value ==  0.0f) m_bCenterSeen      = true;
      else if (value ==  1.0f) m_bPositiveOneSeen = true;
      else if (value == -1.0f) m_bNegativeOneSeen = true;

      if (m_bCenterSeen && m_bPositiveOneSeen && m_bNegativeOneSeen)
      {
        m_state = STATE_DISCRETE_DPAD;
        dsyslog("Discrete D-pad detected on axis %u", m_axisIndex);
      }
    }
    else
    {
      m_state = STATE_NOT_DISCRETE_DPAD;
    }
  }

  // Calculate center position
  if (m_state == STATE_NOT_DISCRETE_DPAD)
  {
    if (value < -ANOMOLOUS_MAGNITUDE)
      m_center = CENTER_NEGATIVE_ONE;
    else if (value > ANOMOLOUS_MAGNITUDE)
      m_center = CENTER_POSITIVE_ONE;
    else
      m_center = CENTER_ZERO;

    if (IsAnomalousTrigger())
      dsyslog("Anomalous trigger detected on axis %u (initial value = %f)", m_axisIndex, value);

    m_state = STATE_CENTER_KNOWN;
  }

  // Process anomalous trigger
  if (IsAnomalousTrigger())
  {
    // Adjust range if value enters opposite semiaxis
    if (m_state == STATE_CENTER_KNOWN)
    {
      if (m_center == CENTER_NEGATIVE_ONE && value > 0.0f)
      {
        m_range = TRIGGER_RANGE_FULL;
        m_state = STATE_RANGE_KNOWN;
      }
      else if (m_center == CENTER_POSITIVE_ONE && value < 0.0f)
      {
        m_range = TRIGGER_RANGE_FULL;
        m_state = STATE_RANGE_KNOWN;
      }
    }

    // Only filter if allowed in the settings
    if (CSettings::Get().FixTriggers())
    {
      // Translate center to zero
      value = value - GetCenter(m_center);

      // Adjust range to lie in an interval of length 1
      if (m_range == TRIGGER_RANGE_FULL)
        value /= 2;
    }
  }

  return value;
}

bool CAnomalousTrigger::IsAnomalousTrigger(void) const
{
  return m_center != CENTER_ZERO;
}

int CAnomalousTrigger::GetCenter(AXIS_CENTER center)
{
  switch (center)
  {
    case CENTER_NEGATIVE_ONE: return -1;
    case CENTER_POSITIVE_ONE: return  1;
    default:
      break;
  }
  return 0;
}

unsigned int CAnomalousTrigger::GetRange(TRIGGER_RANGE range)
{
  switch (range)
  {
    case TRIGGER_RANGE_HALF: return 1;
    case TRIGGER_RANGE_FULL: return 2;
    default:
      break;
  }
  return 1;
}
