#pragma once
#include <string>

enum class CmdType { SetInterval, DebugOn, DebugOff, Reboot, Ota, Unknown };

struct Command {
  CmdType type;
  long value;       // usado por SetInterval (segundos)
  std::string arg;  // usado por Ota (URL)
};

// Interpreta o payload recebido em nexahome/<id>/cmd.
// Não autentica a origem: isso é a fragilidade F04, de propósito.
Command parseCommand(const std::string& payload);
