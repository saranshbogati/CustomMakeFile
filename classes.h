#ifndef MAKEFILE_H
#define MAKEFILE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

using namespace std;

class Macro
{
public:
    string name;
    vector<string> value;
    string value_str;
};

class TargetRules
{
public:
    string name;
    vector<string> prerequisites;
    vector<string> commands;
};

class InferenceRule
{
public:
    string target;
    string source;
    vector<string> commands;
};

class Makefile
{
public:
    vector<Macro> macros;
    vector<TargetRules> targetRules;
    vector<InferenceRule> inferenceRules;
};

class Command
{
public:
    const char *firstArg;
    vector<char *> args;
};

class TargetDependency
{
public:
    unordered_map<string, vector<string>> graph;

    void addTarget(const string &target, const vector<string> &prerequisites)
    {
        string baseName = target.substr(0, target.rfind('.'));
        string extension = target.substr(target.rfind('.') + 1);

        string uniqueTarget = baseName + "_" + extension;

        dependencies[uniqueTarget] = prerequisites;
        for (const auto &prerequisite : prerequisites)
        {
            graph[uniqueTarget].push_back(prerequisite);
        }
    }

    void printGraph() const
    {
        for (const auto &entry : dependencies)
        {
            cout << entry.first << " depends on: ";
            for (const auto &prerequisite : entry.second)
            {
                cout << prerequisite << ' ';
            }
            cout << '\n';
        }
    }

private:
    unordered_map<string, vector<string>> dependencies;
};

#endif // MAKEFILE_H
