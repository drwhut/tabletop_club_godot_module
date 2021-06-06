# Tabletop Club Godot Module

This module allows [Tabletop Club](https://github.com/drwhut/tabletop-club) to
perform specific tasks that are not possible with the vanilla Godot.

## What does this module do?

Right now, this module only does two things:

* It allows for the importing of specific resources from anywhere in the file
  system **at runtime**!

* It allows for warnings and errors to be caught in GDScript.

## How can I use this module?

If you have the Godot source code, you can place this module into the `modules/`
folder, and
[compile Godot](https://docs.godotengine.org/en/stable/development/compiling/index.html)
as you normally would.

**NOTE:** If you are building Godot with the `tools=no` flag, the build will
fail due to the editor's importing code not being linked into the executable.
To fix this, you can either use [my fork](https://github.com/drwhut/godot) and
the latest `tabletop-*` branch, or you can have a look at
[this commit](https://github.com/drwhut/godot/commit/ff5752d23035bc4e4e2da3d4d0d8e8f28691accf)
to see how to fix it directly.
