#include "ops_interpreter.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cctype>

OPSInterpreter::OPSInterpreter() : programCounter(0), running(false) {}

void OPSInterpreter::execute(const std::vector<std::string>& opsCommands) {
    if (opsCommands.empty()) {
        std::cout << "❌ Нет команд для выполнения!" << std::endl;
        return;
    }
    
    commands = opsCommands;
    programCounter = 0;
    running = true;
    
    // Сначала парсим метки
    parseLabels();
    
    std::cout << "\n🔄 ВЫПОЛНЕНИЕ ОПС:" << std::endl;
    std::cout << "Команды: ";
    for (size_t i = 0; i < commands.size(); ++i) {
        std::cout << commands[i];
        if (i < commands.size() - 1) std::cout << " ";
    }
    std::cout << "\n" << std::string(50, '-') << std::endl;
    
    // Основной цикл выполнения
    while (running && programCounter < commands.size()) {
        const std::string& command = commands[programCounter];
        
        std::cout << "PC=" << programCounter << ": " << command;
        
        // Пропускаем метки при выполнении
        if (command.back() == ':') {
            std::cout << " (метка)" << std::endl;
            programCounter++;
            continue;
        }
        
        // Обрабатываем команду
        if (isNumber(command)) {
            // Число - помещаем в стек
            int value = std::stoi(command);
            pushStack(value);
            std::cout << " → стек: " << value;
        }
        else if (isVariable(command)) {
            // Переменная - помещаем её значение в стек
            int value = getVariable(command);
            pushStack(value);
            std::cout << " → стек: " << command << "=" << value;
        }
        else if (isOperator(command)) {
            // Арифметическая операция или сравнение
            if (command == "+" || command == "-" || command == "*" || command == "/") {
                executeArithmetic(command);
            }
            else if (command == ">" || command == "<" || command == "==") {
                executeComparison(command);
            }
            std::cout << " → результат в стеке";
        }
        else if (command == ":=") {
            // Присваивание
            executeAssignment();
            std::cout << " → присваивание";
        }
        else if (command == "jf") {
            // Условный переход - метка должна быть предыдущей командой
            if (programCounter > 0) {
                std::string label = commands[programCounter - 1];
                executeConditionalJump(label);
                std::cout << " → условный переход к " << label;
                continue; // programCounter уже изменен в executeConditionalJump
            }
        }
        else if (command == "j") {
            // Безусловный переход - метка должна быть предыдущей командой
            if (programCounter > 0) {
                std::string label = commands[programCounter - 1];
                executeJump(label);
                std::cout << " → безусловный переход к " << label;
                continue; // programCounter уже изменен в executeJump
            }
        }
        else {
            // Возможно метка для перехода - просто продолжаем
            std::cout << " (пропуск)";
        }
        
        std::cout << std::endl;
        programCounter++;
    }
    
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "✅ Выполнение завершено!" << std::endl;
    printState();
}

void OPSInterpreter::parseLabels() {
    labels.clear();
    for (size_t i = 0; i < commands.size(); ++i) {
        const std::string& command = commands[i];
        if (command.back() == ':') {
            // Это метка
            std::string labelName = command.substr(0, command.length() - 1);
            labels[labelName] = i;
        }
    }
}

bool OPSInterpreter::isNumber(const std::string& str) const {
    if (str.empty()) return false;
    
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        start = 1;
        if (str.length() == 1) return false;
    }
    
    for (size_t i = start; i < str.length(); ++i) {
        if (!std::isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

bool OPSInterpreter::isVariable(const std::string& str) const {
    if (str.empty()) return false;
    if (!std::isalpha(str[0]) && str[0] != '_') return false;
    
    for (size_t i = 1; i < str.length(); ++i) {
        if (!std::isalnum(str[i]) && str[i] != '_') {
            return false;
        }
    }
    return true;
}

bool OPSInterpreter::isOperator(const std::string& str) const {
    return str == "+" || str == "-" || str == "*" || str == "/" ||
           str == ">" || str == "<" || str == "==";
}

bool OPSInterpreter::isCommand(const std::string& str) const {
    return str == ":=" || str == "jf" || str == "j" || str == "r" || str == "w" ||
           str == "m1" || str == "m2" || str == "i";
}

void OPSInterpreter::executeArithmetic(const std::string& op) {
    if (operandStack.size() < 2) {
        error("Недостаточно операндов для операции " + op);
    }
    
    int b = popStack(); // Второй операнд
    int a = popStack(); // Первый операнд
    int result = 0;
    
    if (op == "+") {
        result = a + b;
    } else if (op == "-") {
        result = a - b;
    } else if (op == "*") {
        result = a * b;
    } else if (op == "/") {
        if (b == 0) {
            error("Деление на ноль");
        }
        result = a / b;
    }
    
    pushStack(result);
}

void OPSInterpreter::executeComparison(const std::string& op) {
    if (operandStack.size() < 2) {
        error("Недостаточно операндов для сравнения " + op);
    }
    
    int b = popStack(); // Второй операнд
    int a = popStack(); // Первый операнд
    int result = 0;
    
    if (op == ">") {
        result = (a > b) ? 1 : 0;
    } else if (op == "<") {
        result = (a < b) ? 1 : 0;
    } else if (op == "==") {
        result = (a == b) ? 1 : 0;
    }
    
    pushStack(result);
}

void OPSInterpreter::executeAssignment() {
    if (operandStack.size() < 2) {
        error("Недостаточно операндов для присваивания");
    }
    
    int value = popStack();       // Значение для присваивания
    
    // В стеке должно остаться имя переменной или её адрес
    // Но в нашем случае переменная уже была обработана как число в стеке
    // Нужно найти имя переменной перед операцией присваивания
    
    std::string varName = "";
    
    // Ищем переменную перед текущей позицией (перед ":=")
    for (int i = static_cast<int>(programCounter) - 1; i >= 0; --i) {
        if (isVariable(commands[i]) && commands[i] != "jf" && commands[i] != "j") {
            varName = commands[i];
            break;
        }
    }
    
    if (!varName.empty()) {
        variables[varName] = value;
        // Удаляем значение переменной из стека, если оно там есть
        if (!operandStack.empty()) {
            popStack(); // убираем значение переменной
        }
    } else {
        error("Не найдена переменная для присваивания");
    }
}

void OPSInterpreter::executeJump(const std::string& label) {
    auto it = labels.find(label);
    if (it != labels.end()) {
        programCounter = it->second;
    } else {
        error("Метка не найдена: " + label);
    }
}

void OPSInterpreter::executeConditionalJump(const std::string& label) {
    if (operandStack.empty()) {
        error("Нет условия для условного перехода");
    }
    
    int condition = popStack();
    
    // jf - jump if false (переход если условие ложно)
    if (condition == 0) {
        executeJump(label);
    } else {
        programCounter++; // Продолжаем выполнение
    }
}

int OPSInterpreter::popStack() {
    if (operandStack.empty()) {
        error("Попытка извлечения из пустого стека");
    }
    
    int value = operandStack.top();
    operandStack.pop();
    return value;
}

void OPSInterpreter::pushStack(int value) {
    operandStack.push(value);
}

void OPSInterpreter::setVariable(const std::string& name, int value) {
    variables[name] = value;
}

int OPSInterpreter::getVariable(const std::string& name) const {
    auto it = variables.find(name);
    if (it != variables.end()) {
        return it->second;
    }
    return 0; // Неинициализированные переменные имеют значение 0
}

void OPSInterpreter::printState() const {
    std::cout << "\n📊 СОСТОЯНИЕ ИНТЕРПРЕТАТОРА:" << std::endl;
    
    std::cout << "Переменные:" << std::endl;
    if (variables.empty()) {
        std::cout << "  (нет переменных)" << std::endl;
    } else {
        for (const auto& var : variables) {
            std::cout << "  " << var.first << " = " << var.second << std::endl;
        }
    }
    
    std::cout << "Стек операндов:" << std::endl;
    if (operandStack.empty()) {
        std::cout << "  (пустой)" << std::endl;
    } else {
        std::stack<int> tempStack = operandStack;
        std::vector<int> stackContents;
        while (!tempStack.empty()) {
            stackContents.push_back(tempStack.top());
            tempStack.pop();
        }
        std::cout << "  ";
        for (int i = static_cast<int>(stackContents.size()) - 1; i >= 0; --i) {
            std::cout << stackContents[i] << " ";
        }
        std::cout << "(вершина справа)" << std::endl;
    }
    
    if (!labels.empty()) {
        std::cout << "Метки:" << std::endl;
        for (const auto& label : labels) {
            std::cout << "  " << label.first << " -> позиция " << label.second << std::endl;
        }
    }
}

void OPSInterpreter::reset() {
    while (!operandStack.empty()) {
        operandStack.pop();
    }
    variables.clear();
    labels.clear();
    commands.clear();
    programCounter = 0;
    running = false;
}

void OPSInterpreter::error(const std::string& message) const {
    throw std::runtime_error("Ошибка интерпретатора: " + message + 
                              " (позиция " + std::to_string(programCounter) + ")");
} 