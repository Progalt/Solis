

## Getting Started in Solis

Solis can be embedded in your own application or used as a standalone scripting language much like python. 

You can either clone the repo and compile the interpreter yourself using CMake. If you're on windows I recommend Clang. Otherwise I recommend either Clang or GCC. MSVC will work but some optimisations
are non-standard C and work on Clang and GCC only (Computed Gotos). Of course anything that is non-standard C also has standard implementations so it'll compile and run anywhere. 

Or you can use the binaries in the releases. This is more if you are not embedding it. Just download the zip file and extract it. 

You might want to set an environment path to the build tool of Solis. 

### Files

All solis files should have the extension `.solis`. This extension is used when importing and if not using this extension it could lead to problems. 


### Running

To run a solis file. Run this in the command line: 
```
solis main.solis 
```
It'll run and execute the code.
