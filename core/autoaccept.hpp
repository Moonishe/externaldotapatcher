#pragma once

#include <string>

enum class AutoAcceptMode { Off, Normal, DotaPlus };


bool autoaccept_init();
void autoaccept_shutdown();
void autoaccept_set_mode(AutoAcceptMode mode);
AutoAcceptMode autoaccept_get_mode();
std::string autoaccept_mode_str();
