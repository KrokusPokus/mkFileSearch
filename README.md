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
else
    TOOL="xdotool"
fi

WINDOW_ID=$($TOOL getactivewindow)
FULL_TITLE=$($TOOL getwindowname "$WINDOW_ID")
CLEAN_PATH="${FULL_TITLE% — Dolphin}"
if [ -d "$CLEAN_PATH" ]; then
    /path/to/mkFileSearch "$CLEAN_PATH"
fi
```

Note that this relies on Dolphin to be set to show the full path in its title bar.
(You can get as fancy with this as you like. If you reconstruct the path from Kate or Konsole title bars you can start search from those windows as well.)


# FAQ

### Why did you write this? Is the Dolphin-internal search so bad?

I prefer to have a separate search window I can spawn as needed. Since my file manager windows tend to be sized very narrow und and set to compact mode, they are not fit to display search results.

### So why didn't you use just use KFind?

The way one needs to enter search terms in KFind annoyed the crap out of me. I want to type some word fragments and have it go. KFind doesn't hold with that.

### This is slow. Why not use Baloo indexing?

I don't like indexing.
