#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <regex>
#include "classes.cpp"
#include "parser.cpp"
#include "dependency.cpp"
#include <chrono>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;
// namespace fs = filesystem;
unordered_map<string, filesystem::file_time_type> timestamps;
vector<string> currentFiles;

bool isDirty(TargetRules rule)
{
    filesystem::file_time_type sourceTimestamp = timestamps[rule.name];
    for (string &r : rule.prerequisites)
    {
        filesystem::file_time_type prerequisiteTimeStamp = timestamps[r];
        if (prerequisiteTimeStamp > sourceTimestamp)
        {
            return true;
        }
    }
    return false;
};

string findTargetRuleByName(string name, Makefile makefile)
{
    for (const TargetRules &target : makefile.targetRules)
    {
        if (target.name == name)
        {
            if (isDirty(target) == false)
            {
                continue;
            }
            string command = "";
            for (const string &cmd : target.commands)
            {
                command += cmd + ';';
            }
            if (!command.empty() && command.back() == ';')
            {
                command.pop_back();
            }
            return command;
        };
    }
    return "";
}

string findCommandPath(const string &command)
{
    string whichCommand = "which " + command;
    FILE *pipe = popen(whichCommand.c_str(), "r");
    if (!pipe)
    {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    char buffer[128];
    string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }

    if (pclose(pipe) == -1)
    {
        perror("pclose");
        exit(EXIT_FAILURE);
    }

    // Remove trailing newline characters
    result.erase(result.find_last_not_of("\n") + 1);

    return result;
}

int runCommand(const std::string &command)
{
    std::cout << "Command to run: " << command << std::endl;

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return -1;
    }
    else if (pid > 0)
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        cout << "Exiting from the parent process. Waiting for the child process" << endl;
        // Return the exit status of the child process
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
    else
    {
        // First child process
        pid_t childPid = fork();

        if (childPid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (childPid > 0)
        {
            // Exit the first child process
            exit(EXIT_SUCCESS);
        }
        else
        {
            // Second child process
            // Split the command into arguments
            std::vector<char *> args;
            char *token = strtok(const_cast<char *>(command.c_str()), " ");
            while (token != nullptr)
            {
                args.push_back(token);
                token = strtok(nullptr, " ");
            }
            args.push_back(nullptr); // Null-terminate the array
            cout << "Executing from the child process" << endl;
            // Execute the command in the second child process
            string executablePath = findCommandPath(args[0]);
            cout << "Executable path is " << executablePath << endl;
            const char *firstArg = executablePath.c_str();
            execv(firstArg, args.data());
            perror("execv");
            exit(EXIT_FAILURE);
        }
    }
}

filesystem::file_time_type getLastModifiedTime(const string &filename)
{
    return filesystem::last_write_time(filename);
}

void populateAllCurrentFiles()
{
    string path = "./";
    for (auto &e : filesystem::directory_iterator(path))
    {
        currentFiles.push_back(e.path().string());
    }
}

int main(int argc, char *argv[])
{
    populateAllCurrentFiles();
    for (const string &f : currentFiles)
    {
        filesystem::path filePath(f);
        string fileName = filePath.filename();
        timestamps[fileName] = getLastModifiedTime(f);
    }
    Makefile myMakefile;
    parseMakeFile("makefile", myMakefile);
    // Replace variables in commands
    for (auto &targetRule : myMakefile.targetRules)
    {
        for (auto &command : targetRule.commands)
        {
            command = replace_variables(command, myMakefile.macros, targetRule.name, targetRule.prerequisites, targetRule.prerequisites[0]);
        }
    }

    if (argc > 1)
    {
        if (strcmp(argv[1], "clean") == 0)
        {
            // find the clean command and execute only the clean command
            // cout << "Executing only the clean command" << endl;
            for (auto &t : myMakefile.targetRules)
            {
                string name = t.name;
                if (name == "clean")
                {
                    runCommand(t.commands[0]);
                    return 0;
                }
            }
        }
        // cout << "Argc is greater than one" << endl;
        vector<string> targets(argv + 1, argv + argc);
        for (auto &c : targets)
        {
            cout << "Target: " << c << endl;
        };

        map<string, vector<string>> dependencyGraph;
        for (const TargetRules &rule : myMakefile.targetRules)
        {
            dependencyGraph[rule.name] = rule.prerequisites;
        }

        // Check if the specified targets exist in the dependency graph
        for (const string &target : targets)
        {
            if (dependencyGraph.find(target) == dependencyGraph.end())
            {
                cerr << "Error: Target " << target << " not found in the makefile." << endl;
                return 1; // Return an error code
            }
        }

        vector<string> buildOrder = topologicalSort(dependencyGraph, myMakefile.targetRules, targets);
        for (const string &name : buildOrder)
        {
            // cout << "build name: " << name << endl;
            string command = findTargetRuleByName(name, myMakefile);
            if (!command.empty())
            {
                int result = system(command.c_str());

                // Check the result of the command execution
                if (result != 0)
                {
                    // cerr << "Error: Command execution failed for target " << name << endl;
                    // Handle the error as needed
                }
            }
            else
            {
                // cout << "Thukka muji. No need to execute this command";
                continue;
            }
        }
    }
    else
    {
        // If no arguments provided, build all targets
        map<string, vector<string>> dependencyGraph;
        for (const TargetRules &rule : myMakefile.targetRules)
        {
            dependencyGraph[rule.name] = rule.prerequisites;
        }

        vector<string> buildOrder = topologicalSort(dependencyGraph, myMakefile.targetRules);
        for (const string &name : buildOrder)
        {
            string command = findTargetRuleByName(name, myMakefile);
            if (!command.empty())
            {
                int result = runCommand(command);

                // Check the result of the command execution
                if (result != 0)
                {
                    // cerr << "Error: Command execution failed for target " << name << endl;
                    // Handle the error as needed
                }
            }
            else
            {
                // cout << "No need to run command since its not dirty. Thukka muji " << endl;
            }
        }
    }

    return 0;
}
