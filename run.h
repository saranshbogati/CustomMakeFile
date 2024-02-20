#pragma once
void handlePipedCommand(bool isDebug, const __1::string &command, __1::string &output, bool continueExecution);

void handleSemicolonCommand(bool isDebug, const __1::string &command, __1::string &output, bool continueExecution);

void handleNormalCommand(bool isDebug, const __1::string &command, __1::string &output, bool continueExecution);
