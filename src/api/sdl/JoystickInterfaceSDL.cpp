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

#include "JoystickInterfaceSDL.h"
#include "JoystickSDL.h"
#include "api/JoystickTypes.h"

#include <SDL2/SDL.h>

using namespace JOYSTICK;

const char* CJoystickInterfaceSDL::Name(void) const
{
  return INTERFACE_SDL;
}

bool CJoystickInterfaceSDL::Initialize(void)
{
  return (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) == 0);
}

void CJoystickInterfaceSDL::Deinitialize(void)
{
  SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
}

bool CJoystickInterfaceSDL::ScanForJoysticks(JoystickVector& joysticks)
{
  for (int i = 0; i < SDL_NumJoysticks(); i++)
  {
    if (!SDL_IsGameController(i))
      continue;

    SDL_GameController* pController = SDL_GameControllerOpen(i);
    if (!pController)
      continue;

    joysticks.push_back(JoystickPtr(new CJoystickSDL(pController)));
  }

  return true;
}
