# C_and_Assembly_x86
This program used for filtering audio file. I assigned here the coefficients for a 3rd-order Butterworth 1000Hz low-pass filter.  
Make a program in the C  programming language with a library written in the NASM assembler. 
To create the project, compile the assembly file firsly. The command for it:
    nasm -f win32 filter.asm -o filter.obj
Then create a new DevC++ console project. Choose a 32-bit version of the C compiler  in "Project->options->compiler"
Add an object file to the project by the "projectOptions->parameters->add library"
NOTE: The  assembler and the C compiler must be 32bit versions  (I am currently don`t understand difficult 
 64-bit conventions,  so I  wrote it in 3 2bit environment)  
