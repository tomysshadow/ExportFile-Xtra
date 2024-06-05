# ExportFile Xtra 0.4.2
## By Anthony Kleine

ExportFile Xtra exports the content of Director cast members out to files. It can export all built-in cast member types, as well as Xtra Media.

ExportFile Xtra is an open source software, created by Anthony Kleine. It is not a commercial software. There is no trial period or license key. It is not shareware or donationware. You will never be asked to pay money for it. However, because it is MIT licensed, the code may be freely used and modified, including in commercial products.

Currently, ExportFile Xtra is only available for Windows. However, it may be available for Mac in the future, so any platform specific behaviour will still be noted. ExportFile is compatible with Windows XP to Windows 11.

# How to Use the ExportFile Xtra

This README focuses on how to compile the ExportFile Xtra. If you would like to download the binary instead, please go to the [Releases page.](https://github.com/tomysshadow/ExportFile-Xtra/releases) Install the Xtra as normal, by copying it to the Xtras folder in the Director install directory (located in the Configuration folder in Director MX 2004 and newer.)

For documentation on how to use the ExportFile Xtra in Director, please download the ExportFile Resources folder on the Releases page. The ExportFile Resources folder has:

- The ExportFile Xtra Help document.
- The ExportFile Sample Movie (for Director MX 2004 or newer) with example code.
- The exportfile.h header for Xtra developers.

## Compiling for Windows with Visual Studio

Compiling the ExportFile Xtra requires Visual Studio 2019 or newer, and the Director 11.5 XDK. In order to use the Visual Studio solution, it must be located correctly within the XDK so that the required headers will be found.

1. In the root folder of the XDK (the one that contains folders called Docs, Examples, Include, Lib, and XDKXtras,) create a new folder called Xtras.
2. In the Xtras folder, create a new folder called Script.
3. In the Script folder, create a new folder called ExportFile Xtra. Clone the repository into this directory.

If done correctly, you should now have a directory structure which looks like this:
XDK > Xtras > Script > ExportFile Xtra > winproj > script.sln

Set the `EXPORTFILE_DIRECTOR_XTRAS_DIR` environment variable to the path of your Director Xtras directory (typically similar to `C:\Program Files (x86)\Adobe\Adobe Director 11\Configuration\Xtras`.) When the solution is built, the Xtra will be automatically copied to this directory.

Now that the solution is located correctly so it will be able to find the required headers, and the environment variable is set so the Xtra will be copied when the solution is built, you may open script.sln in Visual Studio and build the solution.

**MAKE SURE TO BUILD FOR x86, NOT x64.** Building the ExportFile Xtra for x64 is not supported.

## ExportFile Test Movie

After building the solution, the ExportFile Test Movie will be played. It will write around 30 MB of files to a new folder (which is excluded from the repository.) If the ExportFile Test Movie fails or crashes, that means the new build is not working as intended, and Visual Studio will report that the build failed. Editing the ExportFile Test Movie requires Director 11.5, the [LeechProtectionRemovalHelp Xtra](https://github.com/tomysshadow/LeechProtectionRemovalHelp-Xtra/releases), and [Valentin's CommandLine Xtra.](https://valentin.dasdeck.com/xtras/commandline_xtra/)