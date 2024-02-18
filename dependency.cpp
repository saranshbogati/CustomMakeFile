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

// enum class VisitState
// {
//     UNVISITED,
//     VISITED,
//     EXPLORING
// };

// bool endsWith(const string &str, const string &suffix)
// {
//     return str.size() >= suffix.size() &&
//            str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
// }

// vector<string> topologicalSort(const map<string, vector<string>> &graph, const vector<TargetRules> &targetRules, const vector<string> &targets = {})
// {
//     unordered_map<string, VisitState> visited;
//     stack<string> dfsStack;
//     vector<string> sorted;

//     for (const auto &[vertex, _] : graph)
//     {
//         if (visited[vertex] == VisitState::UNVISITED)
//         {
//             dfsStack.push(vertex);

//             while (!dfsStack.empty())
//             {
//                 string currentVertex = dfsStack.top();

//                 // Skip source files (keys ending in ".cpp")
//                 if (endsWith(currentVertex, ".cpp"))
//                 {
//                     dfsStack.pop();
//                     continue; // Move to the next iteration
//                 }

//                 // cout << currentVertex << endl;

//                 if (visited[currentVertex] == VisitState::UNVISITED)
//                 {
//                     visited[currentVertex] = VisitState::EXPLORING;

//                     // Check if the key exists in the graph before accessing it
//                     if (graph.find(currentVertex) != graph.end())
//                     {
//                         for (const auto &neighbor : graph.at(currentVertex))
//                         {
//                             if (visited[neighbor] == VisitState::EXPLORING)
//                             {
//                                 throw runtime_error("Cycle detected in the dependency graph!");
//                             }
//                             else if (visited[neighbor] == VisitState::UNVISITED)
//                             {
//                                 dfsStack.push(neighbor);
//                             }
//                         }
//                     }
//                 }
//                 else
//                 {
//                     visited[currentVertex] = VisitState::VISITED;
//                     sorted.push_back(currentVertex);
//                     dfsStack.pop();
//                 }
//             }
//         }
//     }

//     return sorted;
// }

// map<string, vector<string>> buildDependencyGraph(Makefile &myMakefile)
// {
//     map<string, vector<string>> dependencyGraph;
//     for (const TargetRules &rule : myMakefile.targetRules)
//     {
//         dependencyGraph[rule.name] = rule.prerequisites;
//     }
//     return dependencyGraph;
// }
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

    // After checking all the dependencies, execute the commands
    if (!rule.commands.empty())
    {
        DEBUG_COMMENT("Executing commands for target: " + currentTarget, isDebug);
        for (string cmd : rule.commands)
        {
            DEBUG_COMMENT("Executing command: " + cmd, isDebug);
            string output = "";
            run(cmd, output, isDebug);
        }
    }

    // Undefine the debug comment macro
}

#endif // DEPENDENCY_CPP
