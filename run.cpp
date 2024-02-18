#ifndef RUN_CPP
#define RUN_CPP

#include <string>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include "classes.h"
#include "print.cpp"

// #include "run.cpp"

using namespace std;

string findPath(const string &command)
{
    char *path = getenv("PATH");
    if (path == nullptr)
    {
        return "";
    }
    stringstream ss(path);
    string dir;

    while (getline(ss, dir, ':'))
    {
        // Construct the full path to the command executable
        std::filesystem::path commandPath = filesystem::path(dir) / command;
        // Check if the command executable exists
        if (std::filesystem::exists(commandPath) && std::filesystem::is_regular_file(commandPath))
        {
            return commandPath;
        }
    }
    return "";
}

// int runCommand(string &command, vector<pid_t> &childProcesses, bool continueExecution)
// {
// (/ DEBUG_COMMENT / cout << "Command to run: " << command, isDebug);
//     if (command.substr(0, 3) == "cd ")
//     {
//         string directory = command.substr(3);

//         if (chdir(directory.c_str()) != 0)
//         {
//             perror("chdir");
//             exit(EXIT_FAILURE);
//         }

//         return 0;
//     }
//     if (command[0] == '@')
//     {
//         command = command.substr(1); // Skip the @ symbol
//     }
//     pid_t pid = fork();
//     childProcesses.push_back(pid);

//     if (pid == -1)
//     {
//         perror("fork");
//         return -1;
//     }
//     else if (pid == 0)
//     {
//         // Child process
//         // Split the command into arguments
//         vector<char *> args;
//         char *token = strtok(const_cast<char *>(command.c_str()), " ");
//         while (token != nullptr)
//         {
//             args.push_back(token);
//             token = strtok(nullptr, " ");
//         }
//         args.push_back(nullptr); // Null-terminate the array

//         // cout << "Executing from the child process" << endl;

//         // Execute the command in the child process
//         string executablePath = findPath(args[0]);
//         // cout << "Executable path is " << executablePath << endl;
//         const char *firstArg = executablePath.c_str();

//         execv(firstArg, args.data());

//         // If execv fails, print an error message
//         perror("execv");
//         exit(EXIT_FAILURE);
//     }
//     else
//     {
//         // Parent process
//         int status;
//         // cout << "Parent process waiting for child process (PID: " << pid << ")" << endl;
//         waitpid(pid, &status, 0);
//         // cout << "Exiting from the parent process. Waiting for the child process (PID: " << pid << ") finished" << endl;
//         // cout << "Status from the child process (PID: " << pid << ") is " << status << endl;

//         // Return the exit status of the child process
//         if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
//         {
//             // Log the error
//             cerr << "Error: Command execution failed" << endl;

//             if (continueExecution)
//             {
//                 // cout << "Continuing execution after failure." << endl;
//                 return 0; // Return success to indicate continuation
//             }
//             else
//             {
//                 // cout << "Exiting due to failure." << endl;
//                 exit(EXIT_FAILURE);
//             }
//         }
//     }
//     return 0;
// }

// int handleCleanCommand(Makefile &myMakefile, bool continueExecution, vector<pid_t> &childProcesses)
// {
//     for (auto &t : myMakefile.targetRules)
//     {
//         string name = t.name;
//         if (name == "clean")
//         {
//             runCommand(t.commands[0], childProcesses, continueExecution);
//             return 0;
//         }
//     }
//     return {};
// }
string trim(const string &str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");

    if (start == string::npos || end == string::npos)
    {
        return ""; // String contains only whitespace
    }

    return str.substr(start, end - start + 1);
}

vector<string> split(const string &command, const char &delimiter)
{
    vector<string> tokens;
    stringstream iss(command);
    string token;

    while (getline(iss, token, delimiter))
    {
        if (!token.empty())
        {
            token = trim(token);
            tokens.push_back(token);
        }
    }
    return tokens;
}
int run(const string &command, string &output, bool isDebug)
{
    vector<string> tokens = split(command, ' ');
    vector<const char *> c_args;

    for (const string &arg : tokens)
    {
        // cout << "Arg: " << arg << endl;
        c_args.push_back(arg.c_str());
    }
    c_args.push_back(nullptr);
    string executablePath = c_args[0];
    if (c_args[0][0] != '/')
    {
        executablePath = findPath(c_args[0]);
    }

    DEBUG_COMMENT("Executable path: " + executablePath, isDebug);
    if (!executablePath.empty())
    {
        const char *firstArg = executablePath.c_str();
        // Execv creates replace the current process with the new process
        // So create the child process and run execv in the chid
        // the parent waits till the child completes its execution
        int fd[2];
        pipe(fd);
        pid_t id = fork();
        if (id == 0)
        {
            DEBUG_COMMENT("Executing command in the child process: ", isDebug);
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO); // Redirect stdout to the pipe
            close(fd[1]);
            execv(firstArg, const_cast<char *const *>(c_args.data()));
            DEBUG_COMMENT("Failed to execute command: ", isDebug);
            return -1;
        }
        else
        {
            close(fd[1]);
            waitpid(id, nullptr, 0);
            char buffer[2048];
            string res;
            ssize_t bytesRead;
            while ((bytesRead = read(fd[0], buffer, sizeof(buffer))) > 0)
            {
                res.append(buffer, bytesRead);
            }
            close(fd[0]);
            DEBUG_COMMENT("Buffer: " + res, isDebug);
            DEBUG_COMMENT("Child finished executing", isDebug);
            output = res;
        }
    }
    else
    {
        DEBUG_COMMENT("Cannot find executable ", isDebug);
        return -1;
    }
    return 0;
}

int handleCommandMultiple(const string &command, bool isDebug)
{
    string output;
    DEBUG_COMMENT("Cmd: " + command, isDebug);

    // Handle commands separated by ;
    if (command.find(';') != ::string::npos)
    {
        DEBUG_COMMENT("Commands separated by ;", isDebug);
        vector<string> commands = split(command, ';');
        for (const string &cmd : commands)
        {
            int res = run(cmd, output, isDebug);
            if (res != 0)
            {
                DEBUG_COMMENT("Error in executing command", isDebug);
            }
        }
    }
    // Handle pipes(|)
    else if (command.find('|') != ::string::npos)
    {
        DEBUG_COMMENT("Commands separated by |", isDebug);
        vector<string> commands = split(command, '|');
        for (const string &cmd : commands)
        {
            string cmdWithOutput = cmd + " " + output;
            output = "";
            DEBUG_COMMENT("Pipe cmd: " + cmdWithOutput, isDebug);
            int res = run(cmdWithOutput, output, isDebug);
            if (res != 0)
            {
                DEBUG_COMMENT("Error in executing command", isDebug);
            }
        }
    }
    // Handle IO redirection commands (<, >)
    else if (command.find('<') != ::string::npos)
    {
        DEBUG_COMMENT("Commands separated by <", isDebug);
    }
    else if (command.find('>') != ::string::npos)
    {
        DEBUG_COMMENT("Commands separated by >", isDebug);
    }
    // Handle normal commands
    else
    {
        DEBUG_COMMENT("Normal command", isDebug);
        int res = run(command, output, isDebug);
        if (res != 0)
        {
            DEBUG_COMMENT("Error in executing command", isDebug);
        }
    }

    DEBUG_COMMENT("Output of the previous command: " + output, isDebug);
    return 0;
}

#endif // RUN_CPP