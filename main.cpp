#ifndef MAIN_CPP
#define MAIN_CPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <regex>
#include <chrono>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <set>
#include <cstdlib>

// Local files
#include "dependency.cpp"
#include "classes.h"
#include "parser.cpp"
#include "util.cpp"
#include "run.cpp"

using namespace std;

// Global Variable declaration
unordered_map<string, filesystem::file_time_type> timestamps;
vector<string> currentFiles;
vector<pid_t> childProcesses;
volatile sig_atomic_t timeoutOccurred = 0;
bool isDebug = false;
bool isCustomMakefile = false;
int timeValue;
bool printOnly = false;
bool continueExecution = false;
bool blockSignal = false;
int timeout = 0;

void handleSIGINT(int signum)
{
    cout << "SIGINT received." << endl;
    if (blockSignal == true)
    {
        cout << "Program is running with -i flag. Continuing..." << endl;
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}
void handleSIGALRM(int signum)
{
    cout << "Timeout reached. Cleaning up and exiting." << endl;
    timeoutOccurred = 1;
    cleanUp(childProcesses, false, isDebug);
    signal(SIGALRM, SIG_DFL);
}

int handleNestedBuild(Makefile &myMakefile, string &t, vector<string> &visited)
{
    for (const auto &targetRule : myMakefile.targetRules)
    {
        if (find(targetRule.prerequisites.begin(), targetRule.prerequisites.end(), t) != targetRule.prerequisites.end())
        {
            string dependentTarget = targetRule.name;
            DEBUG_COMMENT("Building target with prerequisite: " + dependentTarget, isDebug);

            // Check if the dependent target is dirty
            TargetRules dependentRule = getTargetRule(dependentTarget, myMakefile.targetRules);
            if (dependentRule.name.empty())
            {
                DEBUG_COMMENT("Dependent rule not found. Error ", isDebug);
                return -1;
            }
            if (isDirty(dependentRule, timestamps))
            {
                DEBUG_COMMENT("No need to build dependent target. Nothing changed from last time", isDebug);
                continue;
            }

            // Build the dependent target
            handleTargetRulesDep(myMakefile, dependentTarget, visited, childProcesses, continueExecution, isDebug);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int opt;
    const char *optstring = "f:t:ikpd";
    string makefileValue = "makefile";
    vector<string> targetList;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (!arg.empty() && arg[0] == '-')
        {
            if (arg == "-f" || arg == "-t")
            {
                std::string value = argv[i + 1];
                handleCommandArgs(arg, value, makefileValue, isCustomMakefile,
                                  timeValue, timeout, printOnly, blockSignal,
                                  isDebug, continueExecution, targetList);
                ++i;
            }
            else
            {
                handleCommandArgs(arg, "", makefileValue, isCustomMakefile,
                                  timeValue, timeout, printOnly, blockSignal,
                                  isDebug, continueExecution, targetList);
            }
        }
        else
        {
            targetList.push_back(arg);
        }
    }

    if (blockSignal)
    {
        DEBUG_COMMENT("Setting up singal handler", isDebug);
        signal(SIGINT, handleSIGINT);
    }

    if (timeout > 0)
    {
        DEBUG_COMMENT("Setting timeout " + to_string(timeout), isDebug);
        alarm(timeout);
        signal(SIGALRM, handleSIGALRM);
    }

    DEBUG_COMMENT("Populating timestamps for files in the current directory", isDebug);
    populateAllCurrentFiles(currentFiles);
    for (const string &f : currentFiles)
    {
        filesystem::path filePath(f);
        string fileName = filePath.filename();
        timestamps[fileName] = getLastModifiedTime(f);
    }

    Makefile myMakefile;
    parseMakeFile(makefileValue, myMakefile);
    DEBUG_COMMENT("Parsed makefile", isDebug);

    if (printOnly)
    {
        DEBUG_COMMENT("Print only so only printing makefile");
        return printMakefile(myMakefile);
    }

    DEBUG_COMMENT("Replacing macros for commands in the target rule", isDebug);
    for (TargetRules &rule : myMakefile.targetRules)
    {
        for (auto &command : rule.commands)
        {
            command = replaceVariables(command, myMakefile.macros, rule.name, rule.prerequisites);
        }
    }
    vector<string> visited;
    if (targetList.size() > 0)
    {
        DEBUG_COMMENT("Building specific targets", isDebug);
        for (string t : targetList)
        {
            DEBUG_COMMENT("Building target:" + t, isDebug);
            DEBUG_COMMENT("Checking if target is dirty", isDebug);
            TargetRules rule = getTargetRule(t, myMakefile.targetRules);
            if (rule.name.empty())
            {
                DEBUG_COMMENT("Rule not found. Error ", isDebug);
                return -1;
            }
            if (isDirty(rule, timestamps))
            {
                DEBUG_COMMENT("No need to build target. Nothing changed from last time", isDebug);
                continue;
            }
            handleTargetRulesDep(myMakefile, t, visited, childProcesses, continueExecution, isDebug);
            handleNestedBuild(myMakefile, t, visited);
        }
        return 0;
    }
    DEBUG_COMMENT("Building and executing inference rules", isDebug);
    handleInferenceRulesFile(myMakefile, childProcesses, continueExecution, isDebug);
    DEBUG_COMMENT("Building and executing all target rules", isDebug);
    for (const auto &targetRule : myMakefile.targetRules)
    {
        string currentTarget = targetRule.name;
        if (currentTarget == "clean")
        {
            DEBUG_COMMENT("Skipping clean build", isDebug);
            continue;
        }
        DEBUG_COMMENT("Building target:" + targetRule.name, isDebug);
        DEBUG_COMMENT("Checking if target is dirty", isDebug);
        if (targetRule.name.empty())
        {
            DEBUG_COMMENT("Rule not found. Error ", isDebug);
            return -1;
        }
        if (isDirty(targetRule, timestamps) == false)
        {
            DEBUG_COMMENT("No need to build target. Nothing changed from last time", isDebug);
            continue;
        }
        else
        {
            DEBUG_COMMENT("Target is not dirty. Continuing to build..", isDebug);
            handleTargetRulesDep(myMakefile, currentTarget, visited, childProcesses, continueExecution, isDebug);
        }
    }

    return 0;
}
#endif // MAIN_CPP
