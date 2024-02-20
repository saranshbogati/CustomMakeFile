#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include <filesystem>
#include "classes.h"
#include "run.cpp"
using namespace std;

void handleError(const string &error)
{
    cerr << "Error: " << error << endl;
}

vector<string> splitAndTrim(const string &input)
{
    istringstream iss(input);
    vector<string> result;

    transform(istream_iterator<string>(iss),
              istream_iterator<string>(),
              back_inserter(result),
              [](const string &s)
              {
                  string token = s;
                  token.erase(token.begin(), find_if(token.begin(), token.end(), [](unsigned char ch)
                                                     { return !isspace(ch); }));
                  token.erase(find_if(token.rbegin(), token.rend(), [](unsigned char ch)
                                      { return !isspace(ch); })
                                  .base(),
                              token.end());
                  return token;
              });

    return result;
}
string trimWhitespace(const string &input)
{
    size_t start = input.find_first_not_of(" \t\n\r\f\v");
    size_t end = input.find_last_not_of(" \t\n\r\f\v");

    if (start == string::npos || end == string::npos)
    {
        return "";
    }

    return input.substr(start, end - start + 1);
}

void handleMacros(const string &line, Makefile &makefile)
{
    size_t pos = line.find('=');
    if (pos == string::npos || pos == 0 || pos == line.size() - 1)
    {
        handleError("Invalid macro definition: " + line);
        return;
    }
    string name = line.substr(0, pos);
    string value = line.substr(pos + 1);

    name = trimWhitespace(name);
    value = trimWhitespace(value);

    Macro macro;
    macro.name = name;
    macro.value_str = value;
    macro.value.push_back(value);

    makefile.macros.push_back(macro);
}

TargetRules handleTargetRules(const string &line, Makefile &makefile)
{
    istringstream iss(line);
    string targetName;
    string token;

    iss >> targetName;
    TargetRules targetRule;
    targetRule.name = targetName.substr(0, targetName.size() - 1);
    ;

    iss >> ws;

    while (iss >> token && token != ":")
    {
        targetRule.prerequisites.push_back(token);
    }

    while (iss >> token)
    {
        targetRule.commands.push_back(token);
    }

    makefile.targetRules.push_back(targetRule);
    return targetRule;
}

InferenceRule handleInferenceRules(const string &line, Makefile &makefile)
{
    istringstream iss(line);
    string targetAndSource;
    string token;

    iss >> targetAndSource;
    InferenceRule inferenceRule;
    size_t pos = targetAndSource.find(':');

    if (pos != string::npos)
    {
        inferenceRule.target = targetAndSource.substr(0, pos);
        inferenceRule.source = targetAndSource.substr(pos + 1);
    }

    while (iss >> token)
    {
        inferenceRule.commands.push_back(token);
    }

    makefile.inferenceRules.push_back(inferenceRule);
    return inferenceRule;
}

void parseMakeFile(const string &filename, Makefile &makefile)
{
    ifstream file(filename);

    if (!file.is_open())
    {
        cerr << "Error: Unable to open the makefile " << filename << endl;
        return;
    }

    string line;
    TargetRules currentTargetRule;
    InferenceRule currentInferenceRule;
    bool processingInferenceRule = false;
    bool processingTargetRule = false;
    const regex inferencePat("^(\\.\\w+)(\\.\\w+)?\\s*:");
    smatch t;

    while (getline(file, line))
    {
        if (line.find('=') != string::npos)
        {
            handleMacros(line, makefile);
        }
        else if (regex_match(line, t, inferencePat) || line.find('%') != string::npos)
        {
            currentInferenceRule = handleInferenceRules(line, makefile);
            currentTargetRule = TargetRules();
            processingInferenceRule = true;
            processingTargetRule = false;
        }
        else if (line.find(':') != string::npos)
        {
            TargetRules addedTargetRule = handleTargetRules(line, makefile);
            currentTargetRule = addedTargetRule;
            processingTargetRule = true;
            processingInferenceRule = false;
        }
        else if (!line.empty() && line[0] == '#')
        {
            currentTargetRule = TargetRules();
            currentInferenceRule = InferenceRule();
            if ((processingTargetRule || processingInferenceRule))
            {
                processingTargetRule = !processingTargetRule;
                processingInferenceRule = !processingInferenceRule;
            }
            continue;
        }
        else if (line.empty())
        {
            currentTargetRule = TargetRules();
            currentInferenceRule = InferenceRule();
            if (processingTargetRule || processingInferenceRule)
            {
                processingTargetRule = !processingTargetRule;
                processingInferenceRule = !processingInferenceRule;
            }

            continue;
        }
        else if (processingTargetRule)
        {
            string command = trimWhitespace(line);
            currentTargetRule.commands.push_back(command);
            makefile.targetRules.pop_back();
            makefile.targetRules.push_back(currentTargetRule);
        }
        else if (processingInferenceRule)
        {
            string command = trimWhitespace(line);
            currentInferenceRule.commands.push_back(command);
            makefile.inferenceRules.pop_back();
            makefile.inferenceRules.push_back(currentInferenceRule);
        }
        else
        {
        }
    }

    file.close();
}
string replaceVariables(const string &str, const vector<Macro> &macros, const string &target, const vector<string> &prerequisites)
{
    string result = str;
    string source = prerequisites.empty() ? "" : prerequisites[0];

    for (const auto &macro : macros)
    {
        string var_name = macro.name;

        regex pat("\\$\\{" + var_name + "\\}");
        result = regex_replace(result, pat, macro.value_str);

        regex pat1("\\$\\(" + var_name + "\\)");
        result = regex_replace(result, pat1, macro.value_str);

        regex newPat("\\$" + var_name);
        result = regex_replace(result, newPat, macro.value_str);
    }

    if (result.find('$') == string::npos)
    {
        return result;
    }
    if (!source.empty())
    {
        regex input_pat("\\$<");
        result = regex_replace(result, input_pat, source);
    }

    regex output_pat("\\$@");
    result = regex_replace(result, output_pat, target);

    regex all_prerequisites_pat("\\$\\^");
    string all_prerequisites;
    for (const auto &prereq : prerequisites)
    {
        all_prerequisites += prereq + " ";
    }
    result = regex_replace(result, all_prerequisites_pat, all_prerequisites);
    return result;
}

string replaceMacroVars(const string &input, vector<Macro> macros)
{
    string result = input;
    for (auto macro : macros)
    {
        string pat = "\\$" + macro.name + "|\\$\\(" + macro.name + "\\)";
        regex pattern(pat);
        result = regex_replace(result, pattern, macro.value_str);
    }
    return result;
}

void handleInferenceRulesFile(Makefile makefile, vector<int> childProcesses, bool continueExecution, bool isDebug)
{

    const regex inferenceRegex("^(\\.\\w+)(\\.\\w+)?\\s*");
    if (!makefile.inferenceRules.empty())
    {
        for (auto inferenceRule : makefile.inferenceRules)
        {
            auto target = inferenceRule.target;
            cout << "INF RULE: " << inferenceRule.target << endl;

            smatch match;
            regex_match(inferenceRule.target, match, inferenceRegex);
            string source_ext = match[1];
            string target_ext = "";
            if (match.size() > 2)
            {
                target_ext = match[2];
            }
            for (const auto &entry : filesystem::directory_iterator(filesystem::current_path()))
            {
                if (entry.is_regular_file() && entry.path().extension() == source_ext)
                {
                    string source_file_stem = relative(entry.path().stem(), filesystem::current_path());
                    string source_file = source_file_stem + source_ext;
                    string target_file = source_file_stem + target_ext;

                    if (!inferenceRule.commands.empty())
                    {
                        for (string &cmd : inferenceRule.commands)
                        {
                            string command;
                            command = replaceVariables(cmd, makefile.macros, target_file, {source_file});
                            string output = "";
                            run(command, output, isDebug, continueExecution);
                        }
                    }
                }
            }
        }
    }
}
