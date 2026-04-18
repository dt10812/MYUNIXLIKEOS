# MYUNIXLIKEOS Quick Reference

## File Operations
```
ls          List files
mkdir DIR   Create directory
cd DIR      Change directory
pwd         Print working directory
touch FILE  Create empty file
cat FILE    Display file content
nano FILE   Edit file (enhanced editor)
vi FILE     View/edit file
grep PAT F  Search pattern in file
cp SRC DST  Copy file
mv SRC DST  Move/rename file
rm FILE     Remove file
```

## System Information
```
date        Show current date/time
pwd         Show working directory
free        Show memory usage
memory      Show page allocation
top         Show system stats
vmstat      Virtual memory statistics
```

## System Control
```
clear       Clear screen
shutdown    Shut down system
reboot      Reboot system
help        Show help
help CMD    Help for command
```

## Games & Entertainment
```
snake       Snake game (W/A/S/D to move, Q to quit)
snake color [TYPE] [COLOR]  Set game colors
snake reset Restore default colors
```

## Nano Editor Shortcuts (Enhanced)
```
Ctrl+O      Save file
Ctrl+X      Exit (with save prompt)
Ctrl+R      Reload file
Ctrl+G      Show help
Ctrl+W      Search text
Ctrl+T      Spell check
Ctrl+K      Cut line
Ctrl+U      Paste
Ctrl+C      Show position
Ctrl+J      Justify line
Ctrl+Y      Previous page
Ctrl+V      Next page
Ctrl+A      Home (start of line)
Ctrl+B      Left
Ctrl+F      Right
Ctrl+P      Up
Ctrl+Z      Down
Ctrl+L      Go to line
Ctrl+D      Delete character
Ctrl+H      Delete previous
```

## Color Names
```
black       lightgray    yellow      red
blue        darkgray     cyan        magenta
green       lightblue    lightgreen  lightcyan
lightred    lightmagenta brown       white
```

## File System Hierarchy
```
/                   Root directory
```

## Examples
```
$ echo "Hello World"                  Print text
$ cp file1.txt file2.txt             Copy file
$ mv oldname.txt newname.txt         Rename file
$ cat myfile.txt                      Read file
$ nano editor.txt                     Edit file
$ date                                Show current date
$ pwd                                 Show location
$ rm tempfile.txt                     Delete file
$ free                                Check memory
$ snake                               Play game
$ shutdown                            Turn off
```

## Tips & Tricks

### Editing with nano
1. Type normally to edit
2. Use Ctrl+A/Ctrl+E to jump to line start/end
3. Use Ctrl+L to jump to specific line number
4. Use Ctrl+W to search through document
5. Use arrow simulation: Ctrl+B (←), Ctrl+F (→), Ctrl+P (↑), Ctrl+Z (↓)
6. Use Ctrl+O to save, Ctrl+X to exit

### File Operations
- Copy a file: `cp source.txt backup.txt`
- Rename a file: `mv oldname.txt newname.txt`
- View contents: `cat filename.txt`
- Delete file: `rm filename.txt` (WARNING: No undo!)

### System Information
- Check memory: `free` or `memory`
- Current time: `date`
- System stats: `top` or `vmstat`
- Location: `pwd`

### Customizing (nano)
```
$ nano config.txt
$ cat config.txt
```

## Performance Notes

The OS includes a file cache that:
- Stores up to 16 files in RAM for fast access
- Tracks hit/miss statistics
- Automatically manages memory
- Improves repeated file access

## Keyboard Support

The PS/2 keyboard driver supports:
- All lowercase letters (a-z)
- Numbers (0-9)
- Base symbols: `-=[]\;',./`
- Shift key for uppercase and symbols
- Control key for special functions
- Backspace, Tab, Enter

## File Limits

- Filename length: 32 characters
- File size: Up to 8KB in current VFS
- Directory files: 16 children max
- Total vnodes: 64 maximum

## Common Issues & Solutions

**File appears corrupted**
- Use `rm` to delete and recreate
- Use `cp` to make backup copies

**Nano won't save**
- Press Ctrl+O to save
- Confirm filename
- Press Enter

**System won't boot**
- Check kernel compile with `make clean && make`
- Run with `make run`

**Command not found**
- Try `help` to see available commands
- Check spelling (case-sensitive)

## Getting Help

```
$ help                          List all commands
$ help COMMAND                  Help for specific command
$ nano help.txt                 Create notes
$ cat help.txt                  Read notes
```

## Documentation Files

- **ENHANCEMENTS_SUMMARY.md** - Detailed changes made
- **EXT2_DOCUMENTATION.md** - File system deep dive
- **README.md** - Project overview
- **CONTRIBUTING.md** - How to contribute

---

**Quick Tip**: Use `help` before trying commands to understand their syntax!
