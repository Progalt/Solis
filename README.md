
# Solis 

***Small and fast with a lua-like syntax.***

Everything you would expect from a programming language. Suited to embedding in your application or to use standalone. 

Here's a snippet: 
```lua
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