// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef IMGUI_FILESYSTEM_H_
#define IMGUI_FILESYSTEM_H_

// USAGE EXAMPLE:
/*
#include "imguifilesystem.h"                                                    // imguifilesystem.cpp must be compiled

// Inside a ImGui window:
const bool browseButtonPressed = ImGui::Button("...");                          // we need a trigger boolean variable
static ImGuiFs::Dialog dlg;                                                     // one per dialog (and must be static)
const char* chosenPath = dlg.chooseFileDialog(browseButtonPressed);             // see other dialog types and the full list of arguments for advanced usage
if (strlen(chosenPath)>0) {
    // A path (chosenPath) has been chosen RIGHT NOW. However we can retrieve it later more comfortably using: dlg.getChosenPath()
}
if (strlen(dlg.getChosenPath())>0) {
    ImGui::Text("Chosen file: \"%s\"",dlg.getChosenPath());
}

// If you want to copy the (valid) returned path somewhere, you can use something like:
static char myPath[ImGuiFs::MAX_PATH_BYTES];
if (strlen(dlg.getChosenPath())>0) {
    strcpy(myPath,dlg.getChosenPath());
}
*/

// MISSING FEATURES:
/*
-> [File and Folder] Links are NOT supported (they don't show up).
-> Multiselections in chooseFileDialogs are NOT supported.
-> Hidden and temporary files don't show up on nix systems (by design). Nothing has been done for Windows about it yet.
*/

// COMPILING AND TESTING:
/*
-> Compiled and tested using "ImGui library v1.17 wip"

-> Successfully compiled using gcc, clang and mingw32 compilers.
x> Never compiled on any other compiler (Visual C++'s cl.exe included).

-> Tested on Ubuntu 64bit and Wine 32bit.
x> Never tested on a real Windows OS and on MacOS.
*/

//#define DIRENT_USES_UTF8_CHARS  // Optional. Affects Windows users only. Needs recompilation of imguifilesystem.cpp. Enables long UTF8 paths instead of short ASCII paths.
                                // Unfortunately it's NOT 100% functional (in my tests some folders can't be browsed). Patches are welcome. See "dirent_portable.h" for further info.
                                // When experiencing problems on Windows, trying commenting this definition out is a good start.
                                // On a second thought, I think we should leave this definition commented out (Windows users can always define it at the project level, if needed).

#ifndef IMGUI_API
#include <imgui.h>
#endif //IMGUI_API

//#define IMGUIFS_NO_EXTRA_METHODS    // optional, but it makes this header heavier... TODO: see if I can extract PATH_MAX and FILENAME_MAX with minimal overhead
#ifndef IMGUIFS_NO_EXTRA_METHODS
#include "dirent_portable.h"    // just for PATH_MAX
#include <limits.h>             // on some systems PATH_MAX is here
#include <stdio.h>              // just for FILENAME_MAX
#endif //IMGUIFS_NO_EXTRA_METHODS

namespace ImGuiFs {

#ifndef IMGUIFS_NO_EXTRA_METHODS
const int MAX_FILENAME_BYTES = FILENAME_MAX+1;
const int MAX_PATH_BYTES = PATH_MAX+1;
#else //IMGUIFS_NO_EXTRA_METHODS
extern const int MAX_FILENAME_BYTES;
extern const int MAX_PATH_BYTES;
#   ifdef IMGUI_USE_MINIZIP
#   error Please undefine IMGUIFS_NO_EXTRA_METHODS for IMGUI_USE_MINIZIP to work
#   endif //IMGUI_USE_MINIZIP
#endif //IMGUIFS_NO_EXTRA_METHODS

struct Dialog {
    public:

    // default arguments are usually what most users expect (better not touch them in most cases)
    Dialog(bool noKnownDirectoriesSection=false,bool noCreateDirectorySection=false,bool noFilteringSection=false,bool detectKnownDirectoriesAtEachOpening=false,bool addDisplayByOption=false,bool dontFilterSaveFilePathsEnteredByTheUser=false);
    ~Dialog();

    // "dialogTriggerButton" is usually a bool variable connected to a ImGui::Button(...).
    // returns the chosen path (internally stored). Users can check when the returned path has strlen()>0.
    // "fileFilterExtensionString" can be something like ".png;.jpg;.jpeg;.bmp;.tga;.gif;.tiff;.txt". It's case insensitive.
    // "directory" and "startingFileNameEntry" (only available in saveFileDialog(...)) are quite flexible and can be set to almost everything: the method will use the most resonable choice.
    const char* chooseFileDialog(bool dialogTriggerButton,const char* directory=NULL,const char* fileFilterExtensionString=NULL,const char* windowTitle=NULL,const ImVec2& windowSize=ImVec2(-1,-1),const ImVec2& windowPos=ImVec2(-1,-1),const float windowAlpha=0.875f);
    const char* chooseFolderDialog(bool dialogTriggerButton,const char* directory=NULL,const char* windowTitle=NULL,const ImVec2& windowSize=ImVec2(-1,-1),const ImVec2& windowPos=ImVec2(-1,-1),const float windowAlpha=0.875f);
    const char* saveFileDialog(bool dialogTriggerButton,const char* directory=NULL,const char* startingFileNameEntry=NULL,const char* fileFilterExtensionString=NULL,const char* windowTitle=NULL,const ImVec2& windowSize=ImVec2(-1,-1),const ImVec2& windowPos=ImVec2(-1,-1),const float windowAlpha=0.875f);

    // gets the chosen path (internally stored). It's valid (its strlen()>0) after the user performs a valid selection, until the user performs an invalid selection (e.g. closes the dialog with the close button).
    const char* getChosenPath() const;
    // returns the last directory browsed by the user using this class (internally stored). Can be passed as "directory" parameter in the methods above to reuse last used directory.
    const char* getLastDirectory() const;

    // static variables that are usually OK as they are
    static bool WrapMode;           // (true)
    static ImVec2 WindowSize;       // (600,400) [initial window size when not defined in argument "windowSize"]
    static ImVec4 WindowLTRBOffsets;// (0,0,0,0)    [it might turn useful when using toolbars]

    private:
    struct Internal* internal;
    friend const char* ChooseFileMainMethod(Dialog& ist,const char* directory,const bool _isFolderChooserDialog,const bool _isSaveFileDialog,const char* _saveFileName,const char* fileFilterExtensionString,const char* windowTitle,const ImVec2& windowSize,const ImVec2& windowPos,const float windowAlpha);
};

// Extra methods: completely optional, undocumented and NOT guarateed to work as expected:
#ifndef IMGUIFS_NO_EXTRA_METHODS
// A bit dangerous typedefs:
typedef char FilenameString[MAX_FILENAME_BYTES];
typedef char PathString[MAX_PATH_BYTES];
// Handy typedefs:
typedef ImVector<FilenameString>      FilenameStringVector;
typedef ImVector<PathString>          PathStringVector;

// all the strings should be MAX_PATH_BYTES long
extern bool PathExists(const char* path);
extern void PathGetAbsolute(const char *path, char *rv);
extern void PathGetDirectoryName(const char *filePath, char *rv);
extern void PathGetFileName(const char *filePath, char *rv);
extern void PathGetExtension(const char* filePath,char *rv);
extern void PathAppend(const char* directory,char* rv);
extern void PathSplit(const char* path,FilenameStringVector& rv,bool leaveIntermediateTrailingSlashes=true);

enum Sorting {
    SORT_ORDER_ALPHABETIC=0,
    SORT_ORDER_ALPHABETIC_INVERSE=1,
    SORT_ORDER_LAST_MODIFICATION=2,
    SORT_ORDER_LAST_MODIFICATION_INVERSE=3,
    SORT_ORDER_SIZE=4,
    SORT_ORDER_SIZE_INVERSE=5,
    SORT_ORDER_TYPE=6,
    SORT_ORDER_TYPE_INVERSE=7,
    SORT_ORDER_COUNT
};
extern bool DirectoryExists(const char* path);
extern void DirectoryCreate(const char* directoryName);
extern void DirectoryGetDirectories(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut=NULL,Sorting sorting= SORT_ORDER_ALPHABETIC);
extern void DirectoryGetFiles(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut=NULL, Sorting sorting= SORT_ORDER_ALPHABETIC);
extern bool FileExists(const char* path);
extern bool FileGetContent(const char* path,ImVector<unsigned char>& bufferOut,const char* password=NULL);  // password is used if it's a file inside a zip path when IMGUI_USE_MINIZIP is defined (e.g. path="C://MyDocuments/myzipfile.zip/myzipFile/something.txt")
#ifdef IMGUI_USE_MINIZIP
class UnZipFile {
public:
UnZipFile(const char* zipFilePath=NULL);
bool load(const char* zipFilePath,bool reloadIfAlreadyLoaded=true);
const char* getZipFilePath() const;
bool isValid() const;
void close();

// All these paths are inside the zip file (without the "zipFilePath" prefix)
bool getDirectories(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut=NULL,Sorting sorting= SORT_ORDER_ALPHABETIC,bool prefixResultWithTheFullPathOfTheZipFile=false) const;
bool getFiles(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut=NULL,Sorting sorting= SORT_ORDER_ALPHABETIC,bool prefixResultWithTheFullPathOfTheZipFile=false) const;
unsigned int getFileSize(const char* filePath) const;
bool getFileContent(const char* filePath,ImVector<unsigned char>& bufferOut,const char* password=NULL) const;
bool exists(const char* pathInsideZip, bool reportOnlyFiles=false, bool reportOnlyDirectories=false) const;
bool fileExists(const char* pathInsideZip) const;
bool directoryExists(const char* pathInsideZip) const;
~UnZipFile();
protected:
struct UnZipFileImpl* im;
};

// eg: path="C://MyDocuments/myzipfile.zip/myzipFile/something" -> rv1="C://MyDocuments/myzipfile.zip", rv2="myzipFile/something"
extern bool PathSplitFirstZipFolder(const char* path, char* rv1,char* rv2,bool rv1IsAbsolutePath=true);
// eg: path="C://MyDocuments/myzipfile.zip/myzipFile/something"
extern bool PathExistsWithZipSupport(const char* path, bool reportOnlyFiles=false, bool reportOnlyDirectories=false,bool checkAbsolutePath=true);
#endif //IMGUI_USE_MINIZIP
#endif //IMGUIFS_NO_EXTRA_METHODS

} // namespace ImGuiFs

#endif //IMGUI_FILESYSTEM_H_
