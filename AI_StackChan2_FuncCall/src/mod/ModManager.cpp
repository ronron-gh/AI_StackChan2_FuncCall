#include <Arduino.h>
#include <deque>
#include "ModManager.h"
#include "ModBase.h"
#include <Avatar.h>

using namespace m5avatar;


/// 外部参照 ///
extern Avatar avatar;

///////////////


std::deque<ModBase*> modList;
//UiBase* uiList[MAX_UI_NUM] = {nullptr};
//UiBase *current_ui;


void add_mod(ModBase* mod)
{
  modList.push_back(mod);
}

ModBase* change_mod(void)
{
  ModBase* mod;
  mod = modList[0];
  mod->pause();
  modList.pop_front();
  modList.push_back(mod);
  mod = modList[0];
  mod->init();
  return mod;
}

ModBase* get_current_mod(void)
{
  return modList[0];
}

