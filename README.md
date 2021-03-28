# Diamond-Edit
The goal of this project is to create a simple but robust command-line text editor (with ease of writing code primarily in mind).
Influenced by Vim but designed to be simpler, more compact, and have everything configured to work nicely out of the box.

## Features
Still in development, so will be lacking a lot of these features for now.
Also only written with Linux support currently.

* Custom syntax highlighting for different file types
* Split window view and multiple tabs
* Easy to open many files and navigate between them (workspace management)

It will also obviously have:
* Undo history
* Search and replace
* Minimal memory usage and high performance

## Default Controls

This editor has two modes of operation: Command and Editor.

### Command Mode

* h ... move cursor left one character 
* j ... move cursor down one line
* k ... move cursor up one line
* l ... move cursor right one character
* i ... move cursor left one word
* o ... move cursor right one word

The capitalized versions of the cursor movement commands (shift + key) enable text selection and move the cursor to select as expected. The start of the selection is wherever the cursor is before selection begins.
* H ... move selection end left one character
* J ... move selection end down one line
* K ... move selection end up one line
* L ... move selection end right one character
* I ... move selection left one word
* O ... move selection right one word

* f ... enter editor mode


### Editor Mode

All character keys add the character to the text document like a normal text editor.

Press the Escape key to exit and return to command mode.
