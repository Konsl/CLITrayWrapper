# CLITrayWrapper
 Windows App, which wraps around your Console Application to minimize it into the System Tray

### Overview
 Starting a CLI app using this tool creates a console window and a system tray entry. The specified command is then executed. The window can be hidden or shown by clicking on the tray icon.

### Motivation
 An example use case would be if you wanted to start a specific service on boot, but you didn't want to either always have the window in your taskbar or to manually move it to another virtual desktop.

### Usage
 ```
 Usage: cli-tray-wrapper [initial state (visible|hidden)] [tray icon tooltip] [command...]
 ```
 
 Click on the tray icon to show or hide the window.
