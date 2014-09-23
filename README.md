# Zip Append tool

Very simple tool to append a valid zip archive to an arbitrary file.
This tool modifies the internal offsets in the zip archive, to keep the
appended archive valid.

Usage: `ZipAppend ZipFile TargetFile`

Does not support zip archives with archive comments.

The code is as simple as it gets. There's a project for Visual Studio 2012,
though the code will most likely to build cleanly with any compiler that
supports C++11.

Tested with the following:

 * Visual Studio 2012
 * g++ 4.8.2
 * CLang 3.4
