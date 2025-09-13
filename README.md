# AlwexScript - Programming Language for Embedded Systems
**Version**: 2.0
**Supported OS**: Windows, Linux, macOS
**Latest Updates**: Added terminal command execution and file operations

## Language Features
- Simple syntax similar to natural language
- Dynamic typing
- Support for numbers and strings
- Flow control (conditions, loops)
- Functions and modularity
- Built-in I/O operations
- Library import system
- Terminal command execution
- File operations (read, write, append, check existence)
- Dynamic memory management (Unix systems)

## Installation
### Linux/macOS
```bash
gcc alwex.c -o alwex -Wall -Wextra
sudo mv alwex /usr/local/bin/
```
### Windows
```bash
gcc alwex.c -o alwex.exe -Wall -Wextra
```

## Language Syntax
### Basic Constructs
```alw
# Variables
let x = 10
let name = 'Alice'
let result = x * 2.5

# Output
print 'Hello, World!'
print x

# Input
inp int age 'Enter your age: '
inp string name 'Enter your name: '

# Conditions
if age > 18
    print 'Adult'
else if age > 13
    print 'Teenager'
else
    print 'Child'
end

# Loops
let counter = 0
while counter < 5
    print counter
    let counter = counter + 1
endloop

# Functions
func greet
    print 'Hello from function!'
    print name
end

call greet
```
### Library Import
```alw
import "random"  # Imports random.alw

call randint 1 10
print randint_result

call random
print random_result
```
### Terminal Command Execution (New in v2.0)
```alw
# Execute system commands
exec ls -la              # Linux/macOS
exec dir                 # Windows
exec echo "Hello World"
```
### File Operations (New in v2.0)
```alw
# Write to file
file_write data.txt "Hello, World!"

# Read from file
file_read data.txt

# Append to file
file_append data.txt "Additional content"

# Check file existence
file_exists data.txt
```

### Operators
## Arithmetic: +, -, *, /, plus, minus, mul, div

## Increment/decrement: ++, --, inc, dec

## Comparison: ==, !=, >, <, >=, <=

### Standard Libraries
random.alw Library
```alw
# Generates random integer in range [min, max]
call randint min max
print randint_result

# Generates random float (0, 1)
call random
print random_result

# Selects random element from array
let colors = ['red', 'green', 'blue']
call choice colors
print choice_result
```
# Example Program with Import
```alw
# Random password generator
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
```

# Save password to file (New in v2.0)
```alw
file_write password.txt generate_password_result
print 'Password saved to file!'
```

# Show file content (New in v2.0)
```alw
file_read password.txt
```
## Usage
1. Create a script file (e.g., program.alw)

2. Run the interpreter:

```bash
# Linux/macOS
./alwex program.alw

# Windows
alwex.exe program.alw
```

3. Place library files in the same directory to use them

## System Requirements
- Any OS with GCC compiler (Windows, Linux, macOS)
- 512 KB RAM
- 1 MB disk space

## Enhanced Capabilities (New in v2.0)
# For Unix Systems (Linux, macOS):
# Dynamic memory management - No hard limits on variables, strings, and functions

## Increased limits:
- Variables: 10,000 (was 100)
- Strings: 1,000 (was 30)
- Functions: 500 (was 15)
- Import depth: 50 levels (was 5)

## Terminal command execution with exec command

## File operations for reading, writing, and checking files

## For Windows:
- Terminal command execution with exec command
- File operations for reading, writing, and checking files
- Increased limits compared to previous version

## Development
- Author: Alwex Developer
- License: MIT
- Repository: https://github.com/alwex0920/AlwexScript

## Version History
- v2.0: Added terminal command execution, file operations, and dynamic memory management for Unix systems
- v1.2: Added wait command
- v1.0: Initial release with basic language features and library import system
