Folder for third party libraries. When adding a library, please add an entry here with the license and any additional steps/modifications required to get the library to work in Rayne! Git submodules are preffered, if you have to make heavy modifications, create a fork in gitlab and add the fork as submodule!  
If possible, please create a static library that gets compiled and linked with the engine for each library. Adding a library makes you the maintainer of it!

## Licensing
When adding libraries, make sure that their license is compatible with the proprietary Rayne license! In general, everything that is licensed under the LGPL, MIT, BSD or Apache license is fine. GPL, Creative Commons or no direct license at all are a NO NO! (If there is no license for a given library, contact the vendor and ask them for clarification).  
If you are unsure about a license and if it is compatible, checkout [tl;dr legal](http://www.tldrlegal.com/)

## bullet
### Info
 *  Maintainer: Nils Daumann
 *  Project home: http://bulletphysics.org/
 *  Repository: -
 *  Version: 2.81

### License
Bullet Collision Detection and Physics Library
Copyright (c) 2012 Advanced Micro Devices, Inc.  http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.  
  2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.  
  3. This notice may not be removed or altered from any source distribution. 

## libpng
### Info
 *  Maintainer: Sidney Just
 *  Project home: http://www.libpng.org/
 *  Repository: -
 *  Version: 1.5.13

### Notes
Static library project for iOS and Mac OS X to allow the targets to have the same name.

### License
Copyright (c) 2012 Glenn Randers-Pehrson
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

 *  The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 *  Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 *  This notice may not be removed or altered from any source distribution.

## libz
 *  Maintainer: Sidney Just
 *  Project home: http://www.zlib.net/
 *  Repository: -
 * Version: 1.2.7

### Notes
Only required for Windows. Mac OS X, iOS and Linux ship with a dynamic library that can be linked against.

### Licsense
Copyright (C) 1995-2012 Jean-loup Gailly and Mark Adler

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.  
  2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.  
  3. This notice may not be removed or altered from any source distribution.  