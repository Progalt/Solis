

## Solis

This document outlines the language of Solis, in a short form way. 

All Solis statements are seperated by newlines, not semicolons and the compiler is fairly good at guessing it. 
All variables must be declared ahead of time. This is because the compiler resolves them as it goes, this mostly applies to global variables. 

```
function p()
	println(oop)
end

var oop = "Hello"
```
This isn't valid because `oop` was not defined at the time `p` is compiled. 
This leads into how variables are defined. The `var` keyword defines a new variable. Variables can be redefined with no collision as well. But the previous value will be discarded. 

Variables are implicitly assigned `null` if they aren't initialised. 
```
var x
var y = 10
```
Here `x` is assigned `null` and y is `10`