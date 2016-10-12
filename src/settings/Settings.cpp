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

#include "Settings.h"
#include "log/Log.h"

using namespace JOYSTICK;

#define SETTING_FIX_TRIGGERS      "fixtriggers"
#define SETTING_RETROARCH_CONFIG  "retroarchconfig"

CSettings::CSettings(void)
  : m_bInitialized(false),
    m_bFixTriggers(false),
    m_bGenerateRetroArchConfigs(false)
{
}

CSettings& CSettings::Get(void)
{
  static CSettings _instance;
  return _instance;
}

void CSettings::SetSetting(const std::string& strName, const void* value)
{
  if (strName == SETTING_FIX_TRIGGERS)
  {
    m_bFixTriggers = *static_cast<const bool*>(value);
    dsyslog("Setting \"%s\" set to %s", SETTING_FIX_TRIGGERS, m_bFixTriggers ? "true" : "false");
  }
  if (strName == SETTING_RETROARCH_CONFIG)
  {
    m_bGenerateRetroArchConfigs = *static_cast<const bool*>(value);
    dsyslog("Setting \"%s\" set to %f", SETTING_RETROARCH_CONFIG, m_bGenerateRetroArchConfigs ? "true" : "false");
  }

  m_bInitialized = true;
}
