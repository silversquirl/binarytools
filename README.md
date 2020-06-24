# binarytools

This repository contains a number of simple tools for processing binary files.
Descriptions of each can be found below.

## bincat

Concatenates binary files.

Usage: `bincat FILE ... >OUTFILE`

bincat takes no options; all of its arguments are interpreted as filenames.

## binstrip

Strips trailing NUL bytes from a file.

Usage: `binstrip [OPTIONS] <INPUT_FILE >OUTPUT_FILE`

binstrip can be configured to strip bytes other than NUL using its two options:

- `-c` is used to specify a literal character to strip
- `-n` is used to specify a byte to strip using hexadecimal

The option that is specified last will override previous options.

## binwrite

Creates binary files from its arguments.

Usage: `binwrite [-b BASE] BYTE ... >OUTPUT_FILE\n`

The BYTEs that are specified on the command line are parsed as numbers of the specified BASE (by default hexadecimal) and written to stdout as bytes.

## datobj

Creates an ELF object file from its input files

Usage: `datobj FILE SYMBOL [FILE SYMBOL ...] >OBJECT`

Each input file is dumped into the output ELF's `data` section, under its associated symbol name.

## headerdump

Converts a binary file into a C header.
The resulting header defines an array containing the bytes in the file.

Usage: `headerdump [-v VARNAME] [-w WRAP] <INPUT_FILE >OUTPUT_FILE`

The array in the header is named VARNAME (by default `data`), and the data is wrapped after WRAP bytes.
