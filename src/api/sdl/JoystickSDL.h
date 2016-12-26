/*
 *      Copyright (C) 2016-2017 Sam Lantinga
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
#pragma once

#include "api/Joystick.h"

typedef struct _SDL_GameController SDL_GameController;

namespace JOYSTICK
{
  class CJoystickSDL : public CJoystick
  {
  public:
    CJoystickSDL(SDL_GameController* pController);
    virtual ~CJoystickSDL(void);

    virtual bool Equals(const CJoystick* rhs) const override;

  protected:
    virtual bool ScanEvents(void) override;

  private:
    SDL_GameController *m_pController;
  };
}
