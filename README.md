# Purpose
This fork primarily implements the ability to add characters to the P+EX code menu without having to manually edit and recompile anything.
Adding new characters along with their IDs to "EX_Characters.txt" and running the program is all you need to do.
Additionally, to avoid the need to manually specify output directories, all output files are generated in the same directory as the executable. Note that a new file ("EX_Characters_Changelog.txt") will be generated when the program runs, outlining applied changes and the end state of the list.

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

You can also use quotes in character names by escaping them with a backslash ('\\'). So the line...

	"\"Daisy\"" = 80

... would create a clone with the name "Daisy", *with quotes*.

Lastly, adding a hashtag ('#') or forward slash ('/') to the beginning of a line will comment out that line, having the program disregard it completely. So the following lines...

	#This is a comment
	//So is this, as well as the next one
	/"Daisy" = 80

... will all be treated as comments and ignored by the program.

***Important Note***: Characters in this file are added *in addition to* the characters present in stock P+EX. This includes Knuckles and Ridley, and in the future will include any other characters added to the base build. This means that the only characters you need to add to this file are ones not present in the base P+EX.

# Other Changes
The only other change present in this fork is the addition of a Character ID enum to "Code Menu.h". This is purely to make manually adding IDs a bit more straightforward, and doesn't alter the functionality of the program at all.
