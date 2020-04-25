# OS-Project3

## Division of Labor

- exit: Tyler
- info: Tyler
- size: Chelese
- ls: Chelese + Kevin + Tyler
- cd: Tyler
- creat: Chelese
- mkdir: Kevin
- mv: Tyler
- open: Tyler
- close: Tyler
- read: Tyler
- write: Kevin
- rm: Tyler
- cp: Chelese

## Commands

## exit

Exits the program and frees all allocated resources

## info

Prints out:

- bytes per sector
- sectors per cluster
- reseverd sector count
- number of FATs
- total sectors
- FATsize
- root cluster

## size FILENAME

Prints out the filesize of the file in the current directory with the name FILENAME.

## ls DIRNAME

Prints out all entries within the subdirectory DIRNAME.

If no DIRNAME is given, it prints out all the entries in the current directory.

## cd DIRNAME

Changes the current directory to DIRNAME and updates the PWD variable.

If no DIRNAME is given, it returns to the root directory.

## creat FILENAME

Creates a new file in the current directory named FILENAME.

## mkdir DIRNAME

Creates a new directory in the current directory called DIRNAME.

## mv FROM TO

If TO is a subdirectory, it will move the file called FROM into that directory.

If TO is not found, the file named FROM will be renamed as TO.

## open FILENAME MODE

Opens the file named FILENAME with MODE determining whether it is read-only, write-only, or both.

## close FILENAME

Closes the file named FILENAME if it is open.

## read FILENAME OFFSET SIZE

If the file has been opened in read mode, it prints out SIZE bytes starting at OFFSET.

## write FILENAME OFFSET SIZE "STRING"

If the file has been opened in write mode, it writes SIZE bytes of "STRING" starting at OFFSET.

If OFFSET + SIZE is greater than the current filesize, it will increase the file size for that file's entry.

If SIZE is greater than the length of "STRING", all additional bytes will be 0.

## rm FILENAME

Removes the file called FILENAME and marks all associated FAT entries as empty.

Also removes the long name entry if applicable.

## cp FILENAME TO

Creates a copy of the file called FILENAME in the directory called TO.

If TO is not given, it creates a copy in the current directory called TO.

# Things to Note

All commands will only search the current directory for files and subdirectories. The "." and ".." subdirectories point to their respective locations and are valid arguments for destinations. Absolute paths are not valid arguments.

All file and directory names are converted to uppercase as per the FAT32 specifications, including the user input before being compared to the entry names. Names can be no longer than 8 characters.

# Known Bugs
