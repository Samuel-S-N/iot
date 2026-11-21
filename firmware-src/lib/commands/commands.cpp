#include "commands.h"
#include <cstdlib>

static bool startsWith(const std::string& s, const std::string& p) {
  return s.rfind(p, 0) == 0;
}

Command parseCommand(const std::string& p) {
  if (p == "debug_on")  return {CmdType::DebugOn, 0, ""};
  if (p == "debug_off") return {CmdType::DebugOff, 0, ""};
  if (p == "reboot")    return {CmdType::Reboot, 0, ""};

  const std::string si = "set_interval:";
  if (startsWith(p, si)) {
    char* end = nullptr;
    long v = std::strtol(p.c_str() + si.size(), &end, 10);
    if (end && *end == '\0' && v > 0) return {CmdType::SetInterval, v, ""};
    return {CmdType::Unknown, 0, ""};
  }

  const std::string ota = "ota:";
  if (startsWith(p, ota) && p.size() > ota.size())
    return {CmdType::Ota, 0, p.substr(ota.size())};

  return {CmdType::Unknown, 0, ""};
}
