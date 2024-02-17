#ifndef DEPENDENCY_CPP
#define DEPENDENCY_CPP

#include <vector>
#include <unordered_map>
#include <stack>
#include <string>
#include "classes.cpp"

using namespace std;

enum class VisitState
{
    UNVISITED,
    VISITED,
    EXPLORING
};
bool endsWith(const string &str, const string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

vector<string> topologicalSort(const map<string, vector<string>> &graph, const vector<TargetRules> &targetRules, const vector<string> &targets = {})
{
    unordered_map<string, VisitState> visited;
    stack<string> dfsStack;
    vector<string> sorted;

    for (const auto &[vertex, _] : graph)
    {
        if (visited[vertex] == VisitState::UNVISITED)
        {
            dfsStack.push(vertex);

            while (!dfsStack.empty())
            {
                string currentVertex = dfsStack.top();

                // Skip source files (keys ending in ".cpp")
                if (endsWith(currentVertex, ".cpp"))
                {
                    dfsStack.pop();
                    continue; // Move to the next iteration
                }

                // cout << currentVertex << endl;

                if (visited[currentVertex] == VisitState::UNVISITED)
                {
                    visited[currentVertex] = VisitState::EXPLORING;

                    for (const auto &neighbor : graph.at(currentVertex))
                    {
                        if (visited[neighbor] == VisitState::EXPLORING)
                        {
                            throw runtime_error("Cycle detected in the dependency graph!");
                        }
                        else if (visited[neighbor] == VisitState::UNVISITED)
                        {
                            dfsStack.push(neighbor);
                        }
                    }
                }
                else
                {
                    visited[currentVertex] = VisitState::VISITED;
                    sorted.push_back(currentVertex);
                    dfsStack.pop();
                }
            }
        }
    }

    return sorted;
}

map<string, vector<string>> buildDependencyGraph(Makefile &myMakefile)
{
    map<string, vector<string>> dependencyGraph;
    for (const TargetRules &rule : myMakefile.targetRules)
    {
        dependencyGraph[rule.name] = rule.prerequisites;
    }
    return dependencyGraph;
}

#endif // DEPENDENCY_CPP