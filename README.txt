The given skeleton includes the following files:
- Commands.h/Commands.cpp: The supported commands of smash, each command is represented by a class that inherits from either BuiltInCommand or ExternalCommand. Each command that you add should implement execute, which is a virtual method, that executes the command.
- signals.h/signals.cpp: Declares and implements requires signal handlers: SIGINT handler to handle Ctr+C and SIGTSTP to handle Ctrl+Z. If you are going to implement the bonus part then you have to implement additional handler for SIG_ALRM.
- smash.cpp: Contains the smash main, which runs an infinite loop that receives the next typed command and sends it to SmallShell::executeCommand to handle it. Please note that if you are going to implement the bonus part, then you have to define a handler for SIG_ALRM in the main (in this file).
- Makefile: builds and tests using a basic test your smash. You can use "make zip" to prepare a zip file for submission; this is recommended, which makes sure you follow our submission's structure. 
- test_input1.txt / test_expected_output1.txt: basic test files that being used by the given Makefile to run a basic test on your smash implementation. 

Our solution and the skeleton code as well use a few known design patterns for making the code modular and readable. We use mainly two design pattersn: Singleton and Factory Method. There are many resources on the internet explaining about these design patters; they are, sometimes, known as the GoF (Gan of Four) design patters. We recommend you do a quick review of these two design patters for a better understanding of the skeleton.

How to start:
First, you have to understand the skeleton design.
The given skeleton works as follows:
- in smash.cpp the main function runs an infinite loop that reads the next typed command
- after reading the next command it calls the SmallShell::executeCommand
- SmallShell::executeCommand should create the relevant command class using the factory method CreateCommand
- After instantiating the relevant Command class, you have to:
	- fork if needed
	- call setpgrp from the child process
	- run the created-command execute method (from the child process or parent process?)
	- should the parent wait for the child? if yes, then how? using wait or waitpid?

To implement new commands, you need to:
- Implement the new command Class in Commands.cpp
- Add any private data fields in the created class and initialize them in the ctor
- Implement the new command execute method
- Add if statement to handle it in the SmallShell::CreateCommand

We recommend that you start your implementation with:
- the simple built-in commands (e.g., chprompt/pwd/showpid/cd/...), after making sure that they work fine with no bugs, then move forward
- implement the rest of the built-in commands 
- implement the external commands
- implement the execution of external commands in the background
- implement the jobs list and all relevant commands (fg/bg/jobs/...) 
- implement the I/O redirection and the pipes
- Finally implement the bonus command.

Good luck :)