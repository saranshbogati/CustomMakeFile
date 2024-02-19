#ifndef RUN_CPP
#define RUN_CPP

#include <string>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include "classes.h"
#include "print.cpp"
#include "run.h"

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
        filesystem::path commandPath = filesystem::path(dir) / command;
        if (filesystem::exists(commandPath) && filesystem::is_regular_file(commandPath))
        {
            return commandPath;
        }
    }
    return "";
}

string trim(const string &str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");

    if (start == string::npos || end == string::npos)
    {
        return "";
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

int executePipedCommand(const string &command, string &output, int (&prevPipe)[2], int (&nextPipe)[2], int counter, int totalCommands, bool isDebug)
{
    vector<string> commandTok = split(command, ' ');
    vector<const char *> c_args;

    for (const string &arg : commandTok)
    {
        c_args.push_back(arg.c_str());
    }
    c_args.push_back(nullptr);
    string path = c_args[0];
    if (c_args[0][0] != '/')
    {
        path = findPath(c_args[0]);
    }

    DEBUG_COMMENT("Executable path: " + path, isDebug);
    if (!path.empty())
    {
        const char *firstArg = path.c_str();

        pid_t id = fork();

        if (id == 0)
        {
            DEBUG_COMMENT("Executing command in the child process: " + to_string(counter), isDebug);
            if (counter > 0)
            {
                close(prevPipe[1]);
                dup2(prevPipe[0], STDIN_FILENO);
                close(prevPipe[0]);
            }
            if (counter < totalCommands)
            {
                close(nextPipe[0]);
                dup2(nextPipe[1], STDOUT_FILENO);
                close(nextPipe[1]);
            }
            execv(firstArg, const_cast<char *const *>(c_args.data()));
            DEBUG_COMMENT("Failed to execute command: ", isDebug);
            return -1;
        }
        else
        {
            DEBUG_COMMENT("Inside parent block", isDebug);
            if (counter > 0)
            {
                close(prevPipe[0]);
                close(prevPipe[1]);
            }

            if (counter < totalCommands)
            {
                pipe(prevPipe);
                prevPipe[0] = nextPipe[0];
                prevPipe[1] = nextPipe[1];
            }
            wait(NULL);
        }
    }
    else
    {
        DEBUG_COMMENT("Executable not found !!", isDebug);
        return -1;
    }
    return 0;
}

int run(const string &command, string &output, bool isDebug)
{
    DEBUG_COMMENT("Splitting command: " + command, isDebug);
    vector<string> tokens = split(command, ' ');
    vector<const char *> c_args;

    for (const string &arg : tokens)
    {
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
        int pipefd[2];
        pipe(pipefd);
        pid_t id = fork();
        if (id == 0)
        {
            DEBUG_COMMENT("Executing command in the child process: ", isDebug);
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execv(firstArg, const_cast<char *const *>(c_args.data()));
            DEBUG_COMMENT("Failed to execute command: ", isDebug);
            return -1;
        }
        else
        {
            close(pipefd[1]);
            waitpid(id, nullptr, 0);
            char buffer[2048];
            string res;
            ssize_t bytesRead;
            while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0)
            {
                res.append(buffer, bytesRead);
            }
            close(pipefd[0]);
            cout << "Output: " << res << endl;
            DEBUG_COMMENT("Child finished executing", isDebug);
            output = res;
        }
        close(pipefd[0]);
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
    DEBUG_COMMENT("Command : " + command, isDebug);

    if (command.find(';') != string::npos)
    {
        handleSemicolonCommand(isDebug, command, output);
    }
    else if (command.find('|') != string::npos)
    {
        handlePipedCommand(isDebug, command, output);
    }
    else if (command.find('<') != string::npos)
    {
        DEBUG_COMMENT("Commands separated by <", isDebug);
    }
    else if (command.find('>') != string::npos)
    {
        DEBUG_COMMENT("Commands separated by >", isDebug);
    }
    else
    {
        handleNormalCommand(isDebug, command, output);
    }

    return 0;
}

void handleNormalCommand(bool isDebug, const std::__1::string &command, std::__1::string &output)
{
    DEBUG_COMMENT("Normal command", isDebug);
    int res = run(command, output, isDebug);
    if (res != 0)
    {
        DEBUG_COMMENT("Error in executing command", isDebug);
    }
}

void handleSemicolonCommand(bool isDebug, const std::__1::string &command, std::__1::string &output)
{
    DEBUG_COMMENT("Semicolon found in command", isDebug);
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

void handlePipedCommand(bool isDebug, const std::__1::string &command, std::__1::string &output)
{
    DEBUG_COMMENT("Pipe command", isDebug);

    vector<string> pipedCommands = split(command, '|');
    int commandCounter = 0, totalCommands = pipedCommands.size();
    int prevPipeDescriptor[2] = {-1, -1}, nextPipeDescriptor[2];

    for (const string &currentCommand : pipedCommands)
    {
        pipe(nextPipeDescriptor);
        output = "";
        DEBUG_COMMENT("Command to run: " + currentCommand, isDebug);

        int executionResult = executePipedCommand(currentCommand, output, prevPipeDescriptor,
                                                  nextPipeDescriptor, commandCounter++, totalCommands, isDebug);

        if (executionResult != 0)
        {
            DEBUG_COMMENT("Error in executing command", isDebug);
            exit(EXIT_FAILURE);
        }
    }
    char buffer[128];
    string finalResult;
    ssize_t bytesRead;
    close(prevPipeDescriptor[1]);

    while ((bytesRead = read(prevPipeDescriptor[0], buffer, sizeof(buffer))) > 0)
    {
        finalResult.append(buffer, bytesRead);
    }

    close(prevPipeDescriptor[0]);
    cout << endl;
    cout << finalResult << endl;
}
#endif // RUN_CPP