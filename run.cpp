#ifndef RUN_CPP
#define RUN_CPP
#include <string>
#include <iostream>
#include <unistd.h>

#include "util.cpp"
using namespace std;

int runCommand(string &command, vector<pid_t> &childProcesses, bool continueExecution)
{
    cout << "Command to run: " << command << endl;
    if (command.substr(0, 3) == "cd ")
    {
        string directory = command.substr(3);

        if (chdir(directory.c_str()) != 0)
        {
            perror("chdir");
            exit(EXIT_FAILURE);
        }

        return 0;
    }
    if (command[0] == '@')
    {
        command = command.substr(1); // Skip the @ symbol
    }
    pid_t pid = fork();
    childProcesses.push_back(pid);

    if (pid == -1)
    {
        perror("fork");
        return -1;
    }
    else if (pid == 0)
    {
        // Child process
        // Split the command into arguments
        vector<char *> args;
        char *token = strtok(const_cast<char *>(command.c_str()), " ");
        while (token != nullptr)
        {
            args.push_back(token);
            token = strtok(nullptr, " ");
        }
        args.push_back(nullptr); // Null-terminate the array

        cout << "Executing from the child process" << endl;

        // Execute the command in the child process
        string executablePath = findPath(args[0]);
        cout << "Executable path is " << executablePath << endl;
        const char *firstArg = executablePath.c_str();

        execv(firstArg, args.data());

        // If execv fails, print an error message
        perror("execv");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process
        int status;
        cout << "Parent process waiting for child process (PID: " << pid << ")" << endl;
        waitpid(pid, &status, 0);
        cout << "Exiting from the parent process. Waiting for the child process (PID: " << pid << ") finished" << endl;
        cout << "Status from the child process (PID: " << pid << ") is " << status << endl;

        // Return the exit status of the child process
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            // Log the error
            cerr << "Error: Command execution failed" << endl;

            if (continueExecution)
            {
                cout << "Continuing execution after failure." << endl;
                return 0; // Return success to indicate continuation
            }
            else
            {
                cout << "Exiting due to failure." << endl;
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}

int handleCleanCommand(Makefile &myMakefile, bool continueExecution, vector<pid_t> &childProcesses)
{
    for (auto &t : myMakefile.targetRules)
    {
        string name = t.name;
        if (name == "clean")
        {
            runCommand(t.commands[0], childProcesses, continueExecution);
            return 0;
        }
    }
    return {};
}

#endif // RUN_CPP