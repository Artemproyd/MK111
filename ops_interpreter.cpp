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
        else if (command == ":=") {
            // Присваивание
            executeAssignment();
            std::cout << " → присваивание";
        }
        else if (command == "r") {
            // Операция чтения (read/input)
            executeRead();
            std::cout << " → чтение";
        }
        else if (command == "w") {
            // Операция записи (write/output)
            executeWrite();
            std::cout << " → запись";
        }
        else if (command == "i") {
            // Операция индексации массива
            executeArrayIndex();
            std::cout << " → индексация массива";
        }
        else if (command == "m1") {
            // Выделение памяти для 1D массива
            executeMemAlloc1D();
            std::cout << " → выделение памяти 1D массива";
        }
        else if (command == "m2") {
            // Выделение памяти для 2D массива
            executeMemAlloc2D();
            std::cout << " → выделение памяти 2D массива";
        }
        else if (command == "jf") {
            // Условный переход - метка должна быть предыдущей командой
            if (programCounter > 0) {
                std::string label = commands[programCounter - 1];
                size_t oldPC = programCounter;
                executeConditionalJump(label);
                std::cout << " → условный переход к " << label;
                
                // Если programCounter изменился, значит произошел переход
                if (programCounter != oldPC) {
                    continue; // Переход выполнен, не увеличиваем programCounter
                }
                // Иначе продолжаем обычное выполнение (programCounter будет увеличен)
            } else {
                std::cout << " (нет метки)";
            }
        }
        else if (command == "j") {
            // Безусловный переход - метка должна быть предыдущей командой
            if (programCounter > 0) {
                std::string label = commands[programCounter - 1];
                executeJump(label);
                std::cout << " → безусловный переход к " << label;
                continue; // programCounter уже изменен в executeJump
            } else {
                std::cout << " (нет метки)";
            }
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
        // Проверяем, является ли это аргументом для команды перехода
        else if (programCounter + 1 < commands.size() && 
                 (commands[programCounter + 1] == "jf" || commands[programCounter + 1] == "j")) {
            // Это аргумент для команды перехода, просто пропускаем
            std::cout << " (аргумент для " << commands[programCounter + 1] << ")";
        }
        else if (isVariable(command)) {
            // Проверяем, следует ли за переменной команда присваивания или ввода/вывода
            bool isAssignmentTarget = false;
            if (programCounter + 1 < commands.size() && 
                (commands[programCounter + 1] == ":=" || 
                 commands[programCounter + 1] == "r")) {
                isAssignmentTarget = true;
            }
            
            if (isAssignmentTarget) {
                // Переменная перед := или r - это цель, не загружаем в стек
                std::cout << " → цель для " << commands[programCounter + 1] << ": " << command;
            } else {
                // Обычная переменная или переменная перед w - помещаем её значение в стек
                int value = getVariable(command);
                pushStack(value);
                std::cout << " → стек: " << command << "=" << value;
            }
        }
        else {
            // Проверяем, является ли это меткой для команды перехода
            if (programCounter > 0 && 
                (commands[programCounter - 1] == "jf" || commands[programCounter - 1] == "j")) {
                // Это метка после команды перехода, пропускаем её
                std::cout << " (аргумент для " << commands[programCounter - 1] << ")";
            }
            else if (command.back() == ':') {
                // Метка в коде
                std::cout << " (метка)";
            }
            else {
                // Неизвестная команда
                std::cout << " (неизвестная команда: " << command << ")";
            }
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
    if (operandStack.size() < 1) {
        error("Недостаточно операндов для присваивания");
    }
    
    int value = popStack(); // Значение для присваивания
    
    // Имя переменной должно быть прямо перед командой ":="
    if (programCounter == 0) {
        error("Не найдена переменная для присваивания");
    }
    
    std::string varName = commands[programCounter - 1];
    
    if (!isVariable(varName)) {
        error("Неверное имя переменной для присваивания: " + varName);
    }
    
    variables[varName] = value;
    std::cout << " (" << varName << " = " << value << ")";
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
        // Условие ложно - переходим к метке
        auto it = labels.find(label);
        if (it != labels.end()) {
            programCounter = it->second;
            std::cout << " (переход выполнен: условие = " << condition << ")";
        } else {
            error("Метка не найдена: " + label);
        }
    } else {
        // Условие истинно - продолжаем выполнение (programCounter будет увеличен в основном цикле)
        std::cout << " (переход НЕ выполнен: условие = " << condition << ")";
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
    
    std::cout << "Массивы:" << std::endl;
    if (arrays.empty()) {
        std::cout << "  (нет массивов)" << std::endl;
    } else {
        for (const auto& array : arrays) {
            std::cout << "  " << array.first << "[" << array.second.size() << "] = {";
            for (size_t i = 0; i < array.second.size(); ++i) {
                std::cout << array.second[i];
                if (i < array.second.size() - 1) std::cout << ", ";
            }
            std::cout << "}" << std::endl;
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
    arrays.clear();
    labels.clear();
    commands.clear();
    programCounter = 0;
    running = false;
}

void OPSInterpreter::error(const std::string& message) const {
    throw std::runtime_error("Ошибка интерпретатора: " + message + 
                              " (позиция " + std::to_string(programCounter) + ")");
}

void OPSInterpreter::executeRead() {
    // Операция чтения - запрашиваем значение у пользователя
    std::cout << "\n  Введите значение: ";
    int value;
    std::cin >> value;
    
    // Проверяем, была ли перед командой r операция индексации i
    if (programCounter >= 1 && commands[programCounter - 1] == "i") {
        // Записываем в массив M[index]
        // Нужно найти имя массива и индекс в предыдущих командах
        if (programCounter >= 3) {
            std::string arrayName = commands[programCounter - 3]; // M
            std::string indexVar = commands[programCounter - 2];   // a
            
            int index = getVariable(indexVar);
            
            if (arrays.find(arrayName) == arrays.end()) {
                // Автоматически создаем массив если его нет
                arrays[arrayName] = std::vector<int>(10, 0); // размер по умолчанию
            }
            
            if (index >= 0 && index < static_cast<int>(arrays[arrayName].size())) {
                arrays[arrayName][index] = value;
                std::cout << "  Прочитано в " << arrayName << "[" << index << "] = " << value;
            } else {
                error("Индекс массива вне границ при записи: " + std::to_string(index));
            }
        } else {
            error("Недостаточно информации для записи в массив");
        }
    } else {
        // Обычная запись в переменную
        if (programCounter == 0) {
            error("Не найдена переменная для чтения");
        }
        
        std::string varName = commands[programCounter - 1];
        
        if (!isVariable(varName)) {
            error("Неверное имя переменной для чтения: " + varName);
        }
        
        variables[varName] = value;
        std::cout << "  Прочитано: " << varName << " = " << value;
    }
}

void OPSInterpreter::executeWrite() {
    // Операция записи - выводим значение из стека
    if (operandStack.empty()) {
        error("Нет значения для вывода");
    }
    
    int value = popStack();
    std::cout << "\n  ВЫВОД: " << value;
}

void OPSInterpreter::executeArrayIndex() {
    // Операция индексации массива: M index i → адрес M[index]
    if (operandStack.size() < 1) {
        error("Недостаточно операндов для индексации массива");
    }
    
    int index = popStack();  // Индекс массива
    
    // Имя массива должно быть перед индексом в команде
    if (programCounter < 2) {
        error("Не найдено имя массива для индексации");
    }
    
    std::string arrayName = commands[programCounter - 2];
    
    if (!isArrayName(arrayName)) {
        error("Неверное имя массива: " + arrayName);
    }
    
    // Проверяем границы массива
    if (arrays.find(arrayName) == arrays.end()) {
        error("Массив не инициализирован: " + arrayName);
    }
    
    if (index < 0 || index >= static_cast<int>(arrays[arrayName].size())) {
        error("Индекс массива вне границ: " + std::to_string(index));
    }
    
    // Помещаем значение массива в стек (для чтения)
    // Для записи в массив будет использоваться специальная логика в executeRead
    pushStack(arrays[arrayName][index]);
    std::cout << " (" << arrayName << "[" << index << "] = " << arrays[arrayName][index] << ")";
}

void OPSInterpreter::executeMemAlloc1D() {
    // Формат: M a m1 → выделяет память для массива M размером a
    if (operandStack.size() < 1) {
        error("Недостаточно операндов для выделения памяти");
    }
    
    int size = popStack(); // размер из стека (a)
    
    if (programCounter < 2) {
        error("Не найдено имя массива для выделения памяти");
    }
    
    std::string arrayName = commands[programCounter - 2]; // имя массива M (перед размером a)
    
    if (size < 0) {
        error("Неверный размер массива: " + std::to_string(size));
    }
    
    // Выделяем память для массива размером size+1, чтобы индекс size был валидным
    arrays[arrayName] = std::vector<int>(size + 1, 0);
    
    std::cout << " (выделен массив " << arrayName << "[" << (size + 1) << "], индексы 0-" << size << ")";
}

void OPSInterpreter::executeMemAlloc2D() {
    // mem2: arrayName rows cols m2 → выделяет память для 2D массива
    if (operandStack.size() < 2) {
        error("Недостаточно операндов для выделения памяти 2D массива");
    }
    
    int cols = popStack();
    int rows = popStack();
    
    if (programCounter < 2) {
        error("Не найдено имя массива для выделения памяти");
    }
    
    std::string arrayName = commands[programCounter - 3]; // имя массива перед rows и cols
    
    if (rows <= 0 || cols <= 0) {
        error("Неверные размеры 2D массива: " + std::to_string(rows) + "x" + std::to_string(cols));
    }
    
    // Выделяем память для 2D массива как 1D массив размером rows*cols
    arrays[arrayName] = std::vector<int>(rows * cols, 0);
    
    std::cout << " (выделен 2D массив " << arrayName << "[" << rows << "][" << cols << "])";
}

bool OPSInterpreter::isArrayName(const std::string& name) const {
    // Проверяем, является ли имя именем массива
    return arrays.find(name) != arrays.end() || 
           (isVariable(name) && name.length() == 1 && std::isupper(name[0]));
} 