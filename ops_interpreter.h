#ifndef OPS_INTERPRETER_H
#define OPS_INTERPRETER_H

#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <iostream>

// Интерпретатор ОПС (Обратной Польской записи)
class OPSInterpreter {
public:
    OPSInterpreter();
    
    // Выполнить последовательность команд ОПС
    void execute(const std::vector<std::string>& opsCommands);
    
    // Установить значение переменной (для тестирования)
    void setVariable(const std::string& name, int value);
    
    // Получить значение переменной
    int getVariable(const std::string& name) const;
    
    // Вывести состояние интерпретатора
    void printState() const;
    
    // Очистить состояние
    void reset();

private:
    std::stack<int> operandStack;                    // Стек операндов
    std::unordered_map<std::string, int> variables;  // Таблица переменных
    std::unordered_map<std::string, std::vector<int>> arrays; // Таблица одномерных массивов
    std::unordered_map<std::string, std::vector<std::vector<int>>> arrays2D; // Таблица двумерных массивов
    std::unordered_map<std::string, size_t> labels;  // Таблица меток
    std::vector<std::string> commands;               // Команды ОПС
    size_t programCounter;                           // Счетчик команд
    bool running;                                    // Флаг выполнения
    
    // Вспомогательные методы
    void parseLabels();                              // Найти все метки в коде
    bool isNumber(const std::string& str) const;     // Проверка на число
    bool isVariable(const std::string& str) const;   // Проверка на переменную
    bool isOperator(const std::string& str) const;   // Проверка на оператор
    bool isCommand(const std::string& str) const;    // Проверка на команду
    
    // Выполнение операций
    void executeArithmetic(const std::string& op);   // Арифметические операции
    void executeComparison(const std::string& op);   // Операции сравнения
    void executeAssignment();                        // Присваивание (:=)
    void executeRead();                              // Чтение (r)
    void executeWrite();                             // Запись (w)
    void executeArrayAlloc();                        // Выделение памяти массива (alloc_array)
    void executeArrayGet();                          // Получение элемента массива (array_get)
    void executeArraySet();                          // Установка элемента массива (array_set)
    void executeArrayRead();                         // Чтение в элемент массива (array_read)
    void executeArrayRead2D();                       // Чтение в элемент 2D массива (array_read_2d)
    void executeArrayAlloc2D();                      // Выделение памяти 2D массива (alloc_array_2d)
    void executeArrayGet2D();                        // Получение элемента 2D массива (array_get_2d)
    void executeArraySet2D();                        // Установка элемента 2D массива (array_set_2d)
    void executeDeclare();                           // Объявление переменной (declare)
    void executeJump(const std::string& label);      // Безусловный переход (j)
    void executeConditionalJump(const std::string& label); // Условный переход (jf)
    
    // Работа со стеком
    int popStack();                                  // Извлечь значение из стека
    void pushStack(int value);                       // Поместить значение в стек
    void error(const std::string& message) const;    // Обработка ошибок
    
    // Вспомогательные методы для массивов
    bool isArrayName(const std::string& name) const; // Проверка на имя массива
};

#endif // OPS_INTERPRETER_H 