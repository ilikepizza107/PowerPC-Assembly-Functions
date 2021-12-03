# Purpose
This fork primarily implements the ability to add characters the P+EX code menu without having to manually edit and recompile anything.
Adding new characters along with their IDs to "EX_Characters.txt" and running the program is all you need to do.
Additionally, to avoid the need to manually specify output directories, all output files are generated in the same directory as the executable.

# EX_Characters.txt
This is where you'll add characters for insertion to the code menu.
Add new lines in the following format:

	"Character Name" = ID
	
So, for example, the following line...

	"Daisy" = 80
	
...would add an entry for a character called Daisy with an ID of 80.

You can change the IDs for existing characters this way as well. For example, adding the line...

	"Mario" = 10
	
...would change Mario's ID from 0 to 10.

ID values are processed in decimal by default, but hex values (prefixed with "0x") are supported as well. So the following line...
	
	"Daisy" = 0x50
	
... would be functionally identical to the earlier Daisy example.

Lastly, you can use quotes in character names by escaping them with a backslash ('\\'). So the line...

	"\"Daisy\"" = 80

... would create a clone with the name "Daisy", *with quotes*.
