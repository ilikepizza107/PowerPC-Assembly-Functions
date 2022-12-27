# PowerPC Assembly Functions (Code Menu Building Utility)
A fork of the P+EX code menu which implements the ability to add characters to the code menu without having to manually edit and recompile anything.
Adding new characters along with their IDs to the provided "EX_Characters.txt" and running the program is all you need to do.
Additionally, to avoid the need to manually specify output directories, output files will be created in the "Code_Menu_Output" directory (see below notes for further details). Note that a new file ("Code_Menu_Changelog.txt") will be generated when the program runs, outlining applied changes and the end state of the list.

## Instructions For Use Without Code Editing

If you're going to use this fork and don't intend to edit the code at all, simply download the [latest Release version](https://github.com/QuickLava/PowerPC-Assembly-Functions/releases), no need to download the source code. From there, extract the zipped folder, add new character entries as described below, then run the executable appropriate for your build:
- "PowerPC Assembly Functions.exe": Builds a code menu for console P+EX, producing the following files in the output folder:
	- data.cmnu (goes in pf/menu3/)
	- CodeMenu.txt
	- CodeMenu.asm (the above ASM.txt converted into assembly, goes in Source/Project+/)
	- Code_Menu_Changelog.txt
- "PowerPC Assembly Functions.exe (Dolphin)": Builds a code menu for offline Dolphin P+EX, producing the following files in the output folder:
	- data.cmnu (goes in pf/menu3/)
	- CodeMenu.txt
	- CodeMenu.asm (the above ASM.txt converted into assembly, goes in Source/Project+/)
	- Code_Menu_Changelog.txt
- "PowerPC Assembly Functions.exe (Netplay)": Builds a code menu for netplay Dolphin P+EX, producing the following files in the output folder:
	- dnet.cmnu (goes in pf/menu3/)
	- Net-CodeMenu.txt
	- Net-CodeMenu.asm (the above ASM.txt converted into assembly, goes in Source/Project+/)
	- Net-Code_Menu_Changelog.txt

Review the character list in the produced changelog to ensure that your characters were added correctly. If they were, place the produced files in their appropiate locations, run GCTRM to update your .gct files, and you should be good to go.

## Info On Automatic File Placement and GCTRM

If you place the zipped folder in its entirety into your build folder (ie. such that the entire folder is where your "pf", "rp", and "Source" folders are), the program will additionally offer to copy the produced files into their appropriate locations (as well as backup any existing files). If the program also detects the GCTRM executable and the necessary code files ("RSBE01.txt" and "BOOST.txt" for offline builds, or "NETPLAY.txt" and "NETBOOST.txt" for netplay builds), the program will also offer to run GCTRM for you, backing up and updating your .GCTs automatically.

Note: If the program offers to copy your files into the appropriate locations, but *doesn't* prompt you to run GCTRM, it's because it couldn't find the necessary code files in the build folder. This is especially prone to happenning when trying to run the "Netplay" executable without having "NETPLAY.txt" or "NETBOOST.txt" files in your build folder, as these files do not come with some versions of P+EX. Make sure those exist in your build folder, then run the program again.

Note: Unlike previous releases of this program, you need to copy the full "PowerPC Assembly Functions" folder in its entirety into your build folder in order to enable these automation features.

## Adding to EX_Characters.txt
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

***Important Note***: Characters in this file are added *in addition to* the characters present in stock P+EX. This currently includes Knuckles, Ridley, and Waluigi, and in the future will include any other characters added to the base build. This means that the only characters you need to add to this file are ones not present in base P+EX.

## Adding to EX_Rosters.txt
New lines are added in the following format:

	"Roster Name" = "Roster Filename"
	
So, for example, the following line...

	"Project+" = "CSSRosterPPlus.dat"
	
...would add an entry to the CSS Version Switcher labeled "Project+", which would load "pf/BrawlEx/CSSRosterPPlus.dat" in place of the normal "CSSRoster.dat".

Note that filenames here are really just paths relative to the "pf/BrawlEx/" folder in a build. It's perfectly valid to specify rosters like the following, for instance:

	"In A Subfolder" = "Folder/CSSRoster.dat"
	"In A Super-folder" = "../CSSRoster.dat"
	"In A Sibling Folder" = "../Sibling/CSSRoster.dat"
	
Lastly, be aware that the formatting rules on escape characters and comment characters noted in the section about adding to "EX_Characters.txt" apply to entries in this file as well.

## Adding to EX_Themes.xml
Declare new theme entries within the XML file using the following format:

    <menuTheme name = "YOUR_NAME_HERE!">
    </menuTheme>

In which "name" is the label that will be displayed within the Code Menu itself. Note that here, special characters like quotes may not be escaped with a back slash ('\'), but rather must use a proper [XML Escape String](https://www.ibm.com/docs/en/was-liberty/base?topic=SSEQTP_liberty/com.ibm.websphere.wlp.doc/ae/rwlp_xml_escape.htm). So, for example, if you wanted your theme to be labeled "Test", *with quotation marks*, you'd specify your menuTheme entry as follows:

    <menuTheme name = "&quot;Test&quot;">
    </menuTheme>

To declare a file which will be affected by a given theme, you would add a themeFile entry like so:

    <menuTheme name = "&quot;Test&quot;">
    	<themeFile name = "sc_selcharacter.pac" replacementPrefix = "ex_" />
    </menuTheme>

The "name" field in a themeFile entry denotes which of the theme-able files this entry pertains to. At present, there are 6 theme-able files:
- "mu_menumain.pac"
- "sc_selcharacter.pac"
- "sc_selcharacter2.pac"
- "sc_selmap.pac"
- "sc_sel_event.pac"
- "sc_title.pac"

Be aware that *only* the above files are currently supported! Specifying a name not listed here simply won't do anything, as theme support needs to be implemented on a per-file basis!

The "replacementPrefix" field in the above themeFile entry denotes a three-character long prefix that is used to overwrite the beginning of the associated filename, directing the game to load a different file. So, for instance, in the above example, the replacement prefix "ex_" will be applied to the filename "sc_selcharacter.pac". As a result, rather than look for the file "sc_selcharacter.pac" as it normally would, the game will *instead* attempt to locate and load the file "ex_selcharacter.pac". Note that multiple themeFile entries may be specified in a single menuTheme; so the following declaration, for instance, is perfectly valid:

    <menuTheme name = "&quot;Test&quot;">
    	<themeFile name = "sc_selcharacter.pac" replacementPrefix = "ex_" />
    	<themeFile name = "sc_selcharacter2.pac" replacementPrefix = "ex_" />
    	<themeFile name = "sc_selmap.pac" replacementPrefix = "pp_" />
    	<themeFile name = "sc_title.pac" replacementPrefix = "ez_" />
    </menuTheme>

Notice that it's completely valid to specify themeFile entries which use different prefixes. So long as the corresponding files exist in their respective locations, the game should be able to load them with no issue. Also be aware that, currently, all prefixes strictly *need* to be exactly three characters long! This restriction may be lifted at a later date, at which point this file will be updated to reflect that change, but for now take care to ensure that specified prefixes are the appropriate length!

Additionally, note that any alternate file specified in this way will be loaded from the same directory as its original counterpart! So, for instance, our "ex_selcharacter.pac" file *must* be located in "/pf/menu2/" along with "sc_selcharacter.pac" in order for the game to successfully find it!

*Lastly, and perhaps most importantly, be aware that if the game attempts to load a menu file which doesn't exist on the SD Card, it* ***will*** *crash! Pay close attention that any alternate files you specify in this way are named correctly, and that they are located in the appropriate locations in your build!*


## Instructions for Hands-free Execution

The program supports 4 boolean command line arguments which can be used to force the interactive choices offered by the program to automatically take on certain values. In order, these are:
- CMNUOverride
- ASMOverride
- GCTOverride
- PressButtonToCloseDisable

The first three can be set to either 1 or 0 to force the associated decision to process a value of "Yes" or "No" respectively, or they may be set to "-" to leave the interactive choice intact.

The final argument can be set to 1 to remove the need to press a key to close the program after it finishes running.

So, using the console executable for instance, the following program call...

> "PowerPC Assembly Functions (Console).exe" 1 0 0 1

... would force the program to replace the CMNU file, not replace the ASM or GCT Files, and skip the "press key to exit" prompt, all without any interaction from the user. Another example would be the following call...

> "PowerPC Assembly Functions (Console).exe" - - 1 0

... which would leave the choices for replacing the CMNU and ASM files intact, but would force the GCT files to be built, and leave the "press key to exit" prompt in place.


# Other Changes
- A Character Slot ID enum has been added to "Code Menu.h". This is purely to make manually adding IDs a bit more straightforward, and doesn't alter the functionality of the program at all.

- The top of "PowerPC Assembly Functions.h" now features some additional configuration options. Of particular note is the "MAIN_FOLDER" value, which determines what the expected name of the build folder will be. Another useful option is the "COLLECT_EXTERNAL_EX_CHARACTERS" toggle, which enables or disables reading in characters from external text files. Note that this is actually useable on non P+EX builds as well.

- "Code Menu.cpp" is now where default character lists and most other string constants (eg. those that govern file names, autoGCTRM and fileplacement, etc.) are now defined.

- "ControlCodes.cpp" has been edited to dynamically piece together the path to code menu from the string constants defined in "PowerPC Assembly Functions.cpp" and in "Code Menu.cpp".

