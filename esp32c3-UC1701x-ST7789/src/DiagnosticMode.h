#ifndef DIAGNOSTIC_MODE_H
#define DIAGNOSTIC_MODE_H

#include <Arduino.h>

// 诊断模式标志
extern bool diagnosticModeEnabled;

// 诊断模式函数声明
void enterDiagnosticMode();
void processDiagnosticCommands();
void printDiagnosticHelp();
void printSystemStatus();
void runNetworkTest();
void runDisplayTest();
void toggleFrameRateLimit(bool enable);

#endif // DIAGNOSTIC_MODE_H
