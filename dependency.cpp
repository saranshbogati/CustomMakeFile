#ifndef DEPENDENCY_CPP
#define DEPENDENCY_CPP

#include <vector>
#include <unordered_map>
#include <map>
#include <stack>
#include <string>
#include "classes.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "run.cpp"
#include "print.cpp"

using namespace std;

bool find(vector<string> vec, string toCheck)
{
    for (string s : vec)
    {
        if (s == toCheck)
        {
            return true;
        }
    }
    return false;
}

void handleTargetRulesDep(Makefile &makefile,
                          string &currentTarget, vector<string> &visited,
                          vector<int> childProcesses, bool continueExecution, bool isDebug)
{
    TargetRules rule;

    for (TargetRules r : makefile.targetRules)
    {
        if (r.name == currentTarget)
        {
            rule = r;
            break;
        }
    }
    bool found = find(visited, currentTarget);

    if ((rule.name.empty()) || found)
    {
        DEBUG_COMMENT("Target not found or already visited.", isDebug);

        if (filesystem::exists(currentTarget))
        {
            DEBUG_COMMENT("File exists: " + currentTarget, isDebug);
        }
        return;
    }
    visited.push_back(currentTarget);

    if (!rule.prerequisites.empty())
    {
        for (string dependency : rule.prerequisites)
        {
            bool found = find(visited, dependency);
            if (found == false)
            {
                DEBUG_COMMENT("Visiting dependency: " + dependency, isDebug);
                handleTargetRulesDep(makefile, dependency, visited, childProcesses, continueExecution, isDebug);
            }
        }
    }
    if (!rule.commands.empty())
    {
        DEBUG_COMMENT("Executing commands for target: " + currentTarget, isDebug);
        for (string cmd : rule.commands)
        {
            DEBUG_COMMENT("Executing command: " + cmd, isDebug);
            handleCommandMultiple(cmd, isDebug, continueExecution);
        }
    }
}

#endif // DEPENDENCY_CPP
