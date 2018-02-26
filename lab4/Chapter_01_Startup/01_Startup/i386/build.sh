#!/bin/sh

# usage: ./build.sh [qemu|clean]
#

PROJECT=hello.elf

#Compile if required
if [ $# -eq 0 ] || ( [ $1 = "qemu" ] && [ ! -e $PROJECT ] ); then

CFLAGS="-m32 -march=i386 -Wall -Werror -ffreestanding -nostdlib -fno-stack-protector"
LDFLAGS="-melf_i386 -e arch_start -Ttext=0x100000"

#compile
gcc -c startup.S $CFLAGS
gcc -c hello.c $CFLAGS

#link
ld startup.o hello.o -o $PROJECT $LDFLAGS

#if an error occured => exit
if [ ! $? -eq 0 ] ; then
	exit
fi

echo Created System image: $PROJECT

fi #"compile"

if [ $# -gt 0 ] && [ $1 = "qemu" ]; then
	echo Starting...
	qemu-system-i386 -m 2 -no-kvm -kernel $PROJECT

elif [ $# -gt 0 ] && [ $1 = "cleanall" ]; then
	echo Cleaning...
	rm -rf *.o $PROJECT
fi
