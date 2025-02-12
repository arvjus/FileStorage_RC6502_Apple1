# Flash Disk Util
# Copyright (c) 2024, Arvid Juskaitis (arvydas.juskaitis@gmail.com)

RC6502 Flash Disk module uses W25Q64 chip (8Mb) for data storage. This module supports bulk transfers, entire image can be written or retrieved.
The purpose of this fdutil is to help to manage such image, including initialization of file system, writing, reading, deleting individual files.

## Limitations
- W25Q64 has 8Mb of memory, there are 32Kb reserved for each file, thus 256 total
- SimpleFS has flat directory structure, but supports prefixes. E.g. if we create a file "games/life", a prefix "games/" makes to apper like this file 
is in "games" directory in fdsh on RC6502 Apple-1 Replica  

## Operations
All syntax is the same as in "fdsh", E.g. to write a file, a command like this can be uesed "wfilename#start#stop" where start and stop - addresses in hex  
- Init - init fs image, fill file entries with FFs. Extend or shrink existing image 
- List - List contents of directory
- Write - allocate new entry, write data to disk
- Read - read file by name or block number, return file content
- Delete - delete file by name or block number - fill entire block (32Kb) with 0xff


## Some examples of usage

Create image to store 16 files
$ dfutil test.img i 16

Extend existing image to 256
$ dfutil test.img i 256

List all files
$ dfutil l

List files starting with games
$ dfutil lgames

Write a file name: test, start: a000, stop: 00ff
$ dfutil test.img wtest#a000#a0ff read-from-filename

Read a file by name=test, save on local file system
$ dfutil test.img rtest write-to-filename

Read a file by block id = 1, save on local file system
$ dfutil test.img r#1 write-to-filename

Remove file by name=test
$ dfutil test.img dtest

Remove file by block id=1
$ dfutil test.img d#1

