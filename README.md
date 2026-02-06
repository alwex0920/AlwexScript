# AlwexScript - Programming Language for Embedded Systems
**Version**: 3.1.0
**Supported OS**: Windows, Linux, macOS
**Latest Updates**: Added OOP

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
- Arrays: full support for lists of data
- Loop control: endloop works as break statement
- OOP and Inheritance
- Install library

## Installation
### Linux/macOS
```bash
gcc alwex.c -o alwex -lcurl -Wall -Wextra
sudo mv alwex /usr/local/bin/
```
### Windows
```bash
gcc alwex.c -o alwex.exe -lwininet -Wall -Wextra
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
### Terminal Command Execution
```alw
# Execute system commands
exec ls -la              # Linux/macOS
exec dir                 # Windows
exec echo "Hello World"
```
### File Operations
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
### HTTP interaction
```alw
# GET
http_get <url>
# POST
http_post <url> <data>
# Download file
http_download <url> <filename>
```
### Arrays
```alw
# Create array
let fruits = ['apple', 'banana', 'orange']
let scores = [85, 92, 78, 95]

# Access elements
print fruits[0]      # Output: apple
print scores[2]      # Output: 78

# Array operations
arr_get fruits 1            # Get element at index 1
print arr_get_result        # Output: banana

arr_set scores 0 100        # Set scores[0] = 100

arr_length fruits           # Get array length
print arr_length_result     # Output: 3

arr_push scores 88          # Add element to end
```
### OOP
```alw
# Define a simple class
class Person
    let name = ''
    let age = 0
    
    func constructor
        let this.name = arg0
        let this.age = arg1
    end
    
    func greet
        print 'Hello! My name is:'
        print this.name
        print 'I am years old:'
        print this.age
    end
    
    func birthday
        let this.age = this.age plus 1
        print 'Happy Birthday!'
    end
end

# Create objects
let alice = new Person 'Alice' 25
let bob = new Person 'Bob' 30

# Access properties
print alice.name
print bob.age

# Call methods
call alice.greet
call alice.birthday
print alice.age
```
### OOP Inheritance
```alw
# Base class
class Animal
    let name = ''
    let age = 0
    
    func speak
        print 'Some sound...'
    end
    
    func info
        print 'Name:'
        print this.name
        print 'Age:'
        print this.age
    end
end

# Child class - inherits from Animal
class Dog extends Animal
    let breed = ''
    
    func constructor
        let this.name = arg0
        let this.breed = arg1
        let this.age = 0
    end
    
    # Override parent method
    func speak
        print 'Woof! Woof!'
    end
    
    # New method
    func fetch
        print 'Fetching the ball!'
    end
end

# Another child class
class Cat extends Animal
    let color = ''
    
    func constructor
        let this.name = arg0
        let this.color = arg1
    end
    
    # Override parent method
    func speak
        print 'Meow!'
    end
    
    func sleep
        print 'Zzz...'
    end
end

# Create objects
let dog = new Dog 'Rex' 'German Shepherd'
let cat = new Cat 'Whiskers' 'Orange'

# Use inherited methods
call dog.info
call dog.speak
call dog.fetch

print '---'

call cat.info
call cat.speak
call cat.sleep
```
### Operators
## Arithmetic: +, -, *, /, plus, minus, mul, div

## Increment/decrement: ++, --, inc, dec

## Comparison: ==, !=, >, <, >=, <=

### Standard Libraries
random.alw Library
```alw
func randint
    let __rand_internal = 0
    let temp1 = max - min
    let range = temp1 + 1
    let temp2 = __rand_internal % range
    let randint_result = min + temp2
end

func random
    let __rand_internal = 0
    let random_result = __rand_internal / 32768.0
end

func choice
    arr_length arr
    let __rand_internal = 0
    let idx = __rand_internal % arr_length_result
    arr_get arr idx
    let choice_result = arr_get_result
end
```
# Example Program with Import
```alw
import "random"

func generate_password length
    let chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%'
    let password = ''
    
    let i = 0
    while i < length
        arr_length chars
        let max_idx = arr_length_result - 1
        
        call randint 0 max_idx
        let idx = randint_result
        arr_get chars idx
        
        let password = password + arr_get_result
        let i = i + 1
    endloop
    
    let generate_password_result = password
end

# Generate and save password
inp int pwd_len 'Enter password length: '
call generate_password pwd_len

file_write password.txt generate_password_result
print 'Password saved to password.txt'

file_read password.txt
```

# Save password to file
```alw
file_write password.txt generate_password_result
print 'Password saved to file!'
```

# Show file content
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

4. If you need to install the library:
```bash
alwex install name_lib
``` 

## System Requirements
- Any OS with GCC compiler (Windows, Linux, macOS)
- 512 KB RAM
- 1 MB disk space

## Enhanced Capabilities
# For Unix Systems (Linux, macOS):
# Dynamic memory management - No hard limits on variables, strings, and functions

## Increased limits:
- Variables: 10,000
- Strings: 1,000
- Functions: 500
- Import depth: 50 levels

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

## Telegram
- https://t.me/AlwexScript
- https://t.me/AlwexScriptChat

## Version History
- v3.1.0: Added OOP
- v3.0.1: Fixed print
- v3.0: The installation logic has been added and the library search system has been redesigned, bugs have been fixed
- v2.2: Added arrays support
- v2.1: Added interaction with websites in the form of GET and POST, as well as downloading files
- v2.0: Added terminal command execution, file operations, and dynamic memory management for Unix systems
- v1.2: Added wait command
- v1.0: Initial release with basic language features and library import system
