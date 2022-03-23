# Purpose
This fork primarily implements the ability to add characters to the P+EX code menu without having to manually edit and recompile anything.
Adding new characters along with their IDs to "EX_Characters.txt" and running the program is all you need to do.
Additionally, to avoid the need to manually specify output directories, output files will generally be created in the "EX_Characters_Output" directory (see below notes for further details). Note that a new file ("EX_Characters_Changelog.txt") will be generated when the program runs, outlining applied changes and the end state of the list.

# Notes For Use Without Code Editing
If you're going to use this fork and don't intend to edit the code in Visual Studio at all, simply download the latest Release version, no need to download the whole repo. From there, this program may be used in one of two ways: INSIDE of a build folder (ie. such that the extracted files are where your "pf", "rp", and "Source" folders are), or OUTSIDE of a build folder (ie. anywhere else).

When used OUTSIDE of a build folder, the program will simply read EX_Characters.txt, apply the relevant character changes, and emit the following files into the "EX_Characters_Output" folder:

	- ASM.txt
	- CodeMenu.asm (the above ASM.txt converted into assembly, goes in Source/Project+/)
	- data.cmnu (or dnet.cmnu, if using the DOLPHIN version) (goes in pf/menu3/)
	- EX_Characters_Changelog.txt

When used INSIDE of a build folder, AND "GCTRealMate.exe" is present in that same folder, it will read EX_Characters.txt and apply character changes like normal, but will then place CodeMenu.asm and data.cmnu (or dnet.cmnu) where they belong (backing up the original files if present), and run GCTRM to compile RSBE01.GCT and BOOST.GCT. So, after running the program, you should instead expect to see the following in "EX_Characters_Output"...

	- ASM.txt
	- EX_Characters_Changelog.txt
	
... the following newly edited files in "Source/Project+/"...

	- CodeMenu.asm (the edited code menu assembly file)
	- CodeMenu.asm.bak (a backup of the assembly file that was present in this directory before this program was last run)
	
... the following newly edited files in "pf/menu3/"...

	- data.cmnu (or dnet.cmnu, if using the DOLPHIN version) (the edited version of data.cmnu)
	- data.cmnu.bak (or dnet.cmnu.bak) (a backup of the cmnu file that was present in this directory before this program was last run)
	
... and the following newly edited files in the base folder:
	
	- codeset.txt
	- log.txt
	- BOOST.GCT (or NETBOOST.GCT if using the DOLPHIN version)
	- BOOST.GCT.bak (or NETBOOST.GCT.bak if using the DOLPHIN version)
	- RSBE01.GCT (or NETPLAY.GCT if using the DOLPHIN version)
	- RSBE01.GCT.bak (or NETPLAY.GCT.bak if using the DOLPHIN version)
	
Note that regardless of use, inside or outside of a build directory, you MUST have an "EX_Characters_Output" folder in the same directory as the executable; otherwise, the program won't run properly. If you're having problems where the program isn't outputting anything, this is almost certainly the problem.

# Adding to EX_Characters.txt
New lines are added in the following format:

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
A Character ID enum has been added to "Code Menu.h". This is purely to make manually adding IDs a bit more straightforward, and doesn't alter the functionality of the program at all.

Additionally, ControlCodes.cpp has been edited such that Dolphin builds will read "dnet.cmnu" as its code menu, not "data.cmnu". Credit to Kapedani.

