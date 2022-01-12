# Windows CE CAB Info
This tool extracts information from the 000 file inside a Windows CE CAB installer file. You can use it to find out which processor architecture as well as which Version of Windows CE the program was compiled for.

If instead of a .000 file a .cab file is passed, the tool used cabextract to extract the .000 file first.

## Usage

```
tbd
```
### Example: JSON output
```bash
$ wcecabinfo -j file.cab
```

### Example: Single field output
```bash
$ wcecabinfo -f WCEArch file.cab
ARM
```

## Useful fields
Using the -b option prints the 3 most useful fields for identifying Windows CE software

 - **WCEApp** - Indicates whether this is a Windows CE Binary, based on architecture and subsystem. Not 100% reliable for early Windows CE apps.
 - **WCEArch** - Architecture, can be one of: "MIPS", "SH3", "SH4", "ARM", "X86"
 - **WCEVersion** - Windows CE Core version, usually one of: "1.0", "1.01", "2.0", "2.01", "2.10", "2.11", "2.12", "3.0", "4.0", "4.10", "4.20", "5.0", "6.0", "7.0", "8.0"

Example:
```bash
$ wcecabinfo -b ./app.cab
WCEVersion: 2.0
WCEArch: SH3 MIPS
 ```

## JSON Output
The tool outputs formatted JSON when used with the -j option, ideal for being used in JS/TS apps.

Typescript types are provides in WinCEPEInfoType.ts.