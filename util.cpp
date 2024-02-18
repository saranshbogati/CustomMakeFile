#ifndef UTIL_CPP
#define UTIL_CPP
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>
#include <iostream>

#include "classes.cpp"
#include "parser.cpp"
#include "dependency.cpp"
#include <signal.h>

using namespace std;

vector<string> getCommandVector(const vector<string> &commands)
{
    vector<string> result;

    for (const string &command : commands)
    {
        istringstream iss(command);
        string singleCommand;

        while (getline(iss, singleCommand, ';'))
        {
            singleCommand.erase(singleCommand.find_last_not_of(" \t\n\r\f\v") + 1);
            singleCommand.erase(0, singleCommand.find_first_not_of(" \t\n\r\f\v"));

            if (!singleCommand.empty())
            {
                result.push_back(singleCommand);
            }
        }
    }

    return result;
}

filesystem::file_time_type getLastModifiedTime(const string &filename)
{
    return filesystem::last_write_time(filename);
}

bool isDirty(TargetRules rule, unordered_map<string, filesystem::file_time_type> &timestamps)
{
    if (rule.name == "clean")
    {
        return false;
    }

    filesystem::file_time_type sourceTimestamp = timestamps[rule.name];

    for (string &r : rule.prerequisites)
    {
        filesystem::file_time_type prerequisiteTimeStamp = timestamps[r];
        if (prerequisiteTimeStamp > sourceTimestamp)
        {
            return true;
        }
    }
    if (rule.prerequisites.size() == 0)
    {
        return true;
    }
    return false;
};

TargetRules getTargetRule(string name, vector<TargetRules> rules)
{
    for (const TargetRules &rule : rules)
    {
        if (name == rule.name)
        {
            return rule;
        }
    }
    return TargetRules();
}

vector<string> findTargetRuleByName(string name, Makefile makefile, unordered_map<string, filesystem::file_time_type> &timestamps)
{
    for (const TargetRules &target : makefile.targetRules)
    {
        if (target.name == name)
        {
            if (isDirty(target, timestamps) == false)
            {
                continue;
            }

            return getCommandVector(target.commands);
        };
    }
    return {};
}

void populateAllCurrentFiles(vector<string> &currentFiles)
{
    string path = "./";
    for (auto &e : filesystem::directory_iterator(path))
    {
        currentFiles.push_back(e.path().string());
    }
}

void printDebugMsg(string msg, bool &isDebug)
{
    if (isDebug == true)
    {
        cout << msg << endl;
    }
}

void cleanUp(vector<pid_t> &childProcesses, bool isSuccess = false, bool isDebug = false)
{
    if (childProcesses.size() > 0)
    {
        for (pid_t child : childProcesses)
        {
            if (child > 0)
            {
                if (kill(child, SIGTERM) == 0)
                {
                    printDebugMsg("Process terminated successfully.", isDebug);
                }
                else
                {
                    perror("kill");
                    printDebugMsg("Error terminating process.", isDebug);
                }
            }
        }
    }
    cout << "Exiting ..." << endl;
    if (isSuccess == false)
    {
        exit(EXIT_FAILURE);
    }
    else
    {
        exit(EXIT_SUCCESS);
    }
}

void handleCommandArgs(int argc, char *argv[],
                       char *optarg, int opt, const char *&makefileValue,
                       bool &isCustomMakefile, char *&timeValue, int &timeout,
                       bool &printOnly, bool &blockSignal, bool &isDebug, bool &continueExecution, vector<string> &targetList)
{
    switch (opt)
    {
    case 'f':
        makefileValue = optarg;
        isCustomMakefile = true;
        cout << "Option '-f' detected with argument: " << makefileValue << endl;
        break;
    case 't':
        timeValue = optarg;
        cout << "Option '-t' detected with argument: " << timeValue << endl;
        timeout = atoi(optarg);
        cout << "Timeout set to " << timeout << " seconds." << endl;
        break;
    case 'p':
        cout << "Option '-p' detected" << endl;
        printOnly = true;
        break;
    case 'd':
        cout << "Option '-d' detected" << endl;
        isDebug = true;
        break;
    case 'i':
        cout << "Option '-i' detected" << endl;
        blockSignal = true;
        break;
    case 'k':
        cout << "Option '-k' detected" << endl;
        continueExecution = true;
        break;

    default:
        break;
    }
}

int printMakefile(Makefile &myMakefile)
{
    cout << "Macros:" << endl;
    for (Macro m : myMakefile.macros)
    {
        cout << m.name << " = " << m.value_str << endl;
    }
    cout << endl;
    cout << "Target Rules: " << endl;
    for (TargetRules t : myMakefile.targetRules)
    {
        cout << t.name << ":" << endl;
        cout << "Prerequisites:";
        for (string p : t.prerequisites)
        {
            cout << p << " ";
        }
        cout << endl;
        cout << "Commands: " << endl;
        for (string c : t.commands)
        {
            cout << c << endl;
        }
    }
    return 0;
}

#endif // UTIL_CPP