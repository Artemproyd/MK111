#include "ops_generator.h"
#include <iostream>
#include <sstream>

OPSGenerator::OPSGenerator() {
    initCommandTable();
}

void OPSGenerator::initCommandTable() {
    // Инициализация таблицы команд для различных типов токенов
    commandTable["KEYWORD"] = {OPSCommandType::DECLARE, 2}; // тип, имя
    commandTable["IDENTIFIER"] = {OPSCommandType::DECLARE, 2}; // для объявления переменных
    commandTable["NUMBER"] = {OPSCommandType::PUSH_CONST, 1}; // значение
    
    // Операторы
    commandTable["PLUS"] = {OPSCommandType::ADD, 0};
    commandTable["MINUS"] = {OPSCommandType::SUB, 0};
    commandTable["MULTIPLY"] = {OPSCommandType::MUL, 0};
    commandTable["DIVIDE"] = {OPSCommandType::DIV, 0};
    
    // Присваивание
    commandTable["ASSIGN"] = {OPSCommandType::ASSIGN, 1}; // имя переменной

    // Сравнение
    commandTable["GT"] = {OPSCommandType::GT, 0}; // >
    commandTable["LT"] = {OPSCommandType::LT, 0}; // <
    commandTable["EQ"] = {OPSCommandType::EQ, 0}; // ==
}

void OPSGenerator::generateCommand(const std::string& tokenType, const std::string& tokenValue) {
    if (tokenType == "KEYWORD") {
        if (tokenValue == "if") {
            // Начало условного оператора if
            ifCount++;
            std::string labelElse = "L" + std::to_string(ifCount) + "_else";
            generatedCode.emplace_back(OPSCommandType::JZ, labelElse);
            return;
        }
        else {
            currentType = tokenValue;
            return;
        }
    }

    if (tokenType == "IDENTIFIER") {
        if (!currentType.empty()) {
            // Это объявление переменной - для примера "int x" создадим "x int"
            generatedCode.emplace_back(OPSCommandType::DECLARE, tokenValue, currentType);
            lastIdentifier = tokenValue;
            return;
        } else {
            // Это использование существующей переменной
            generatedCode.emplace_back(OPSCommandType::PUSH_VAR, tokenValue);
            lastIdentifier = tokenValue;
            return;
        }
    }

    if (tokenType == "OPERATOR") {
        if (tokenValue == "=") {
            if (!lastIdentifier.empty()) {
                generatedCode.emplace_back(OPSCommandType::ASSIGN, lastIdentifier);
            }
            return;
        } 
        else if (tokenValue == "+") {
            generatedCode.emplace_back(OPSCommandType::ADD);
            return;
        }
        else if (tokenValue == "-") {
            generatedCode.emplace_back(OPSCommandType::SUB);
            return;
        }
        else if (tokenValue == "*") {
            generatedCode.emplace_back(OPSCommandType::MUL);
            return;
        }
        else if (tokenValue == "/") {
            generatedCode.emplace_back(OPSCommandType::DIV);
            return;
        }
        else if (tokenValue == ">") {
            generatedCode.emplace_back(OPSCommandType::GT);
            return;
        }
        else if (tokenValue == "<") {
            generatedCode.emplace_back(OPSCommandType::LT);
            return;
        }
        else if (tokenValue == "==") {
            generatedCode.emplace_back(OPSCommandType::EQ);
            return;
        }
    }

    if (tokenType == "LEFT_BRACE") {
        // Начало блока кода
        return;
    }

    if (tokenType == "RIGHT_BRACE") {
        // Конец блока кода
        if (ifCount > 0) {
            std::string labelEnd = "L" + std::to_string(ifCount) + "_end";
            generatedCode.emplace_back(OPSCommandType::LABEL, labelEnd);
            ifCount--;
        }
        return;
    }

    if (tokenType == "NUMBER") {
        generatedCode.emplace_back(OPSCommandType::PUSH_CONST, tokenValue);
        return;
    }
}

std::vector<OPSCommand> OPSGenerator::getGeneratedCode() const {
    return generatedCode;
}

void OPSGenerator::reset() {
    generatedCode.clear();
    currentType.clear();
    lastIdentifier.clear();
    ifCount = 0;
} 