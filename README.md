# open_tabletop_import_module
A Godot Engine module allowing open-tabletop to import selected resources at runtime.

**NOTE:** Change these lines in `main/main.cpp` to stop the terminal from showing on Windows:

```cpp
1923 #ifdef DEBUG_ENABLED // <-- Change to: #ifdef DEBUG_MEMORY_ENABLED
1924     // Append a suffix to the window title to denote that the project is running
1925     // from a debug build (including the editor). Since this results in lower performance,
1926     // this should be clearly presented to the user.
1927     DisplayServer::get_singleton()->window_set_title(vformat("%s (DEBUG)", appname));
1928 #else
1929     DisplayServer::get_singleton()->window_set_title(appname);
1930 #endif
```

```cpp
2102 OS::get_singleton()->set_main_loop(main_loop);
2103 // <-- Change to: OS::get_singleton()->set_console_visible(false);
2104 return true;
```
