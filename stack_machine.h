#ifndef STACK_MACHINE_H
#define STACK_MACHINE_H

#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

// Состояния магазинного автомата
enum class MachineState {
    INITIAL,
    EXPRESSION,
    TERM,
    FACTOR,
    AFTER_OPERAND,
    ERROR
};

// Входные символы для автомата
enum class InputSymbol {
    NUMBER,
    IDENTIFIER,
    OPERATOR,
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    KEYWORD,
    END
};

// Класс магазинного автомата
class StackMachine {
public:
    StackMachine();
    
    // Обработка входного символа
    bool processSymbol(InputSymbol symbol, const std::string& value);
    
    // Получить текущее состояние
    MachineState getCurrentState() const { return currentState; }
    
    // Получить содержимое стека
    std::vector<std::string> getStack() const;
    
    // Очистить состояние автомата
    void reset();

private:
    std::stack<std::string> operandStack;
    std::stack<std::string> operatorStack;
    MachineState currentState;
    
    // Тип для ключа перехода
    using TransitionKey = std::pair<MachineState, InputSymbol>;
    
    // Таблица переходов
    struct TransitionKeyHash {
        std::size_t operator()(const TransitionKey& key) const {
            return std::hash<int>()(static_cast<int>(key.first)) ^ 
                   (std::hash<int>()(static_cast<int>(key.second)) << 1);
        }
    };
    
    struct TransitionKeyEqual {
        bool operator()(const TransitionKey& lhs, const TransitionKey& rhs) const {
            return lhs.first == rhs.first && lhs.second == rhs.second;
        }
    };

    std::unordered_map<TransitionKey, MachineState, TransitionKeyHash, TransitionKeyEqual> transitionTable;
    
    // Инициализация таблицы переходов
    void initTransitionTable();
    
    // Применение операции к стеку
    void applyOperation(const std::string& op);
    
    // Проверка приоритета операторов
    int getOperatorPriority(const std::string& op) const;
};

#endif // STACK_MACHINE_H 