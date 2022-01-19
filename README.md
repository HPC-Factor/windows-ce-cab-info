# Windows CE CAB Info
**wcecabinfo** is a cli tool to extract information from the .000 file inside a Windows CE CAB installer file. 
It can be used to retrieve data such as application name, processor architecture and the versions of Windows CE the program is compatible with.

A .000 file or a .cab file can be passed as input. If a .cab file is passed, the tool uses [cabextract](https://www.cabextract.org.uk/) to extract the .000 file first.

This program supports piped input, however this is only possible for .000 files.

## Dependencies

This project includes [cJSON](https://github.com/DaveGamble/cJSON) to generate JSON output.

For being able to pass .cab files to the program, [cabextract](https://www.cabextract.org.uk/) needs to be installed on the system and be in `$PATH`

## Usage

```
Usage: wcecabinfo [-j] [-r] [-V] FILE
Print information about a CAB .000 file. Input can be either a cab file or an already extracted .000 file.
If a cab file is provided, cabextract is needed to handle extraction.

  -j, --json               print output as JSON
  -r, --reg                print output as Windows Reg format
                           overrides --json option
  -h, --help               print help
  -v, --version            print version information
  -V, --verbose            print verbose logs

Examples:
  wcecabinfo f.cab     Print information about file f.cab
  wcecabinfo -j f.000  Print JSON formatted information about file f.000
```
### Example: JSON output

```bash
$ wcecabinfo -j file.cab
```

### Example: .reg output

```bash
$ wcecabinfo -r file.000
```

## Output fields

 - **appName** - Application name as defined in the file
 - **provider** - Provider (e.g. developer or publisher)
 - **architecture** - Architecture, can be one of: `MIPS`, `SH3`, `SH4`, `ARM`, `X86`, `THUMB`
 - **unsupported** - List of unsupported platforms, usually one or more of  `PALM-SIZE PC`, `HPC`, `PALM PC`, `PALM PC2`, `POCKETPC`, `JUPITER`
 - **minCeVersion** - Minimum version of Windows CE needed to run this application, usually one of: `1.0`, `1.01`, `2.0`, `2.01`, `2.10`, `2.11`, `2.12`, `3.0`, `4.0`, `4.10`, `4.20`, `5.0`, `6.0`, `7.0`, `8.0`
 - **maxCeVersion** - Maximum version of Windows CE needed to run this application, can be a non-existing version, e.g. `4.99`
 - **minCeBuildNumber** - Minimum Windows CE build number
 - **maxCeBuildNumber** - Maximum Windows CE build number

 More output fields such as files, directories, registry entries and links are available in the JSON output. For more information, consult `typescript/WinCeCab000Info.ts`.

## JSON Output

The tool outputs formatted JSON when used with the `-j` flag, ideal for being used in JS/TS apps.

Typescript types are provides in `typescript/WinCeCab000Info.ts`.

## Registry (.reg) output

This tool supports outputting the registry data in the Windows .reg format, use the `-r` flag for this.

## Building

```bash
make clean && make
```

## Credits

Thanks go to
 - Stuart Caie - for documenting the .000 format
 - C:Amie and Rich Hawley - For help and feedback