![KDE Plasma](images/screenshot_plasma.png)

# mkFileSearch

mkFileSearch is a Qt6 based simple and fast file search tool for Linux and Windows that does not rely on any indexing services. It allows for searching by filename and/or content either with regular search terms or RegEx expressions.

Without further arguments given, it defaults to the $HOME folder.
The intended workflow for this app is to open it with `mkFileSearch /path/to/search`, either from a file manager that allows for launching external apps this way, or via hotkey.

For example, to launch it for searching inside the path that the active Dolphin window is dispaying, a script like this could be bound to a hotkey in KDE Plasma:

```
#!/bin/bash

if [ "$XDG_SESSION_TYPE" == "wayland" ]; then
    TOOL="kdotool"
    OPTIONS=""
else
    TOOL="xdotool"
    OPTIONS="--onlyvisible"
fi


# 1. Die ID des aktiven Fensters holen
WINDOW_ID=$($TOOL getactivewindow)

# 2. Den Namen (Titel) des Fensters auslesen
FULL_TITLE=$($TOOL getwindowname "$WINDOW_ID")

# 3. Den Titel bereinigen
# Wir löschen alles ab dem " — Dolphin" Teil (Achtung: langer Gedankenstrich!)
# Die Syntax ${VAR%muster} löscht das Muster am Ende des Strings
CLEAN_PATH="${FULL_TITLE% — Dolphin}"

# 4. Überprüfung und Ausführung
if [ -d "$CLEAN_PATH" ]; then
    /path/to/mkFileSearch "$CLEAN_PATH"
fi
```

Note that this needs Dolphin to be set up to show the full path in its title bar.
