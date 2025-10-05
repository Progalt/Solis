
# Solis 

***Small and fast with a Lua-like syntax and a dash of Ruby.***

Everything you would expect from a programming language. Suited to embedding in your application or to use standalone. 

Here's a snippet: 
```
class CoffeeMaker

	static var coffeesMade = 0

	function brew(type)
		
		println("Brewing your coffee...")

		self.coffeesMade = self.coffeesMade + 1

		println("Made your coffee: " + type)
	end
end

var coffeeMaker = CoffeeMaker()

coffeeMaker.brew("Latte")
coffeeMaker.brew("Americano")

println(CoffeeMaker.coffeesMade)
```

The language was designed with game scripting in mind but is suited for a variety of uses much like Lua. Unlike lua, it has more object oriented style objects meaning not everything is a table, and there 
is built in support for more types. 


With a fully featured C API for embedding the language easily. 


### Some optimisations

On release builds Nan-Boxing is used to pack values into 64 bits, similar to stuff like V8 and LuaJIT. 

On Clang (I think GCC too, but it's not tested) Jump Tables are used instead of standard switch statements. 

### Compiling

A C compliant compiler is needed, and that's about it. It doesn't depend on anything but the stdlib, in some cases
it may depend on the OS APIs like Windows API. 

I recommend Clang, over something like MSVC. The Clang C Compiler has Jump Tables, which do provide a pretty good optimisation 
over standard switches. 


### Resources: 
- The amazing https://craftinginterpreters.com/
- https://www.lua.org/
- https://wren.io/