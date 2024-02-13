// classes
#pragma once
#include <string>
#include <vector>
#include <queue>
#include <map>

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

class TargetDependency
{
public:
    unordered_map<string, vector<string>> graph;

    void addTarget(const string &target, const vector<string> &prerequisites)
    {
        // Extract base name and extension
        string baseName = target.substr(0, target.rfind('.'));
        string extension = target.substr(target.rfind('.') + 1);

        // Use unique identifier for each combination
        string uniqueTarget = baseName + "_" + extension;

        dependencies[uniqueTarget] = prerequisites;
        for (const auto &prerequisite : prerequisites)
        {
            // Update graph with unique identifiers
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