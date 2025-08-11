# AlwexScript - Язык программирования для встраиваемых систем

**Версия**: 1.1  
**Поддержка ОС**: Windows, Linux, macOS  
**Обновления**: Добавлена система импорта библиотек

## Особенности языка
- Простой синтаксис, похожий на естественный язык
- Динамическая типизация
- Поддержка чисел и строк
- Контроль потока выполнения (условия, циклы)
- Функции и модульность
- Встроенные операции ввода/вывода
- **Система импорта библиотек**

## Установка

### Linux/macOS
```bash
gcc alwex.c -o alwex -Wall -Wextra
sudo mv alwex /usr/local/bin/
```

### Windows
```bash
gcc alwex.c -o alwex.exe -Wall -Wextra
```

## Синтаксис языка

### Основные конструкции
```alw
# Переменные
let x = 10
let name = 'Alice'
let result = x * 2.5

# Вывод данных
print 'Hello, World!'
print x

# Ввод данных
inp int age 'Enter your age: '
inp string name 'Enter your name: '

# Условия
if age > 18
    print 'Adult'
else if age > 13
    print 'Teenager'
else
    print 'Child'
end

# Циклы
let counter = 0
while counter < 5
    print counter
    let counter = counter + 1
endloop

# Функции
func greet
    print 'Hello from function!'
    print name
end

call greet
```

### Импорт библиотек
```alw
import "random"  # Импортирует random.alw

call randint 1 10
print randint_result

call random
print random_result
```

### Операторы
- Арифметические: `+`, `-`, `*`, `/`, `plus`, `minus`, `mul`, `div`
- Инкремент/декремент: `++`, `--`, `inc`, `dec`
- Сравнения: `==`, `!=`, `>`, `<`, `>=`, `<=`

## Стандартные библиотеки

### Библиотека random.alw
```alw
# Генерирует случайное целое в диапазоне [min, max]
call randint min max
print randint_result

# Генерирует случайное дробное число [0, 1)
call random
print random_result

# Выбирает случайный элемент из массива
let colors = ['red', 'green', 'blue']
call choice colors
print choice_result
```

## Пример программы с импортом
```alw
# Генератор случайных паролей
import "random"

func generate_password length
    let chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*'
    let password = ''
    
    let i = 0
    while i < length
        call randint 0 strlen(chars)-1
        let idx = randint_result
        let char = chars[idx]
        let password = password + char
        let i = i + 1
    endloop
    
    let generate_password_result = password
end

inp int pwd_len 'Enter password length: '
call generate_password pwd_len
print 'Your password: '
print generate_password_result
```

## Использование

1. Создайте файл скрипта (например, `program.alw`)
2. Запустите интерпретатор:
   ```bash
   # Linux/macOS
   ./alwex program.alw
   
   # Windows
   alwex.exe program.alw
   ```

3. Для использования библиотек разместите их в той же директории

## Системные требования
- Любая ОС с компилятором GCC (Windows, Linux, macOS)
- 512 КБ оперативной памяти
- 1 МБ дискового пространства

## Ограничения
- Максимальная глубина импорта: 5 уровней
- Максимальный размер файла: 64 КБ
- Максимальное количество переменных: 100
- Максимальная длина строки: 128 символов

## Разработка
- Автор: Alwex Developer
- Лицензия: MIT
- Репозиторий: https://github.com/alwex0920/AlwexScript
