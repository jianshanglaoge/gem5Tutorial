# Creating a very simple SimObject
**Note**: gem5 has SimObject named SimpleObject. Implementing another SimpleObject SimObject will result in confusing compiler issues.

Almost all objects in gem5 inherit from the base SimObject type. SimObjects export the main interfaces to all objects in gem5. SimObjects are wrapped C++ objects that are accessible from the Python configuration scripts.

SimObjects can have many parameters, which are set via the Python configuration files. In addition to simple parameters like integers and floating point numbers, they can also have other SimObjects as parameters. This allows you to create complex system hierarchies, like real machines.

In this chapter, we will walk through creating a simple “HelloWorld” SimObject. The goal is to introduce you to how SimObjects are created and the required boilerplate code for all SimObjects. We will also create a simple Python configuration script which instantiates our SimObject.

In the next few chapters, we will take this simple SimObject and expand on it to include debugging support, dynamic events, and parameters.


## Step 1: Create a Python class for your new SimObject
Each SimObject has a Python class which is associated with it. This Python class describes the parameters of your SimObject that can be controlled from the Python configuration files. For our simple SimObject, we are just going to start out with no parameters. Thus, we simply need to declare a new class for our SimObject and set it’s name and the C++ header that will define the C++ class for the SimObject.

We can create a file, HelloObject.py, in src/learning_gem5/part2. If you have cloned the gem5 repository you’ll have the files mentioned in this tutorial completed under src/learning_gem5/part2 and configs/learning_gem5/part2. You can delete these or move them elsewhere to follow this tutorial.

``` 
from m5.params import *
from m5.SimObject import SimObject

class HelloObject(SimObject):
    type = 'HelloObject'
    cxx_header = "learning_gem5/part2/hello_object.hh"
```

It is not required that the type be the same as the name of the class, but it is convention. The type is the C++ class that you are wrapping with this Python SimObject. Only in special circumstances should the type and the class name be different.

The cxx_header is the file that contains the declaration of the class used as the type parameter. Again, the convention is to use the name of the SimObject with all lowercase and underscores, but this is only convention. You can specify any header file here.

The cxx_class is an attribute specifying the newly created SimObject is declared within the gem5 namespace. Most SimObjects in the gem5 code base are declared within the gem5 namespace!
## Step 2: Implement your SimObject in C++
Next, we need to create hello_object.hh and hello_object.cc in src/learning_gem5/part2/ directory which will implement the HelloObject.

We’ll start with the header file for our C++ object. By convention, gem5 wraps all header files in #ifndef/#endif with the name of the file and the directory its in so there are no circular includes.

SimObjects should be declared within the gem5 namespace. Therefore, we declare our class within the namespace gem5 scope.

The only thing we need to do in the file is to declare our class. Since HelloObject is a SimObject, it must inherit from the C++ SimObject class. Most of the time, your SimObject’s parent will be a subclass of SimObject, not SimObject itself.

The SimObject class specifies many virtual functions. However, none of these functions are pure virtual, so in the simplest case, there is no need to implement any functions except for the constructor.

The constructor for all SimObjects assumes it will take a parameter object. This parameter object is automatically created by the build system and is based on the Python class for the SimObject, like the one we created above. The name for this parameter type is generated automatically from the name of your object. For our “HelloObject” the parameter type’s name is “HelloObjectParams”.

The code required for our simple header file is listed below.

```
#ifndef __LEARNING_GEM5_HELLO_OBJECT_HH__
#define __LEARNING_GEM5_HELLO_OBJECT_HH__

#include "params/HelloObject.hh"
#include "sim/sim_object.hh"

namespace gem5
{

class HelloObject : public SimObject
{
  public:
    HelloObject(const HelloObjectParams &p);
};

} // namespace gem5

#endif // __LEARNING_GEM5_HELLO_OBJECT_HH__
```

Next, we need to implement two functions in the .cc file, not just one. The first function, is the constructor for the HelloObject. Here we simply pass the parameter object to the SimObject parent and print “Hello world!”

Normally, you would never use std::cout in gem5. Instead, you should use debug flags. In the next chapter, we will modify this to use debug flags instead. However, for now, we’ll simply use std::cout because it is simple.

```
#include "learning_gem5/part2/hello_object.hh"

#include <iostream>

namespace gem5
{

HelloObject::HelloObject(const HelloObjectParams &params) :
    SimObject(params)
{
    std::cout << "Hello World! From a SimObject!" << std::endl;
}

} // namespace gem5
```

**Note**: If the constructor of your SimObject follows the following signature,
```
Foo(const FooParams &)
```

then a FooParams::create() method will be automatically defined. The purpose of the create() method is to call the SimObject constructor and return an instance of the SimObject. Most SimObject will follow this pattern; however, if your SimObject does not follow this pattern, the gem5 SimObject documetation provides more information about manually implementing the create() method.

## Step 3: Register the SimObject and C++ file
In order for the C++ file to be compiled and the Python file to be parsed we need to tell the build system about these files. gem5 uses SCons as the build system, so you simply have to create a SConscript file in the directory with the code for the SimObject. If there is already a SConscript file for that directory, simply add the following declarations to that file.

This file is simply a normal Python file, so you can write any Python code you want in this file. Some of the scripting can become quite complicated. gem5 leverages this to automatically create code for SimObjects and to compile the domain-specific languages like SLICC and the ISA language.

In the SConscript file, there are a number of functions automatically defined after you import them. See the section on that…

To get your new SimObject to compile, you simply need to create a new file with the name “SConscript” in the src/learning_gem5/part2 directory. In this file, you have to declare the SimObject and the .cc file. Below is the required code.

```
Import('*')

SimObject('HelloObject.py', sim_objects=['HelloObject'])
Source('hello_object.cc')
```

## Step 4: (Re)-build gem5
To compile and link your new files you simply need to recompile gem5. The below example assumes you are using the x86 ISA, but nothing in our object requires an ISA so, this will work with any of gem5’s ISAs.

```
scons build/X86/gem5.opt
```

## Step 5: Create the config scripts to use your new SimObject
Now that you have implemented a SimObject, and it has been compiled into gem5, you need to create or modify a Python config file hello.py in configs/learning_gem5/part2 to instantiate your object. Since your object is very simple a system object is not required! CPUs are not needed, or caches, or anything, except a Root object. All gem5 instances require a Root object.

Walking through creating a very simple configuration script, first, import m5 and all of the objects you have compiled.

```
import m5
from m5.objects import *
```

Next, you have to instantiate the Root object, as required by all gem5 instances.

```
root = Root(full_system = False)
```

Now, you can instantiate the HelloObject you created. All you need to do is call the Python “constructor”. Later, we will look at how to specify parameters via the Python constructor. In addition to creating an instantiation of your object, you need to make sure that it is a child of the root object. Only SimObjects that are children of the Root object are instantiated in C++.

```
root.hello = HelloObject()
```

Finally, you need to call instantiate on the m5 module and actually run the simulation!

```
m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))
```

Remember to rebuild gem5 after modifying files in the src/ directory. The command line to run the config file is in the output below after ‘command line:’. The output should look something like the following:

*Note*: If the code for the future section “Adding parameters to SimObjects and more events”, (goodbye_object) is in your src/learning_gem5/part2 directory, hello.py will cause an error. If you delete those files or move them outside of the gem5 directory hello.py should give the output below.

```
    gem5 Simulator System.  http://gem5.org
    gem5 is copyrighted software; use the --copyright option for details.

    gem5 compiled May  4 2016 11:37:41
    gem5 started May  4 2016 11:44:28
    gem5 executing on mustardseed.cs.wisc.edu, pid 22480
    command line: build/X86/gem5.opt configs/learning_gem5/part2/hello.py

    Global frequency set at 1000000000000 ticks per second
    Hello World! From a SimObject!
    Beginning simulation!
    info: Entering event queue @ 0.  Starting simulation...
    Exiting @ tick 18446744073709551615 because simulate() limit reached
```

Congrats! You have written your first SimObject. In the next chapters, we will extend this SimObject and explore what you can do with SimObjects.

## Step 6: Extend Lab: Finding where SimObject is called

From the starter, the SimObject is defined abstractly. In this part we will figure out when and where the SimObject object is really be called in a config scripts. 

The code shows that there are 3 possible place SimObject might be called(After the instantiation of SimObject is created, after all the objects are instantiated or after simulation begin), to check that we should add some flags in the Python config file hello.py in configs/learning_gem5/part2 

```
print("SimObject Hello is called after Simobject instantiation is created:")
root.hello = HelloObject()
print("SimObject Hello is called after all the Object is instantiated:")
m5.instantiate()
print("Beginning simulation!")
print("SimObject Hello is called after simulation begin:")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))
```

The command line to run the config file is in the output below after ‘command line:’. The output should look something like the following:

```
gem5 Simulator System.  https://www.gem5.org
gem5 is copyrighted software; use the --copyright option for details.

gem5 version 22.0.0.2
gem5 compiled Dec  7 2022 08:08:10
gem5 started Dec  7 2022 08:26:52
gem5 executing on ubuntu, pid 2832
command line: build/X86/gem5.opt configs/learning_gem5/part2/hello1.py

SimObject Hello is called after Simobject instantiation is created:
SimObject Hello is called after all the Object is instantiated:
Global frequency set at 1000000000000 ticks per second
Hello World! From a SimObject!
Beginning simulation!
SimObject Hello is called after simulation begin:
build/X86/sim/simulate.cc:194: info: Entering event queue @ 0.  Starting simulation...
Exiting @ tick 18446744073709551615 because simulate() limit reached
```

From the output we could tell that SimObject is really called at all the objects are instantiated, before the simulation. 

