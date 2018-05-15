# nsisunz [![License: zlib](https://img.shields.io/badge/License-zlib-blue.svg)](https://en.wikipedia.org/wiki/Zlib_License) [![NSIS: 3.0+](https://img.shields.io/badge/NSIS-3.0%2B-orange.svg)](https://en.wikipedia.org/wiki/Nullsoft_Scriptable_Install_System)

**nsisunz** is a [NSIS](https://en.wikipedia.org/wiki/Nullsoft_Scriptable_Install_System) plugin which allows you to extract files from ZIP archives.

> **nsisunz** is great when you use another NSIS plug-in named _NSISdl_ to download a ZIP file from the internet. Download a small installer which lets the user choose the components he/she want to install and the installer downloads it (_QuickTime Setup_ does this).

## Origins & History:
- _nsisunz_ was originally written by Saivert (http://saivert.com/), and hosted here: http://nsis.sourceforge.net/Nsisunz_plug-in
- Unicode NSIS support [was added by Gringoloco023](http://portableapps.com/node/21879), February 6 2010
- Further improvements by past-due, 2018+
  - Update NSIS plugin API to 3.0
  - Update bundled [zlib](https://zlib.net) to [1.2.11](https://zlib.net/ChangeLog.txt)
  - Update bundled Minizip to 1.1-5
  - [CMake](https://cmake.org) build support (builds both ANSI and Unicode DLLs)

## Examples:

> For more, see the `Examples/` directory.

#### UnzipToLog

```NSIS
InitPluginsDir
; Call plug-in. Push filename to ZIP first, and the dest. folder last.
nsisunz::UnzipToLog "$PLUGINSDIR\myzipfile.zip" "$INSTDIR"

; Always check result on stack
Pop $0
StrCmp $0 "success" ok
DetailPrint "$0" ;print error message to log
ok:
```

#### Unzip

```NSIS
; You can also use the "Unzip" function if you don't want to use the log.
; It is a lot quicker, and is preferred for large ZIP archives.
nsisunz::Unzip "$PLUGINSDIR\myzipfile.zip" "$INSTDIR"
```

## Credits:
- Based on code in NSIS Zip2Exe
  portions Copyright © 1999-2001 Miguel Garrido (mgarrido01@hotmail.com)
- [ZLIB](https://zlib.net) - Copyright © Mark Adler
- MiniZip - Copyright © 1998-2010 - by Gilles Vollant - version 1.1 64 bits from Mathias Svensson

## License:
Condition of use and distribution are the same as [zlib](https://en.wikipedia.org/wiki/Zlib_License):

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
