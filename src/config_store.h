#pragma once

#include <Preferences.h>

#include "bridge_types.h"

namespace config_store {

void loadConfig(Preferences &prefs, BridgeConfig &config);
void saveConfig(Preferences &prefs, const BridgeConfig &config);

}  // namespace config_store
