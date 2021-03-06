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


#ifndef IMGUIVARIOUSCONTROLS_H_
#define IMGUIVARIOUSCONTROLS_H_

#ifndef IMGUI_API
#include <imgui.h>
#endif //IMGUI_API

// USAGE
/*
#include "imguivariouscontrols.h"

// inside a ImGui::Window:
ImGui::TestProgressBar();

ImGui::TestPopupMenuSimple();
*/



namespace ImGui {

// Please note that you can tweak the "format" argument if you want to add a prefix (or a suffix) piece of text to the text that appears at the right of the bar.
// returns the value "fraction" in 0.f-1.f.
// It does not need any ID.
float ProgressBar(const char* optionalPrefixText,float value,const float minValue=0.f,const float maxValue=1.f,const char* format="%1.0f%%",const ImVec2& sizeOfBarWithoutTextInPixels=ImVec2(-1,-1),
                 const ImVec4& colorLeft=ImVec4(0,1,0,0.8),const ImVec4& colorRight=ImVec4(0,0.4,0,0.8),const ImVec4& colorBorder=ImVec4(0.25,0.25,1.0,1));

void TestProgressBar();

// Single column popup menu without icon support. It disappears when the mouse goes away.
// Returns -1 when no entries has been selected.
// Optional scrollUpEntryText returns index of -2,scrollDownEntryText -3 (but they must be manually handled by the user)
int PopupMenuSimple(bool& open, const char** pEntries, int numEntries, const char* optionalTitle=NULL, int* pOptionalHoveredEntryOut=NULL, int startIndex=0, int endIndex=-1, bool reverseItems=false, const char* scrollUpEntryText=NULL, const char* scrollDownEntryText=NULL);

// returns -1 if nothing has been chosen, 0 if copy has been clicked, 1 if cut has been clicked and 2 if paste has been clicked
int PopupMenuSimpleCopyCutPasteOnLastItem(bool readOnly=false);

class PopupMenuSimpleParams {
public:
    bool open;
    int getSelectedEntry() const {return selectedEntry;}    // optional (use PopupMenuSimple(...) return value)
protected:
    int selectedEntry;
    int hoveredEntry;
    int endIndex;
    int startIndex;
    float scrollTimer;
    bool resetScrollingWhenRestart;
public:
    PopupMenuSimpleParams(bool _resetScrollingWhenRestart=true)
    : open(false),selectedEntry(-1),hoveredEntry(-1),endIndex(-1),startIndex(-1),scrollTimer(ImGui::GetTime()),resetScrollingWhenRestart(_resetScrollingWhenRestart)
    {}
friend int PopupMenuSimple(PopupMenuSimpleParams& params,const char** pTotalEntries,int numTotalEntries,int numAllowedEntries,bool reverseItems,const char* optionalTitle,const char* scrollUpEntryText,const char* scrollDownEntryText);
};

int PopupMenuSimple(PopupMenuSimpleParams& params,const char** pTotalEntries,int numTotalEntries,int numAllowedEntries,bool reverseItems=false,const char* optionalTitle=NULL,const char* scrollUpEntryText="   ^   ",const char* scrollDownEntryText="   v   ");

void TestPopupMenuSimple(const char* scrollUpEntryText="   ^   ",const char* scrollDownEntryText="   v   ");

// Single column popup menu with icon support. It disappears when the mouse goes away. Never tested.
// User is supposed to create a static instance of it, add entries once, and then call "render()".
class PopupMenu {
protected:
// TODO: Merge IconData into PopupMenuEntry
    struct IconData {
        ImTextureID user_texture_id;
        ImVec2 uv0;
        ImVec2 uv1;
        ImVec4 bg_col;
        ImVec4 tint_col;
        IconData(ImTextureID _user_texture_id=NULL,const ImVec2& _uv0 = ImVec2(0,0),const ImVec2& _uv1 = ImVec2(1,1),const ImVec4& _bg_col = ImVec4(0,0,0,1),const ImVec4& _tint_col = ImVec4(1,1,1,1))
            : user_texture_id(_user_texture_id),uv0(_uv0),uv1(_uv1),bg_col(_bg_col),tint_col(_tint_col)
        {}
        IconData(const IconData& o) {*this = o;}
        inline int compareTo(const IconData& o) const {
            if ((size_t) user_texture_id < (size_t) o.user_texture_id) return -1;
            if (user_texture_id==o.user_texture_id) {
                if (uv0.y < o.uv0.y) return -1;
                if (uv0.y == o.uv0.y)   {
                    if (uv0.x < o.uv0.x) return -1;
                    if (uv0.x == o.uv0.x) return 0;
                }
            }
            return 1;
        }
        const IconData& operator=(const IconData& o) {
            user_texture_id = o.user_texture_id;
            uv0 = o.uv0; uv1 = o.uv1; bg_col = o.bg_col; tint_col = o.tint_col;
            return *this;
        }
    };
public:    
    struct PopupMenuEntry : public IconData  {
    public:
        enum {
            MAX_POPUP_MENU_ENTRY_TEXT_SIZE = 512
        };
        char text[MAX_POPUP_MENU_ENTRY_TEXT_SIZE];
        bool selectable;
        PopupMenuEntry(const char* _text=NULL,bool _selectable=false,ImTextureID _user_texture_id=NULL,const ImVec2& _uv0 = ImVec2(0,0),const ImVec2& _uv1 = ImVec2(1,1),const ImVec4& _bg_col = ImVec4(0,0,0,1),const ImVec4& _tint_col = ImVec4(1,1,1,1))
            :  IconData(_user_texture_id,_uv0,_uv1,_bg_col,_tint_col),selectable(_selectable)
        {
            if (_text)   {
                IM_ASSERT(strlen(_text)<MAX_POPUP_MENU_ENTRY_TEXT_SIZE);
                strcpy(text,_text);
            }
            else text[0]='\0';
        }
        PopupMenuEntry(const PopupMenuEntry& o) : IconData(o) {*this = o;}
        inline int compareTo(const PopupMenuEntry& o) const {
            if (text[0]=='\0')  {
                if (o.text[0]!='\0') return 1;
            }
            else if (o.text[0]=='\0') return -1;
            const int c = strcmp(text,o.text);
            if (c!=0) return c;
            if ((size_t) user_texture_id < (size_t) o.user_texture_id) return -1;
            if (user_texture_id==o.user_texture_id) {
                if (uv0.y < o.uv0.y) return -1;
                if (uv0.y == o.uv0.y)   {
                    if (uv0.x < o.uv0.x) return -1;
                    if (uv0.x == o.uv0.x) return 0;
                }
            }
            return 1;
        }
        const PopupMenuEntry& operator=(const PopupMenuEntry& o) {
            IconData::operator=(o);
            selectable = o.selectable;
            if (o.text[0]!='\0') strcpy(text,o.text);
            else text[0]='\0';
            return *this;
        }
    };

mutable int selectedEntry;  // of last frame. otherwise -1
ImVector <PopupMenuEntry> entries;  // should be protected, but maybe the user wants to modify it at runtime: in case inherit from this class

void addEntryTitle(const char* text,bool addSeparator=true) {
    entries.push_back(PopupMenuEntry(text,false));
    if (addSeparator) addEntrySeparator();
}
void addEntrySeparator() {
    entries.push_back(PopupMenuEntry(NULL,false));
}
void addEntry(const char* _text,ImTextureID _user_texture_id=NULL,const ImVec2& _uv0 = ImVec2(0,0),const ImVec2& _uv1 = ImVec2(1,1),const ImVec4& _bg_col = ImVec4(0,0,0,1),const ImVec4& _tint_col = ImVec4(1,1,1,1))  {
    entries.push_back(PopupMenuEntry(_text,true,_user_texture_id,_uv0,_uv1,_bg_col,_tint_col));
}

// of last frame. otherwise -1
int getSelectedEntry() const {return selectedEntry;}

// please set "open" to "true" when starting popup.
// When the menu closes, you have open==false and as a return value "selectedEntry"
// The returned "selectedEntry" (and "getSelectedEntry()") are !=-1 only at the exact frame the menu entry is selected.
int render(bool& open) const    {
    selectedEntry = -1;
    if (!open) return selectedEntry;
    const int numEntries = (int) entries.size();
    if (numEntries==0) {
        open = false;
        return selectedEntry;
    }    

    static const ImVec4 transparentColor(1,1,1,0);   
    ImGui::PushStyleColor(ImGuiCol_Button,transparentColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered]);
    ImVec2 iconSize;iconSize.x = iconSize.y = ImGui::GetTextLineHeight();

    ImGui::PushID(&entries);
    //ImGui::BeginPopup(&open);
    ImGui::OpenPopup("MyOwnMenu");
    if (ImGui::BeginPopup("MyOwnMenu")) {
        bool imageClicked = false;
        for (int i = 0; i < numEntries; i++)    {
            const PopupMenuEntry& entry = entries[i];
            imageClicked = false;
            if (entry.user_texture_id) {
                imageClicked = ImGui::ImageButton((void*)entry.user_texture_id,iconSize,entry.uv0,entry.uv1,0,entry.bg_col,entry.tint_col) && entry.selectable;
                ImGui::SameLine();
            }
            if (strlen(entry.text)==0) ImGui::Separator();
            else if (entry.selectable)  {
                if (ImGui::Selectable(entry.text, false) || imageClicked)  {
                    selectedEntry = i;
                    open = false;    // Hide menu
                }
            }
            else ImGui::Text("%s",entry.text);
        }
        if (open)   // close menu when mouse goes away
        {
            ImVec2 pos = ImGui::GetWindowPos();pos.x-=5;pos.y-=5;
            ImVec2 size = ImGui::GetWindowSize();size.x+=10;size.y+=10;
            const ImVec2& mousePos = ImGui::GetIO().MousePos;
            if (mousePos.x<pos.x || mousePos.y<pos.y || mousePos.x>pos.x+size.x || mousePos.y>pos.y+size.y) open = false;
        }
    }
    ImGui::EndPopup();
    ImGui::PopID();
    ImGui::PopStyleColor(2);

    return selectedEntry;
}

bool isEmpty() const {return entries.size()==0;}

};

// Based on the code from: https://github.com/benoitjacquier/imgui
bool ColorChooser(bool* open,ImVec4* pColorOut=NULL, bool supportsAlpha=true);
// Based on the code from: https://github.com/benoitjacquier/imgui
bool ColorCombo(const char* label,ImVec4 *pColorOut=NULL,bool supportsAlpha=false,float width=0.f,bool closeWhenMouseLeavesIt=true);


// Based on the code from: https://github.com/Roflraging (see https://github.com/ocornut/imgui/issues/383)
/*
    *pOptionalCursorPosOut;      // Out
    *pOptionalSelectionStartOut; // Out (== to SelectionEnd when no selection)
    *pOptionalSelectionEndOut;   // Out
*/
bool InputTextMultilineWithHorizontalScrolling(const char* label, char* buf, size_t buf_size, float height, ImGuiInputTextFlags flags = 0, bool* pOptionalIsHoveredOut=NULL, int* pOptionalCursorPosOut=NULL, int* pOptionalSelectionStartOut=NULL, int* pOptionalSelectionEndOut=NULL, float SCROLL_WIDTH=2000.f);

// Based on the code from: https://github.com/Roflraging (see https://github.com/ocornut/imgui/issues/383)
/*
  staticBoolVar is true if the popup_menu is open
  The three integers represent the cursorPos, the selectionStart and the selectionEnd position.
  Must be static and be in an array.
*/
bool InputTextMultilineWithHorizontalScrollingAndCopyCutPasteMenu(const char* label, char* buf, int buf_size, float height,bool& staticBoolVar, int* staticArrayOfThreeIntegersHere, ImGuiInputTextFlags flags=0, bool*pOptionalHoveredOut=NULL,float SCROLL_WIDTH=2000.f,const char* copyName=NULL, const char* cutName=NULL, const char *pasteName=NULL);

// label is used as id
// <0 frame_padding uses default frame padding settings. 0 for no padding
bool ImageButtonWithText(ImTextureID texId,const char* label,const ImVec2& imageSize=ImVec2(0,0), const ImVec2& uv0 = ImVec2(0,0),  const ImVec2& uv1 = ImVec2(1,1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0,0,0,0), const ImVec4& tint_col = ImVec4(1,1,1,1));

#ifndef NO_IMGUIVARIOUSCONTROLS_ANIMATEDIMAGE
// One instance per image can feed multiple widgets
struct AnimatedImage {
    // TODO: load still images as fallback and load gifs from memory
    public:
    typedef void (*FreeTextureDelegate)(ImTextureID& texid);
    typedef void (*GenerateOrUpdateTextureDelegate)(ImTextureID& imtexid,int width,int height,int channels,const unsigned char* pixels,bool useMipmapsIfPossible,bool wraps,bool wrapt);
    void SetFreeTextureCallback(FreeTextureDelegate freeTextureCb) {FreeTextureCb=freeTextureCb;}
    void SetGenerateOrUpdateTextureCallback(GenerateOrUpdateTextureDelegate generateOrUpdateTextureCb) {GenerateOrUpdateTextureCb=generateOrUpdateTextureCb;}

    AnimatedImage(char const *filename,bool useHoverModeIfSupported=false); // 'hoverMode' is supported only if all frames fit 'MaxPersistentTextureSize'
    AnimatedImage(ImTextureID myTexId,int animationImageWidth,int animationImageHeight,int numFrames,int numFramesPerRowInTexture,int numFramesPerColumnInTexture,float delayBetweenFramesInCs,bool useHoverMode=false); // 'hoverMode' always available. 'myTexId' is yours.
    AnimatedImage();    // You'll need to manually call 'load' o 'create'
    ~AnimatedImage();   // calls 'clear'
    void clear();   // releases the textures that are created inside the class

    // Main methods
    void render(ImVec2 size=ImVec2(0,0), const ImVec2& uv0=ImVec2(0,0), const ImVec2& uv1=ImVec2(1,1), const ImVec4& tint_col=ImVec4(1,1,1,1), const ImVec4& border_col=ImVec4(0,0,0,0)) const;
    bool renderAsButton(const char* label,ImVec2 size=ImVec2(0,0), const ImVec2& uv0 = ImVec2(0,0),  const ImVec2& uv1 = ImVec2(1,1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0,0,0,0), const ImVec4& tint_col = ImVec4(1,1,1,1));    // <0 frame_padding uses default frame padding settings. 0 for no padding

    // Less useful methods
    bool load(char const *filename,bool useHoverModeIfSupported=false); // 'hoverMode' is supported only if all frames fit 'MaxPersistentTextureSize'
    bool create(ImTextureID myTexId,int animationImageWidth,int animationImageHeight,int numFrames,int numFramesPerRowInTexture,int numFramesPerColumnInTexture,float delayBetweenFramesInCs,bool useHoverMode=false); // 'hoverMode' always available. 'myTexId' is yours.
    int getWidth() const;
    int getHeight() const;
    int getNumFrames() const;
    bool areAllFramesInASingleTexture() const;  // when true, 'hoverMode' was available in ctr/load/create (but it can't change at runtime)

    static ImVec2 MaxPersistentTextureSize;   // 2048,2048 (Enlarge the buffer if needed for 'hoverMode': but using smaller animated images and less frames is better)

    private:
    AnimatedImage(const AnimatedImage& ) {}     // Actually maybe we could allow some of these for containers...
    void operator=(const AnimatedImage& ) {}
    static FreeTextureDelegate FreeTextureCb;
    static GenerateOrUpdateTextureDelegate GenerateOrUpdateTextureCb;
    friend struct AnimatedImageInternal;
    struct AnimatedImageInternal* ptr;
};
#endif //NO_IMGUIVARIOUSCONTROLS_ANIMATEDIMAGE

// zoomCenter is panning in [(0,0),(1,1)]
// returns true if some user interaction have been processed
bool ImageZoomAndPan(ImTextureID user_texture_id, const ImVec2& size,float aspectRatio,float& zoom,ImVec2& zoomCenter,int panMouseButtonDrag=1,int resetZoomAndPanMouseButton=2,const ImVec2& zoomMaxAndZoomStep=ImVec2(16.f,1.025f));


// Basic tree view implementation
// TODO: See if we can switch context-menu with a single-shot RMB click (when a menu is still open)
class TreeViewNode {
protected:
    friend class TreeView;
    friend struct MyTreeViewHelperStruct;
public:
    enum State {
        STATE_NONE      = 0,
        STATE_OPEN      = 1,
        STATE_SELECTED  = 1<<2,
        STATE_CHECKED   = 1<<3,
        STATE_DEFAULT   = 1<<4,     // user state (but its look is hard-coded)
        STATE_DISABLED  = 1<<5,     // user state (but its look is hard-coded)
        STATE_FORCE_CHECKBOX  = 1<<6, // checkbox always visible (even if disabled in its TreeView)
        STATE_HIDDEN    = 1<<7,       // node (and its child nodes) not visible at all

        // user states (without any specific look)
        STATE_USER5     = 1<<8,

        // user states (but its look can be set in TreeView::getTextColorForStateColor(...))
        STATE_COLOR1    = 1<<9,
        STATE_COLOR2    = 1<<10,    // STATE_COLOR1 wins on it (but only as far as the look is concerned)
        STATE_COLOR3    = 1<<11,    // STATE_COLOR1 and STATE_COLOR2 win on it (but only as far as the look is concerned)

        // user states (without any specific look)
        STATE_USER1     = 1<<12,
        STATE_USER2     = 1<<13,
        STATE_USER3     = 1<<14,
        STATE_USER4     = 1<<15
    };
    mutable int state;

    enum Mode {
        MODE_NONE = 0,

        MODE_ROOT = 1,
        MODE_INTERMEDIATE = 2,
        MODE_LEAF = 4,

        MODE_ROOT_INTERMEDIATE = 3,
        MODE_ROOT_LEAF = 5,
        MODE_INTERMEDIATE_LEAF = 6,
        MODE_ALL = 7,
    };

    enum EventType {
        EVENT_NONE = 0,
        EVENT_STATE_CHANGED,
        EVENT_DOUBLE_CLICKED,
        EVENT_RENAMED
    };
    struct Event {
        TreeViewNode* node;
        EventType type;
        State state;bool wasStateRemoved;
        Event() {set();}
        inline void reset() {set();}
        inline void set(TreeViewNode* _node=NULL,EventType _type=EVENT_NONE,State _state=STATE_NONE,bool _wasStateRemoved=false)    {node=_node;type=_type;state=_state;wasStateRemoved=_wasStateRemoved;}
    };

    struct Data {
        char* displayName;      // can't be NULL (it's set to "" when NULL)
        char* tooltip;          // optional (can be NULL)
        char* userText;         // user stuff, optional (can be NULL)
        int   userId;           // user stuff
        Data(const char* _displayedName=NULL,const char* _tooltip=NULL,const char* _userText=NULL,const int _id=0) : displayName(NULL),tooltip(NULL),userText(NULL),userId(0) {
            set(_displayedName,_tooltip,_userText,_id);
        }
        Data(const Data& o) : displayName(NULL),tooltip(NULL),userText(NULL),userId(0) {*this=o;}
        ~Data() {
            if (displayName){ImGui::MemFree(displayName);displayName=NULL;}
            if (tooltip)    {ImGui::MemFree(tooltip);tooltip=NULL;}
            if (userText)   {ImGui::MemFree(userText);userText=NULL;}
            userId=0;
        }
        void set(const char* _displayName=NULL,const char* _tooltip=NULL,const char* _userText=NULL,const int _id=0) {
            SetString(displayName,_displayName,false);
            SetString(tooltip,_tooltip,true);
            SetString(userText,_userText,true);
            userId = _id;
        }        
        inline static void SetString(char*& destText,const char* text,bool allowNullDestText=true) {
            if (destText) {ImGui::MemFree(destText);destText=NULL;}
            const char e = '\0';
            if (!text && !allowNullDestText) text=&e;
            if (text)  {
                const int sz = strlen(text);
                destText = (char*) ImGui::MemAlloc(sz+1);strcpy(destText,text);
            }
        }       
        const Data& operator=(const Data& o) {
            set(o.displayName,o.tooltip,o.userText,o.userId);
            return *this;
        }
    };


protected:

    static TreeViewNode* CreateNode(const Data& _data,TreeViewNode* _parentNode=NULL,int nodeIndex=-1,bool addEmptyChildNodeVector=false);

    void render(void *ptr, int numIndents=1);

public:

    inline TreeViewNode* addChildNode(const Data& _data,int nodeIndex=-1,bool addEmptyChildNodeVector=false)    {
        return CreateNode(_data,this,nodeIndex,addEmptyChildNodeVector);
    }
    inline TreeViewNode* addSiblingNode(const Data& _data,int parentNodeIndex=-1)    {
        IM_ASSERT(parentNode);
        return CreateNode(_data,parentNode,parentNodeIndex);
    }
    static void DeleteNode(TreeViewNode* n);

    class TreeView& getTreeView();              // slightly slow
    const class TreeView& getTreeView() const;  // slightly slow
    TreeViewNode* getParentNode();
    const TreeViewNode* getParentNode() const;
    int getNodeIndex() const;
    void moveNodeTo(int nodeIndex);
    inline bool isLeafNode() const {return childNodes==NULL;}       // Please note that non-leaf nodes can have childNodes->size()==0
    inline bool isRootNode() const {return !parentNode || !parentNode->parentNode;}
    inline int getNumChildNodes() const {return childNodes ? childNodes->size() : 0;}
    inline TreeViewNode* getChildNode(int index=0) {return (childNodes && childNodes->size()>index) ? (*childNodes)[index] : NULL;}
    inline const TreeViewNode* getChildNode(int index=0) const {return (childNodes && childNodes->size()>index) ? (*childNodes)[index] : NULL;}
    void deleteAllChildNodes(bool leaveEmptyChildNodeVector=false);
    void addEmptyChildNodeVector();         // Only works if "childNodes==NULL" (and allocates it)
    void removeEmptyChildNodeVector();      // Only works if (childNodes->size()==0" (and deallocates it)
    int getNumSiblings(bool includeMe=true) const;
    TreeViewNode* getSiblingNode(int nodeIndexInParentHierarchy=-1);
    const TreeViewNode* getSiblingNode(int nodeIndexInParentHierarchy=-1) const;
    int getDepth() const;   // root nodes have depth = 0

    static void Swap(TreeViewNode*& n1,TreeViewNode*& n2); // untested
    void startRenamingMode();       // starts renaming the node
    bool isInRenamingMode() const;

    void sortChildNodes(bool recursive,int (*comp)(const void *, const void *));
    void sortChildNodesByDisplayName(bool recursive=false,bool reverseOrder=false);
    void sortChildNodesByTooltip(bool recursive=false,bool reverseOrder=false);
    void sortChildNodesByUserText(bool recursive=false,bool reverseOrder=false);
    void sortChildNodesByUserId(bool recursive=false,bool reverseOrder=false);

    inline void addState(int stateFlag) const {state|=stateFlag;}
    inline void removeState(int stateFlag) const {state&=~stateFlag;}
    inline void toggleState(int stateFlag) const {state^=stateFlag;}
    inline bool isStatePresent(int stateFlag) const {return ((state&stateFlag)==stateFlag);}
    inline bool isStateMissing(int stateFlag) const {return ((state&stateFlag)!=stateFlag);}

    void addStateToAllChildNodes(int stateFlag, bool recursive = false) const;
    void removeStateFromAllChildNodes(int stateFlag, bool recursive = false) const;
    bool isStatePresentInAllChildNodes(int stateFlag) const;
    bool isStateMissingInAllChildNodes(int stateFlag) const;

    void addStateToAllDescendants(int stateFlag) const {addStateToAllChildNodes(stateFlag,true);}
    void removeStateFromAllDescendants(int stateFlag) const {removeStateFromAllChildNodes(stateFlag,true);}
    bool isStatePresentInAllDescendants(int stateFlag) const;
    bool isStateMissingInAllDescendants(int stateFlag) const;

    // These return the first matching parentNode (if "recursive==false" they return node->parentNode or NULL).
    TreeViewNode* getFirstParentNodeWithState(int stateFlag,bool recursive=true);
    const TreeViewNode* getFirstParentNodeWithState(int stateFlag,bool recursive=true) const;
    TreeViewNode* getFirstParentNodeWithoutState(int stateFlag,bool recursive=true);
    const TreeViewNode* getFirstParentNodeWithoutState(int stateFlag,bool recursive=true) const;

    // if "recursive==true" deleting the "result nodes" in order shouldn't work, but probably it works in the reverse order (TO TEST)
    void getAllChildNodesWithState(ImVector<TreeViewNode*>& result,int stateFlag,bool recursive = false,bool clearResultBeforeUsage=true) const;
    void getAllChildNodesWithoutState(ImVector<TreeViewNode*>& result,int stateFlag,bool recursive = false,bool clearResultBeforeUsage=true) const;

    // To remove
/*
    void dbgDisplay(const int indent = 0) const {
        for (int i=0;i<indent;i++) printf(" ");
        printf("%s (%s) (%d) [parent=%s]\n",data.displayName?data.displayName:"NULL",data.text?data.text:"NULL",data.id,parentNode?(parentNode->data.displayName):"NULL");
        if (childNodes && childNodes->size()>0) {
            for (int i=0;i<childNodes->size();i++) {
                TreeViewNode* n = (*childNodes)[i];
                if (n) {n->dbgDisplay(indent+3);}
            }
        }
    }
*/

    // "data" accessors
    inline const char* getDisplayName() const {return data.displayName;}
    inline void setDisplayName(const char* _displayName) {Data::SetString(data.displayName,_displayName,false);}
    inline const char* getTooltip() const {return data.tooltip;}
    inline void setTooltip(const char* _tooltip) {Data::SetString(data.tooltip,_tooltip,true);}
    inline const char* getUserText() const {return data.userText;}
    inline void setUserText(const char* _userText) {Data::SetString(data.userText,_userText,true);}
    inline int& getUserId() {return data.userId;}
    inline const int& getUserId() const {return data.userId;}
    inline void setUserId(int uid) {data.userId=uid;}




    void *userPtr;  // Not mine

protected:

    TreeViewNode(const TreeViewNode::Data& _data=TreeViewNode::Data(), TreeViewNode* _parentNode=NULL, int nodeIndex=-1, bool addEmptyChildNodeVector=false);
    virtual ~TreeViewNode();

    Data data;

    TreeViewNode* parentNode;
    ImVector<TreeViewNode*>* childNodes;

    inline unsigned int getMode() const {
        int m = MODE_NONE;if (childNodes==NULL) m|=MODE_LEAF;
        if (!parentNode || !parentNode->parentNode) m|=MODE_ROOT;
        if (m==MODE_NONE) m = MODE_INTERMEDIATE;return m;
    }
    inline static bool MatchMode(unsigned int m,unsigned int nodeM) {
        // Hp) nodeM can't be MODE_NONE
        return (m==MODE_ALL || (m!=MODE_NONE && (m&nodeM)));
    }

protected:
    TreeViewNode(const TreeViewNode&) {}
    void operator=(const TreeViewNode&) {}
};

class TreeView : protected TreeViewNode {
protected:
friend class TreeViewNode;
friend struct MyTreeViewHelperStruct;
public:

    TreeView(Mode _selectionMode=MODE_ALL,bool _allowMultipleSelection=false,Mode _checkboxMode=MODE_NONE,bool _allowAutoCheckboxBehaviour=true,bool _inheritDisabledLook=true);
    virtual ~TreeView();
    bool isInited() {return inited;}
    bool render();  // Main method (makes inited = true). Returns "lastEvent", containing the node that's changed in some way (e.g. double-clicked, end-edited or basic state changed)
    inline Event& getLastEvent() const {return lastEvent;}

    inline int getNumRootNodes() const {return childNodes->size();}
    inline TreeViewNode* getRootNode(int index=0) {return ((childNodes->size()>index) ? (*childNodes)[index] : NULL);}
    inline const TreeViewNode* getRootNode(int index=0) const {return ((childNodes->size()>index) ? (*childNodes)[index] : NULL);}

    inline TreeViewNode* addRootNode(const TreeViewNode::Data& _data,int nodeIndex=-1,bool addEmptyChildNodeVector=false)    {
        return TreeViewNode::CreateNode(_data,this,nodeIndex,addEmptyChildNodeVector);
    }
    inline static void DeleteNode(TreeViewNode* n) {TreeViewNode::DeleteNode(n);}    

    void clear();

    // sorting
    void sortRootNodes(bool recursive,int (*comp)(const void *, const void *))  {TreeViewNode::sortChildNodes(recursive,comp);}
    void sortRootNodesByDisplayName(bool recursive=false,bool reverseOrder=false) {TreeViewNode::sortChildNodesByDisplayName(recursive,reverseOrder);}
    void sortRootNodesByTooltip(bool recursive=false,bool reverseOrder=false) {TreeViewNode::sortChildNodesByTooltip(recursive,reverseOrder);}
    void sortRootNodesByUserText(bool recursive=false,bool reverseOrder=false) {TreeViewNode::sortChildNodesByUserText(recursive,reverseOrder);}
    void sortRootNodesByUserId(bool recursive=false,bool reverseOrder=false) {TreeViewNode::sortChildNodesByUserId(recursive,reverseOrder);}

    // state
    void addStateToAllRootNodes(int stateFlag, bool recursive = false) const {TreeViewNode::addStateToAllChildNodes(stateFlag,recursive);}
    void removeStateFromAllRootNodes(int stateFlag, bool recursive = false) const {TreeViewNode::removeStateFromAllChildNodes(stateFlag,recursive);}
    bool isStatePresentInAllRootNodes(int stateFlag) const {return TreeView::isStatePresentInAllChildNodes(stateFlag);}
    bool isStateMissingInAllRootNodes(int stateFlag) const {return TreeView::isStateMissingInAllChildNodes(stateFlag);}

    // These methods are related to the whole node hierarchy
    void addStateToAllDescendants(int stateFlag) const {TreeViewNode::addStateToAllDescendants(stateFlag);}
    void removeStateFromAllDescendants(int stateFlag) const {TreeViewNode::removeStateFromAllDescendants(stateFlag);}
    bool isStatePresentInAllDescendants(int stateFlag) const {return TreeViewNode::isStatePresentInAllDescendants(stateFlag);}
    bool isStateMissingInAllDescendants(int stateFlag) const {return TreeViewNode::isStateMissingInAllDescendants(stateFlag);}

    // if "recursive==true" deleting the "result nodes" in order shouldn't work, but probably it works in the reverse order (TO TEST)
    void getAllRootNodesWithState(ImVector<TreeViewNode*>& result,int stateFlag,bool recursive = false,bool clearResultBeforeUsage=true) const {TreeViewNode::getAllChildNodesWithState(result,stateFlag,recursive,clearResultBeforeUsage);}
    void getAllRootNodesWithoutState(ImVector<TreeViewNode*>& result,int stateFlag,bool recursive = false,bool clearResultBeforeUsage=true) const {TreeViewNode::getAllChildNodesWithoutState(result,stateFlag,recursive,clearResultBeforeUsage);}


    // Callbacks:
    typedef void (*TreeViewNodeCallback)(TreeViewNode* node,TreeView& parent,void* userPtr);
    void setTreeViewNodePopupMenuDrawerCb(TreeViewNodeCallback cb,void* userPtr=NULL) {treeViewNodePopupMenuDrawerCb = cb;treeViewNodePopupMenuDrawerCbUserPtr = userPtr;}
    inline TreeViewNodeCallback getTreeViewNodePopupMenuDrawerCb() const {return treeViewNodePopupMenuDrawerCb;}
    inline static const char* GetTreeViewNodePopupMenuName() {return "TreeViewNodePopupMenu";}  // you can use this name inside the callback: e.g. ImGui::BeginPopup(ImGui::TreeView::GetTreeViewNodePopupMenuName());
    // must return true if icon is hovered. If set, use ImGui::SameLine() before returning
    typedef bool (*TreeViewNodeDrawIconCallback)(TreeViewNode* node,TreeView& parent,void* userPtr);
    void setTreeViewNodeDrawIconCb(TreeViewNodeDrawIconCallback cb,void* userPtr=NULL) {treeViewNodeDrawIconCb = cb;treeViewNodeDrawIconCbUserPtr = userPtr;}
    inline TreeViewNodeDrawIconCallback getTreeViewNodeDrawIconCb() const {return treeViewNodeDrawIconCb;}
    // just called after all rendering in this node (can be used to append stuff at the right of the line)
    typedef void (*TreeViewNodeAfterDrawCallback)(TreeViewNode* node,TreeView& parent,float windowWidth,void* userPtr);
    void setTreeViewNodeAfterDrawCb(TreeViewNodeAfterDrawCallback cb,void* userPtr=NULL) {treeViewNodeAfterDrawCb = cb;treeViewNodeAfterDrawCbUserPtr = userPtr;}
    inline TreeViewNodeAfterDrawCallback getTreeViewNodeAfterDrawCb() const {return treeViewNodeAfterDrawCb;}
    // called after a node is created and before it's deleted (usable for TreeViewNode::userPtrs)
    typedef void (*TreeViewNodeCreationDelationCallback)(TreeViewNode* node,TreeView& parent,bool delation,void* userPtr);
    void setTreeViewNodeCreationDelationCb(TreeViewNodeCreationDelationCallback cb,void* userPtr=NULL) {treeViewNodeCreationDelationCb = cb;treeViewNodeCreationDelationCbUserPtr = userPtr;}
    inline TreeViewNodeCreationDelationCallback getTreeViewNodeCreationDelationCb() const {return treeViewNodeCreationDelationCb;}


    void *userPtr;                  // user stuff, not mine

    ImVec4* getTextColorForStateColor(int aStateColorFlag) const;
    ImVec4* getTextDisabledColorForStateColor(int aStateColorFlag) const;

    void setTextColorForStateColor(int aStateColorFlag,const ImVec4& textColor,float disabledTextColorAlphaFactor=0.5f) const;

//-------------------------------------------------------------------------------
#       if (!defined(NO_IMGUIHELPER) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
public:
        bool save(ImGuiHelper::Serializer& s);
        bool save(const char* filename);
        static bool Save(const char* filename, TreeView **pTreeViews, int numTreeviews);
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
public:
        bool load(ImGuiHelper::Deserializer& d,const char** pOptionalBufferStart=NULL);
        bool load(const char* filename);
        static bool Load(const char* filename,TreeView** pTreeViews,int numTreeviews);
#       endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#       endif //NO_IMGUIHELPER_SERIALIZATION
//--------------------------------------------------------------------------------

    static void SetFontCheckBoxGlyphs(const char* emptyState,const char* fillState);
    static inline bool HasCustomCheckBoxGlyphs() {return FontCheckBoxGlyphs[0][0]!='\0';}
    static void SetFontArrowGlyphs(const char* leftArrow,const char* downArrow);
    static inline bool HasCustomArrowGlyphs() {return FontArrowGlyphs[0][0]!='\0';}


    // TODO: Fix stuff in this area ------------------------------------------------
    // we leave these public...     // to make protected
    unsigned int selectionMode;     // it SHOULD be a TreeViewNode::Mode, but we use a uint to fit ImGui::ComboFlag(...) directly (to change)
    bool allowMultipleSelection;

    unsigned int checkboxMode;
    bool allowAutoCheckboxBehaviour;

    bool inheritDisabledLook;   // Not the STATE_DISABLED flag! Just the look. (Can we replace it with a unsigned int for generic state look-inheritance?)
    //------------------------------------------------------------------------------

protected:
    TreeViewNodeCallback treeViewNodePopupMenuDrawerCb;
    void* treeViewNodePopupMenuDrawerCbUserPtr;
    TreeViewNodeDrawIconCallback treeViewNodeDrawIconCb;
    void* treeViewNodeDrawIconCbUserPtr;
    TreeViewNodeAfterDrawCallback treeViewNodeAfterDrawCb;
    void* treeViewNodeAfterDrawCbUserPtr;
    TreeViewNodeCreationDelationCallback treeViewNodeCreationDelationCb;
    void* treeViewNodeCreationDelationCbUserPtr;

    bool inited;
    mutable ImVec4 stateColors[6];  // 3 pairs of textColor-textDisabledColor

    mutable Event lastEvent;

    static char FontCheckBoxGlyphs[2][5];
    static char FontArrowGlyphs[2][5];


protected:
    TreeView(const TreeView&) {}
    void operator=(const TreeView&) {}
    TreeView(const TreeViewNode&) {}
    void operator=(const TreeViewNode&) {}



};
typedef TreeViewNode::Data TreeViewNodeData;
typedef TreeViewNode::State TreeViewNodeState;
typedef TreeViewNode::Mode TreeViewNodeMode;
typedef TreeViewNode::Event TreeViewEvent;



} // namespace ImGui



#endif
