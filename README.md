# File_Storage_RC6502_Apple1

This is a collection of HW / SW to provide file storage capabilty for RC6502 Apple1 Replica.
	
## Main Highlights
	* Uses W25Q64 chip to srore the data
	* Simple File system to keep files structured
	* Utility programs to prepare disk images or porsions of FS on regular OS (tested on Linux)			
	
## Status
	In beta testing. I startdet to use it on daily basis, but there are some minor issues.
 
## Desired improvements
	* Better error handling. In most cases if user does input error or something unexpected happens in communication, the command is just silently ignored.
 	* Better performance in data transfer. It takes about 3 secs to load 1Kb of data.
 	* Integrasion with WozMon. 
 
## Screenshots
![emulator](https://github.com/arvjus/FDStorage_RC6502_Apple1/blob/main/gallery/apple1_1.jpg)
	
![emulator](https://github.com/arvjus/FDStorage_RC6502_Apple1/blob/main/gallery/apple1_2.jpg)
	 
![emulator](https://github.com/arvjus/FDStorage_RC6502_Apple1/blob/main/gallery/fdsh.jpg)
	
![emulator](https://github.com/arvjus/FDStorage_RC6502_Apple1/blob/main/gallery/emulator.jpg)
