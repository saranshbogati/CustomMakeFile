#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include "classes.cpp"

using namespace std;

void handleError(const string &error)
{
    cerr << "Error: " << error << endl;
}

vector<string> splitAndTrim(const string &input)
{
    istringstream iss(input);
    string token;
    vector<string> result;

    while (getline(iss >> ws, token, ' '))
    {
        result.push_back(token);
    }

    return result;
}
string trimWhitespace(const string &input)
{
    size_t start = input.find_first_not_of(" \t\n\r\f\v");
    size_t end = input.find_last_not_of(" \t\n\r\f\v");

    if (start == string::npos || end == string::npos)
    {
        // The string is either empty or contains only whitespaces
        return "";
    }

    return input.substr(start, end - start + 1);
}

void handleMacros(const string &line, Makefile &makefile)
{
    istringstream iss(line);
    string name, equalSign, value;

    iss >> ws >> name >> equalSign;

    if (equalSign != "=")
    {
        handleError("Invalid macro definition: " + line);
        return;
    }
    getline(iss, value);
    vector<string> value_list = splitAndTrim(value);
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

void handleInferenceRules(const string &line, Makefile &makefile)
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
    else
    {
        handleError("Invalid format for inference rule: " + line);
        return;
    }

    // Extract and store commands
    while (iss >> token)
    {
        inferenceRule.commands.push_back(token);
    }

    makefile.inferenceRules.push_back(inferenceRule);
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
    bool isInitial = false;

    while (getline(file, line))
    {
        if (line.find('=') != string::npos)
        {
            handleMacros(line, makefile);
        }
        else if (line.find('%') != string::npos)
        {
            handleInferenceRules(line, makefile);
            currentTargetRule = TargetRules();
        }
        else if (line.find(':') != string::npos)
        {
            TargetRules addedTargetRule = handleTargetRules(line, makefile);
            currentTargetRule = addedTargetRule;
            isInitial = true;
        }
        else if (!line.empty() && line[0] == '#')
        {
            currentTargetRule = TargetRules();
            continue;
        }
        else if (line.empty())
        {
            currentTargetRule = TargetRules();

            continue;
        }
        else if ((!currentTargetRule.commands.empty() && !line.empty()) || isInitial == true)
        {
            string command = trimWhitespace(line);
            currentTargetRule.commands.push_back(command);
            makefile.targetRules.pop_back();
            makefile.targetRules.push_back(currentTargetRule);
            // currentTargetRule = TargetRules();
            isInitial = false;
        }
        else
        {
            // Handle other cases or report an error if needed
            // cerr << "Unexpected line: " << line << endl;
        }
    }

    file.close();
}

string replace_variables(const string &str, const vector<Macro> &macros, const string &target, const vector<string> &prerequisites, const string &source)
{
    string result = str;

    // Replace ${var} format
    for (const auto &macro : macros)
    {
        string var_name = macro.name;
        regex pat("\\$\\{" + var_name + "\\}");
        result = regex_replace(result, pat, macro.value_str);
    }
    for (const auto &macro : macros)
    {
        string var_name = macro.name;
        regex pat("\\$" + var_name);
        result = regex_replace(result, pat, macro.value_str);
    }

    if (str.find('$') == string::npos)
    {
        return str; // No replacements needed
    }
    // Replace $< with source file
    regex input_pat("\\$<");
    result = regex_replace(result, input_pat, source);

    // Replace $@ with target file
    regex output_pat("\\$@");
    result = regex_replace(result, output_pat, target);

    // Replace $^ with space-separated prerequisites
    regex all_prerequisites_pat("\\$\\^");
    string all_prerequisites;
    for (const auto &prereq : prerequisites)
    {
        all_prerequisites += prereq + " ";
    }
    result = regex_replace(result, all_prerequisites_pat, all_prerequisites);

    // Add more replacements as needed...

    return result;
}
