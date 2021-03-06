# SSMView
This is a simple OpenGL renderer for models in SSM format.
These models were used by games such as Deer Hunter 4, and Primal Prey, both developed by SunStorm Interactive in the early 2000's.

I have only tested this on models from Primal Prey, but Deer Hunter 4 uses the same engine, and in theory this should work with those models as well.

To use this utility you will need access to the games original data. In the future I will try to include some original models
in the format; however at the moment that is not feasible. To my knowledge there is no extant documentation on this file format,
and as such I have had to reverse engineer everything myself. There is still a significant amount of knowledge missing and as a
result, I cannot produce models in this format at the present time.

## Compilation

This codebase depends on CGLM (available on GitHub), glfw and libepoxy.
Once those are installed you can compile with make.

## Usage

The program requires as an argument the path to a valid SSM file.

Example:

```
./main ~/.wine/drive_c/Program\ Files\ \(x86\)/Primal\ Prey/Home/models/animals/utahraptor.ssm
```

## Screenshots

![Triceratops](images/trike.png?raw=true "Triceratops")
![Utahraptor](images/utahraptor.png?raw=true "Utahraptor")
![Tyrannosaurus](images/t_rex.png?raw=true "Tyrannosaurus")
![Rail Gun](images/railgun.png?raw=true "Railgun")

## TODO

Right now the codebase is a complete mess. This was a hastily programmed tool
that I wrote in about 12 hours of work as I was reverse engineering the files
themselves. In the future I want to expand the functionality, and clean up the
code.

