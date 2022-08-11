#! /bin/tcsh
# mx25 : makefile

shell : shell.c shell_lib.so
	gcc -Wall shell.c shell_lib.so -o shell -lreadline

shell_lib.so : shell_lib.c
	gcc -Wall -fpic -shared shell_lib.c -o shell_lib.so

