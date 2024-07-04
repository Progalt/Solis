
## Values

Solis is made up of multiple different built-in value types. 

### Null

Represents a lack of a value. It can't do anything but can be tested for. If used in a conditional it returns false. 

### Booleans

A boolean represents a true or false value. Implemented as class `Bool`. 

### Numbers

Numbers are implemented as 64 bit double types. Which allows for high enough precision with both integers or decimal numbers. Implemented as class `Number`.

### Strings

Strings are a specialised array of bytes. Created using double quotes `"`.

## Variables

Variables are defined using `var`: 

```
var x = "Hello, Variable"
```
Now `x` in the current scope contains the string `Hello, Variable`. This is fairly standard if you've done any programming before. These variables can then be passed around by name. Unlike some interpreters there is 
little performance penalty for declaring global variables over local. Both are a simple array lookup and resolved at compile time. 
