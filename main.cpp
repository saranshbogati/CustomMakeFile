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
#include <cstdlib>

// Local files
#include "dependency.cpp"
#include "classes.cpp"
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
char *timeValue;
bool printOnly = false;
bool continueExecution = false;
bool blockSignal = false;
int timeout = 0;

// functions for handling signals
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

int main(int argc, char *argv[])
{
    int opt;
    const char *optstring = "f:t:ikpd";
    const char *makefileValue = "makefile";
    // cout << "Sleeping ...." << endl;

    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        handleCommandArgs(optarg, opt, makefileValue, isCustomMakefile, timeValue, timeout, printOnly, blockSignal, continueExecution);
    }

    // Set up signal handlers
    if (blockSignal)
    {
        signal(SIGINT, handleSIGINT);
    }

    // Set up timeout handler
    if (timeout > 0)
    {
        alarm(timeout);
        signal(SIGALRM, handleSIGALRM);
    }
    // sleep(200);

    // Populate timestamps for all current files
    populateAllCurrentFiles(currentFiles);
    for (const string &f : currentFiles)
    {
        filesystem::path filePath(f);
        string fileName = filePath.filename();
        timestamps[fileName] = getLastModifiedTime(f);
    }

    // Load makefile (custom path if desired)
    Makefile myMakefile;
    parseMakeFile(makefileValue, myMakefile);

    // Replace variables in commands
    for (TargetRules &rule : myMakefile.targetRules)
    {
        for (auto &command : rule.commands)
        {
            cout << "command" << command << endl;
            command = replaceVariables(command, myMakefile.macros, rule.name, rule.prerequisites);
        }
    }

    // Build dependency graph
    map<string, vector<string>> dependencyGraph = buildDependencyGraph(myMakefile);

    // Print makefile if requested
    if (printOnly)
    {
        return printMakefile(myMakefile);
    }

    // Handle specific targets or build all
    vector<string> targets;
    if (argc > 1)
    {
        targets.assign(argv + optind, argv + argc);
    }

    // Check if targets exist
    for (const string &target : targets)
    {
        if (dependencyGraph.find(target) == dependencyGraph.end())
        {
            cerr << "Error: Target '" << target << "' not found in the makefile." << endl;
            return 1;
        }
    }

    // Perform topological sort
    vector<string> buildOrder = topologicalSort(dependencyGraph, myMakefile.targetRules, targets);

    // Execute commands for each target in build order
    for (const string &name : buildOrder)
    {
        if (name == "clean")
        {
            continue; // Handle "clean" command separately
        }

        vector<string> commands = findTargetRuleByName(name, myMakefile, timestamps);
        if (commands.empty())
        {
            continue; // Skip targets without commands
        }

        for (string &command : commands)
        {
            // sleep(100); // Simulate work (replace with actual processing)
            int result = runCommand(command, childProcesses, continueExecution);

            // Handle command execution errors
            if (result != 0)
            {
                // ... (optional error handling)
            }
        }
    }

    // Handle "clean" command if present
    if (find(targets.begin(), targets.end(), "clean") != targets.end())
    {
        handleCleanCommand(myMakefile, continueExecution, childProcesses);
    }

    // Cleanup on timeout or program completion
    // cleanUp(childProcesses, true, isDebug);

    return 0;
}
#endif // MAIN_CPP
