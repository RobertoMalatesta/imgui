#include "imguitabwindow.h"
#include <imgui_internal.h>
#include <imgui.h>  // intellisense

// TODO: Clean this code, it's a mess!



namespace ImGui {

namespace DrawListHelper {

// Two main additions:
// 1) PathFillAndStroke in the same method (so that we don't have to build the path twice)
// 2) VerticalGradient: looks good but must be very slow (I keep converting ImU32 <-> ImVec4 hundreds of times to lerp values)
inline static void GetVerticalGradientTopAndBottomColors(ImU32 c,float fillColorGradientDeltaIn0_05,ImU32& tc,ImU32& bc)  {
    if (fillColorGradientDeltaIn0_05<=0) {tc=bc=c;return;}
    if (fillColorGradientDeltaIn0_05>0.5f) fillColorGradientDeltaIn0_05=0.5f;
    const ImVec4 cf = ColorConvertU32ToFloat4(c);
    ImVec4 tmp(cf.x+fillColorGradientDeltaIn0_05<=1.f?cf.x+fillColorGradientDeltaIn0_05:1.f,
               cf.y+fillColorGradientDeltaIn0_05<=1.f?cf.y+fillColorGradientDeltaIn0_05:1.f,
               cf.z+fillColorGradientDeltaIn0_05<=1.f?cf.z+fillColorGradientDeltaIn0_05:1.f,
               cf.w+fillColorGradientDeltaIn0_05<=1.f?cf.w+fillColorGradientDeltaIn0_05:1.f);
    tc = ColorConvertFloat4ToU32(tmp);
    tmp=ImVec4(cf.x-fillColorGradientDeltaIn0_05>0.f?cf.x-fillColorGradientDeltaIn0_05:0.f,
               cf.y-fillColorGradientDeltaIn0_05>0.f?cf.y-fillColorGradientDeltaIn0_05:0.f,
               cf.z-fillColorGradientDeltaIn0_05>0.f?cf.z-fillColorGradientDeltaIn0_05:0.f,
               cf.w-fillColorGradientDeltaIn0_05>0.f?cf.w-fillColorGradientDeltaIn0_05:0.f);
    bc = ColorConvertFloat4ToU32(tmp);
}
inline static ImU32 GetVerticalGradient(const ImVec4& ct,const ImVec4& cb,float DH,float H)    {
    IM_ASSERT(H!=0);
    const float fa = DH/H;
    const float fc = (1.f-fa);
    return ColorConvertFloat4ToU32(ImVec4(
        ct.x * fc + cb.x * fa,
        ct.y * fc + cb.y * fa,
        ct.z * fc + cb.z * fa,
        ct.w * fc + cb.w * fa)
    );
}
void ImDrawListAddConvexPolyFilledWithVerticalGradient(ImDrawList *dl, const ImVec2 *points, const int points_count, ImU32 colTop, ImU32 colBot, bool anti_aliased,float miny=-1.f,float maxy=-1.f)
{
    if (!dl) return;
    if (colTop==colBot)  {
        dl->AddConvexPolyFilled(points,points_count,colTop,anti_aliased);
        return;
    }
    const ImVec2 uv = GImGui->FontTexUvWhitePixel;
    anti_aliased &= GImGui->Style.AntiAliasedShapes;
    //if (ImGui::GetIO().KeyCtrl) anti_aliased = false; // Debug

    int height=0;
    if (miny<=0 || maxy<=0) {
        const float max_float = 999999999999999999.f;
        miny=max_float;maxy=-max_float;
        for (int i = 0; i < points_count; i++) {
            const float h = points[i].y;
            if (h < miny) miny = h;
            else if (h > maxy) maxy = h;
        }
    }
    height = maxy-miny;
    const ImVec4 colTopf = ColorConvertU32ToFloat4(colTop);
    const ImVec4 colBotf = ColorConvertU32ToFloat4(colBot);


    if (anti_aliased)
    {
        // Anti-aliased Fill
        const float AA_SIZE = 1.0f;
        //const ImU32 col_trans = col & 0x00ffffff;
        const ImVec4 colTransTopf(colTopf.x,colTopf.y,colTopf.z,0.f);
        const ImVec4 colTransBotf(colBotf.x,colBotf.y,colBotf.z,0.f);
        const int idx_count = (points_count-2)*3 + points_count*6;
        const int vtx_count = (points_count*2);
        dl->PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = dl->_VtxCurrentIdx;
        unsigned int vtx_outer_idx = dl->_VtxCurrentIdx+1;
        for (int i = 2; i < points_count; i++)
        {
            dl->_IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); dl->_IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+((i-1)<<1)); dl->_IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx+(i<<1));
            dl->_IdxWritePtr += 3;
        }

        // Compute normals
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * sizeof(ImVec2));
        for (int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            ImVec2 diff = p1 - p0;
            diff *= ImInvLength(diff, 1.0f);
            temp_normals[i0].x = diff.y;
            temp_normals[i0].y = -diff.x;
        }

        for (int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            ImVec2 dm = (n0 + n1) * 0.5f;
            float dmr2 = dm.x*dm.x + dm.y*dm.y;
            if (dmr2 > 0.000001f)
            {
                float scale = 1.0f / dmr2;
                if (scale > 100.0f) scale = 100.0f;
                dm *= scale;
            }
            dm *= AA_SIZE * 0.5f;

            // Add vertices
            //_VtxWritePtr[0].pos = (points[i1] - dm); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            //_VtxWritePtr[1].pos = (points[i1] + dm); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            dl->_VtxWritePtr[0].pos = (points[i1] - dm); dl->_VtxWritePtr[0].uv = uv; dl->_VtxWritePtr[0].col = GetVerticalGradient(colTopf,colBotf,points[i1].y-miny,height);        // Inner
            dl->_VtxWritePtr[1].pos = (points[i1] + dm); dl->_VtxWritePtr[1].uv = uv; dl->_VtxWritePtr[1].col = GetVerticalGradient(colTransTopf,colTransBotf,points[i1].y-miny,height);  // Outer
            dl->_VtxWritePtr += 2;

            // Add indexes for fringes
            dl->_IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx+(i1<<1)); dl->_IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+(i0<<1)); dl->_IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx+(i0<<1));
            dl->_IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx+(i0<<1)); dl->_IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx+(i1<<1)); dl->_IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx+(i1<<1));
            dl->_IdxWritePtr += 6;
        }
        dl->_VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count-2)*3;
        const int vtx_count = points_count;
        dl->PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            //_VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            dl->_VtxWritePtr[0].pos = points[i]; dl->_VtxWritePtr[0].uv = uv; dl->_VtxWritePtr[0].col = GetVerticalGradient(colTopf,colBotf,points[i].y-miny,height);
            dl->_VtxWritePtr++;
        }
        for (int i = 2; i < points_count; i++)
        {
            dl->_IdxWritePtr[0] = (ImDrawIdx)(dl->_VtxCurrentIdx); dl->_IdxWritePtr[1] = (ImDrawIdx)(dl->_VtxCurrentIdx+i-1); dl->_IdxWritePtr[2] = (ImDrawIdx)(dl->_VtxCurrentIdx+i);
            dl->_IdxWritePtr += 3;
        }
        dl->_VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}
void ImDrawListPathFillWithVerticalGradientAndStroke(ImDrawList *dl, const ImU32 &fillColorTop, const ImU32 &fillColorBottom, const ImU32 &strokeColor, bool strokeClosed, float strokeThickness, bool antiAliased,float miny,float maxy)    {
    if (!dl) return;
    if (fillColorTop==fillColorBottom) dl->AddConvexPolyFilled(dl->_Path.Data,dl->_Path.Size, fillColorTop, antiAliased);
    else if ((fillColorTop >> 24) != 0 || (fillColorBottom >> 24) != 0) ImDrawListAddConvexPolyFilledWithVerticalGradient(dl, dl->_Path.Data, dl->_Path.Size, fillColorTop, fillColorBottom, antiAliased,miny,maxy);
    if ((strokeColor>> 24)!= 0 && strokeThickness>0) dl->AddPolyline(dl->_Path.Data, dl->_Path.Size, strokeColor, strokeClosed, strokeThickness, antiAliased);
    dl->PathClear();
}
void ImDrawListPathFillAndStroke(ImDrawList *dl, const ImU32 &fillColor, const ImU32 &strokeColor, bool strokeClosed, float strokeThickness, bool antiAliased)    {
    if (!dl) return;
    if ((fillColor >> 24) != 0) dl->AddConvexPolyFilled(dl->_Path.Data, dl->_Path.Size, fillColor, antiAliased);
    if ((strokeColor>> 24)!= 0 && strokeThickness>0) dl->AddPolyline(dl->_Path.Data, dl->_Path.Size, strokeColor, strokeClosed, strokeThickness, antiAliased);
    dl->PathClear();
}
void ImDrawListAddRect(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, const ImU32 &fillColor, const ImU32 &strokeColor, float rounding, int rounding_corners, float strokeThickness, bool antiAliased) {
    if (!dl || (((fillColor >> 24) == 0) && ((strokeColor >> 24) == 0)))  return;
    dl->PathRect(a, b, rounding, rounding_corners);
    ImDrawListPathFillAndStroke(dl,fillColor,strokeColor,true,strokeThickness,antiAliased);
}
void ImDrawListAddRectWithVerticalGradient(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, const ImU32 &fillColorTop, const ImU32 &fillColorBottom, const ImU32 &strokeColor, float rounding, int rounding_corners, float strokeThickness, bool antiAliased) {
    if (!dl || (((fillColorTop >> 24) == 0) && ((fillColorBottom >> 24) == 0) && ((strokeColor >> 24) == 0)))  return;
    dl->PathRect(a, b, rounding, rounding_corners);
    ImDrawListPathFillWithVerticalGradientAndStroke(dl,fillColorTop,fillColorBottom,strokeColor,true,strokeThickness,antiAliased,a.y,b.y);
}
void ImDrawListAddRectWithVerticalGradient(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, const ImU32 &fillColor, float fillColorGradientDeltaIn0_05, const ImU32 &strokeColor, float rounding, int rounding_corners, float strokeThickness, bool antiAliased)
{
    ImU32 fillColorTop,fillColorBottom;GetVerticalGradientTopAndBottomColors(fillColor,fillColorGradientDeltaIn0_05,fillColorTop,fillColorBottom);
    ImDrawListAddRectWithVerticalGradient(dl,a,b,fillColorTop,fillColorBottom,strokeColor,rounding,rounding_corners,strokeThickness,antiAliased);
}

}   //DrawListHelper

// TabLabelStyle --------------------------------------------------------------------------------------------------
TabLabelStyle TabLabelStyle::style;
const char* TabLabelStyle::ColorNames[TabLabelStyle::Col_TabLabel_Count] = {
"Col_TabLabel","Col_TabLabelHovered","Col_TabLabelActive","Col_TabLabelBorder","Col_TabLabelText",
"Col_TabLabelSelected","Col_TabLabelSelectedHovered","Col_TabLabelSelectedActive","Col_TabLabelSelectedBorder",
"Col_TabLabelSelectedText","Col_TabLabelCloseButtonHovered","Col_TabLabelCloseButtonActive","Col_TabLabelCloseButtonBorder","Col_TabLabelCloseButtonTextHovered"
};
// These bit operations are probably non-endian independent, but ImGui uses them too and so do I.
inline static ImU32 ColorMergeWithStyleAlpha(ImU32 c) {
    ImU32 alpha = ((float)(c>>24))*GImGui->Style.Alpha;
    return ((c&0x00FFFFFF)|(alpha<<24));
}
inline static ImU32 ColorMergeWithStyleAlpha(ImU32 c,float alphaMult) {
    ImU32 alpha = ((float)(c>>24))*alphaMult;
    return ((c&0x00FFFFFF)|(alpha<<24));
}
TabLabelStyle::TabLabelStyle()    {
    colors[Col_TabLabel]                   = ImColor(39,44,48,255);
    colors[Col_TabLabelHovered]            = ImColor(59,64,68,255);
    colors[Col_TabLabelActive]             = ImColor(78,86,82,255);
    colors[Col_TabLabelBorder]             = ImColor(23,27,30,255);
    colors[Col_TabLabelText]               = ImColor(160,164,167,255);

    const float alphaSelected = .35f;
    colors[Col_TabLabelSelected]           = ColorMergeWithStyleAlpha(colors[Col_TabLabel],alphaSelected);
    colors[Col_TabLabelSelectedHovered]    = ColorMergeWithStyleAlpha(colors[Col_TabLabelHovered],alphaSelected);
    colors[Col_TabLabelSelectedActive]     = ColorMergeWithStyleAlpha(colors[Col_TabLabelActive],alphaSelected);
    colors[Col_TabLabelSelectedBorder]     = ColorMergeWithStyleAlpha(colors[Col_TabLabelBorder],alphaSelected);
    colors[Col_TabLabelSelectedText]       = ColorMergeWithStyleAlpha(ImColor(130,134,137,225),.7f);

    colors[Col_TabLabelCloseButtonHovered]       = ImColor(166,0,11,255);
    colors[Col_TabLabelCloseButtonActive]        = ImColor(206,40,51,255);
    colors[Col_TabLabelCloseButtonBorder]        = colors[Col_TabLabelBorder];
    colors[Col_TabLabelCloseButtonTextHovered]   = colors[Col_TabLabelText];

    fillColorGradientDeltaIn0_05 = 0.2f;//0.05f; // vertical gradient if > 0 (looks nice but it's very slow)
    rounding = 9.f;
    borderWidth = 2.f;

    closeButtonRounding = 0.f;
    closeButtonBorderWidth = 1.f;
    closeButtonTextWidth = 3.f;

    font = fontSelected = NULL;

    antialiasing = false;
}

bool TabLabelStyle::Edit(TabLabelStyle &s)  {
    bool changed = false;
    const float dragSpeed = 0.25f;
    const char prec[] = "%1.1f";
    ImGui::PushID(&s);

    changed|=ImGui::DragFloat("fillColorGradientDeltaIn0_05",&s.fillColorGradientDeltaIn0_05,0.01f,0.f,.5f,"%1.3f");
    changed|=ImGui::DragFloat("rounding",&s.rounding,dragSpeed,0.0f,16.f,prec);
    changed|=ImGui::DragFloat("borderWidth",&s.borderWidth,.01f,0.f,5.f,"%1.2f");
    ImGui::Spacing();

    changed|=ImGui::DragFloat("closeButtonRounding",&s.closeButtonRounding,dragSpeed,0.0f,16.f,prec);
    changed|=ImGui::DragFloat("closeButtonBorderWidth",&s.closeButtonBorderWidth,.01f,0.f,5.f,"%1.2f");
    changed|=ImGui::DragFloat("closeButtonTextWidth",&s.closeButtonTextWidth,.01f,0.f,5.f,"%1.2f");
    ImGui::Spacing();

    changed|=ImGui::Checkbox("antialiasing",&s.antialiasing);
    ImGui::Spacing();

    static int item=0;
    ImGui::Combo("Color",&item, &TabLabelStyle::ColorNames[0], TabLabelStyle::Col_TabLabel_Count, -1);
    ImGui::Spacing();
    ImVec4 tmp = ImColor(s.colors[item]);
    const bool color_changed = ImGui::ColorEdit4("color",&tmp.x);
    if (color_changed) s.colors[item] = ImColor(tmp);
    changed|=color_changed;

    ImGui::PopID();
    return changed;
}
const TabLabelStyle& TabLabelStyle::GetMergedWithWindowAlpha()    {
    static TabLabelStyle S;
    static int frameCnt=-1;
    if (frameCnt!=ImGui::GetFrameCount())   {
        frameCnt=ImGui::GetFrameCount();
        S = TabLabelStyle::style;
    for (int i=0;i<Col_TabLabel_Count;i++) S.colors[i] = ColorMergeWithStyleAlpha(style.colors[i]);
    }
    return S;
}
inline const TabLabelStyle& TabLabelStyleGetMergedWithAlphaForOverlayUsage()    {
    static TabLabelStyle S;static int frameCnt=-1;static const float alpha = 0.75f;
    if (frameCnt!=ImGui::GetFrameCount())   {
        frameCnt=ImGui::GetFrameCount();
        S = TabLabelStyle::style;
    for (int i=0;i<TabLabelStyle::Col_TabLabel_Count;i++) S.colors[i] = ColorMergeWithStyleAlpha(TabLabelStyle::style.colors[i],alpha);
    }
    return S;
}
//----------------------------------------------------------------------------------------------------------------------

//=======================================================================================
// Main method to draw the tab label
// The TabLabelStyle used by this method won't be merged with the Window Alpha (please provide a pOptionalStyleToUseIn using TabLabelStyle::GetMergedWithWIndowAlpha() if needed).
static bool TabButton(const char *label, bool selected, bool *pCloseButtonPressedOut=NULL, const char* textOverrideIn=NULL, ImVec2 *pJustReturnItsSizeHereOut=NULL, const TabLabelStyle* pOptionalStyleToUseIn=NULL,ImFont *fontOverride=NULL, ImVec2 *pOptionalJustDrawTabButtonGraphicsUnderMouseWithThisOffset=NULL, ImDrawList *drawListOverride=NULL)  {
    // Based on ImGui::ButtonEx(...)
    bool *pHoveredOut = NULL;           // removed from args (can be queried from outside)
    bool *pCloseButtonHovered = NULL;   // removed from args (who cares if the close button is hovered?)
    const int flags = 0;                // what's this ?
    const bool hasCloseButton = pCloseButtonHovered || pCloseButtonPressedOut;

    const bool isFakeControl = pOptionalJustDrawTabButtonGraphicsUnderMouseWithThisOffset || pJustReturnItsSizeHereOut;

    ImGuiWindow* window = GetCurrentWindow();
    if (window && window->SkipItems && !isFakeControl)  return false;

    //ImGuiState& g = *GImGui;
    const ImGuiStyle& style = ImGui::GetStyle();
    const TabLabelStyle& tabStyle = pOptionalStyleToUseIn ? *pOptionalStyleToUseIn : TabLabelStyle::Get();
    const ImGuiID id = isFakeControl ? 0 : window->GetID(label);
    if (textOverrideIn) label = textOverrideIn;

    if (!fontOverride) fontOverride = selected ? tabStyle.fontSelected : tabStyle.font;
    if (fontOverride) ImGui::PushFont(fontOverride);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    ImVec2 pos = window ? window->DC.CursorPos : ImVec2(0,0);
    if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset)    pos.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;
    ImVec2 size(label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);
    float btnWidth = label_size.y*0.75f,btnSpacingX = label_size.y*0.25f;
    float extraWidthForBtn = hasCloseButton ? (btnSpacingX*2.f+btnWidth) : 0;
    if (hasCloseButton) size.x+=extraWidthForBtn;
    if (pJustReturnItsSizeHereOut) {*pJustReturnItsSizeHereOut=size;if (fontOverride) ImGui::PopFont();return false;}

    const ImRect bb(pOptionalJustDrawTabButtonGraphicsUnderMouseWithThisOffset ? *pOptionalJustDrawTabButtonGraphicsUnderMouseWithThisOffset : pos,
                    (pOptionalJustDrawTabButtonGraphicsUnderMouseWithThisOffset ? *pOptionalJustDrawTabButtonGraphicsUnderMouseWithThisOffset : pos) + size);
    if (!pOptionalJustDrawTabButtonGraphicsUnderMouseWithThisOffset) {
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, &id)) {if (fontOverride) ImGui::PopFont();return false;}
    }

    //if (window->DC.ButtonRepeat) flags |= ImGuiButtonFlags_Repeat;    // What's this ?
    bool hovered=false, held=false;
    bool pressed = isFakeControl ? false : ButtonBehavior(bb, id, &hovered, &held, flags);
    bool btnHovered = false;
    bool btnPressed = false;
    ImVec2 startBtn(0,0),endBtn(0,0);
    if (hasCloseButton)    {
        startBtn = ImVec2(bb.Max.x-extraWidthForBtn+btnSpacingX*0.5f,bb.Min.y+(size.y-btnWidth)*0.5f);
        endBtn = ImVec2(startBtn.x+btnWidth,startBtn.y+btnWidth);
        if (!isFakeControl) {
            btnHovered = hovered && ImGui::IsMouseHoveringRect(startBtn,endBtn);
            btnPressed = pressed && btnHovered;
            if (btnPressed) pressed = false;
            if (pCloseButtonHovered) *pCloseButtonHovered = btnHovered;
            if (pCloseButtonPressedOut) * pCloseButtonPressedOut = btnPressed;
        }
    }
    if (pHoveredOut) *pHoveredOut = hovered && !btnHovered;  // We may choose not to return "hovered" when the close btn is hovered.

    // Render

    const ImU32 col = (hovered && !btnHovered && held) ? tabStyle.colors[selected ? TabLabelStyle::Col_TabLabelSelectedActive : TabLabelStyle::Col_TabLabelActive] : (hovered && !btnHovered) ? tabStyle.colors[selected ? TabLabelStyle::Col_TabLabelSelectedHovered : TabLabelStyle::Col_TabLabelHovered] : tabStyle.colors[selected ? TabLabelStyle::Col_TabLabelSelected : TabLabelStyle::Col_TabLabel];
    const ImU32 colText = tabStyle.colors[selected ? TabLabelStyle::Col_TabLabelSelectedText : TabLabelStyle::Col_TabLabelText];

    if (!drawListOverride) drawListOverride = window->DrawList;

    // Canvas
    DrawListHelper::ImDrawListAddRectWithVerticalGradient(drawListOverride,bb.Min, bb.Max,col,tabStyle.fillColorGradientDeltaIn0_05,tabStyle.colors[selected ? TabLabelStyle::Col_TabLabelSelectedBorder : TabLabelStyle::Col_TabLabelBorder],tabStyle.rounding,1|2,tabStyle.borderWidth,tabStyle.antialiasing);

    // Text
    ImGui::PushStyleColor(ImGuiCol_Text,ImGui::ColorConvertU32ToFloat4(colText));
    if (!pOptionalJustDrawTabButtonGraphicsUnderMouseWithThisOffset)  RenderTextClipped(bb.Min,ImVec2(bb.Max.x-extraWidthForBtn,bb.Max.y), label, NULL, &label_size, ImGuiAlign_Center | ImGuiAlign_VCenter);
    else    {
        ImVec2 textPos(bb.Min.x+(bb.Max.x-bb.Min.x-label_size.x-extraWidthForBtn)*0.5f,bb.Min.y+(bb.Max.y-bb.Min.y-label_size.y)*0.5f);
        drawListOverride->AddText(textPos,colText,label);
    }
    ImGui::PopStyleColor();



    //fprintf(stderr,"bb.Min=%d,%d bb.Max=%d,%d label_size=%d,%d extraWidthForBtn=%d\n",(int)bb.Min.x,(int)bb.Min.y,(int)bb.Max.x,(int)bb.Max.y,(int)label_size.x,(int)label_size.y,(int)extraWidthForBtn);
    if (hasCloseButton) {
    const ImU32 col = (held && btnHovered) ? tabStyle.colors[TabLabelStyle::Col_TabLabelCloseButtonActive] : btnHovered ? tabStyle.colors[TabLabelStyle::Col_TabLabelCloseButtonHovered] : 0;
    if (btnHovered) DrawListHelper::ImDrawListAddRect(drawListOverride,startBtn, endBtn, col,tabStyle.colors[TabLabelStyle::Col_TabLabelCloseButtonBorder],tabStyle.closeButtonRounding,0x0F,tabStyle.closeButtonBorderWidth,tabStyle.antialiasing);

        const float cross_extent = (btnWidth * 0.5f * 0.7071f);// - 1.0f;
        const ImVec2 center((startBtn.x+endBtn.x)*0.5f,(startBtn.y+endBtn.y)*0.5f);
        const ImU32 cross_col = tabStyle.colors[(btnHovered) ? TabLabelStyle::Col_TabLabelCloseButtonTextHovered : selected ? TabLabelStyle::Col_TabLabelSelectedText : TabLabelStyle::Col_TabLabelText];//btnHovered ? 0xFFFF0000 : ImGui::GetColorU32(ImGuiCol_Text);
        drawListOverride->AddLine(center + ImVec2(+cross_extent,+cross_extent), center + ImVec2(-cross_extent,-cross_extent), cross_col,tabStyle.closeButtonTextWidth);
        drawListOverride->AddLine(center + ImVec2(+cross_extent,-cross_extent), center + ImVec2(-cross_extent,+cross_extent), cross_col,tabStyle.closeButtonTextWidth);

    }
    if (fontOverride) ImGui::PopFont();

    return pressed;
}
//========================================================================================
// Main code starts here


void TabWindow::TabLabel::DestroyTabLabel(TabWindow::TabLabel*& tab)  {
    if (TabWindow::TabLabelDeletingCb) TabWindow::TabLabelDeletingCb(tab);
    tab->~TabLabel();
    ImGui::MemFree(tab);
    tab=NULL;
}

struct TabNode  {
    friend class TabLabel;
    friend class TabWindow;
    friend struct TabWindowDragData;

    ImVector<TabWindow::TabLabel* > tabs;   // only in leaf nodes
    TabWindow::TabLabel* selectedTab;
    TabNode *parent;   // (reference)
    TabNode *child[2];  // (owned)
    char* name;         // (owned)
    float splitterPerc; // in [0,1]
    bool horizontal;
    TabNode() {tabs.clear();selectedTab=NULL;parent=NULL;for (int i=0;i<2;i++) child[i]=NULL;name=NULL;
               horizontal=false;splitterPerc=0.5f;}
    ~TabNode() {
        clear();
        if (name) {ImGui::MemFree(name);name=NULL;}
    }
    inline bool isLeafNode() const {return (!child[0] && !child[1]);}
    void clear()  {
        for (int i=0;i<2;i++) {
            TabNode*& ch = child[i];
            if (ch) {
                ch->clear();  // delete child nodes too
                ch->~TabNode();
                ImGui::MemFree(ch);
                ch=NULL;
            }
        }
        for (int i=0,isz=tabs.size();i<isz;i++) {
            TabWindow::TabLabel*& tab = tabs[i];
            TabWindow::TabLabel::DestroyTabLabel(tab);
        }
        tabs.clear();
    }
    TabNode *addTabLabel(TabWindow::TabLabel *tab, int childPosLTRB=-1, int pos=-1)     {
        IM_ASSERT(tab);
        IM_ASSERT(this->isLeafNode());
        IM_ASSERT(!parent || (!parent->isLeafNode() && parent->child[0] && parent->child[1] && parent->tabs.size()==0));
        if (childPosLTRB==-1)   {
            if (pos<0 || pos>tabs.size()) pos=tabs.size();
            tabs.push_back(tab);
            for (int i=tabs.size()-2;i>=pos;--i) tabs[i+1] = tabs[i];
            tabs[pos] = tab;
            return this;
        }
        IM_ASSERT(childPosLTRB>=0 && childPosLTRB<4);
        horizontal = (childPosLTRB==1 || childPosLTRB==3);
        splitterPerc = 0.5f;
        const bool spFirst = (childPosLTRB==0 || childPosLTRB==1);
        // create the two child nodes
        for (int i=0;i<2;i++)   {
            TabNode* ch = child[i];
            ch = (TabNode*) ImGui::MemAlloc(sizeof(TabNode));
            new (ch) TabNode();
            child[i] = ch;
            ch->parent = this;
            /*if (name) {
            int sz = strlen(name)+8;
            ch->name = (char*) ImGui::MemAlloc(sz);
            strcpy(ch->name,name);
            strcat(ch->name,".child");
            sprintf(&ch->name[sz-2],"%d",i);
            ch->name[sz-1]='\0';
            //fprintf(stderr,"Added TabNode: \"%s\"\n",ch->name); // "##main.child3"
        }*/
        }
        assignChildNames(false);

        // We must move tabs to child[]:
        TabNode* ch = spFirst ? child[1] : child[0];
        ch->tabs.resize(tabs.size());
        for (int i=0,isz=tabs.size();i<isz;i++) {
            TabWindow::TabLabel* tab = tabs[i];
            ch->tabs[i] = tab;
        }
        tabs.clear();
        ch->selectedTab = selectedTab;
        selectedTab = NULL;
        // We must insert tab
        ch = spFirst ? child[0] : child[1];
        ch->selectedTab = tab;
        return ch->addTabLabel(tab,-1,pos);
    }
    TabNode* findTabLabel(TabWindow::TabLabel* tab,bool recursive=false)  {
        if (!tab) return NULL;
        if (recursive) {
            TabNode * n = NULL;
            for (int i=0;i<2;i++)
                if ((n=child[i]->findTabLabel(tab,true))) return n;
        }
        for (int i=0,isz=tabs.size();i<isz;i++)
            if (tabs[i]==tab) return this;
        return NULL;
    }
    TabWindow::TabLabel* findTabLabelFromUserPtr(void* value,TabNode** pOptionalParentNodeOut=NULL)  {
        TabWindow::TabLabel* tab = NULL;
        for (int i=0;i<2;i++)
            if ((tab=child[i]->findTabLabelFromUserPtr(value,pOptionalParentNodeOut))!=NULL) return tab;
        for (int i=0,isz=tabs.size();i<isz;i++)
            if (tabs[i]->userPtr==value) {
                if (pOptionalParentNodeOut) *pOptionalParentNodeOut = this;
                return tabs[i];
            }
        return NULL;
    }
    TabWindow::TabLabel* findTabLabelFromUserText(const char* value,TabNode** pOptionalParentNodeOut=NULL)  {
        TabWindow::TabLabel* tab = NULL;
        for (int i=0;i<2;i++)
            if ((tab=child[i]->findTabLabelFromUserText(value,pOptionalParentNodeOut))!=NULL) return tab;
        for (int i=0,isz=tabs.size();i<isz;i++)
            if (tabs[i]->userText && strcmp(tabs[i]->userText,value)==0) {
                if (pOptionalParentNodeOut) *pOptionalParentNodeOut = this;
                return tabs[i];
            }
        return NULL;
    }
    TabWindow::TabLabel* findTabLabelFromTooltip(const char* value,TabNode** pOptionalParentNodeOut=NULL)  {
        TabWindow::TabLabel* tab = NULL;
        for (int i=0;i<2;i++)
            if ((tab=child[i]->findTabLabelFromTooltip(value,pOptionalParentNodeOut))!=NULL) return tab;
        for (int i=0,isz=tabs.size();i<isz;i++)
            if (strcmp(tabs[i]->tooltip,value)==0) {
                if (pOptionalParentNodeOut) *pOptionalParentNodeOut = this;
                return tabs[i];
            }
        return NULL;
    }
    bool removeTabLabel(TabWindow::TabLabel* tab,bool recursive=false,TabNode** pOptionalActiveTabNodeToChange=NULL,bool dontDeleteTabLabel=false)  {
        if (!tab) return false;
        if (recursive) {
            for (int i=0;i<2;i++)
                if (child[i] && child[i]->removeTabLabel(tab,true,pOptionalActiveTabNodeToChange,dontDeleteTabLabel)) return true;
        }
        IM_ASSERT(tab);
        IM_ASSERT(this->isLeafNode());
        for (int i=0,isz=tabs.size();i<isz;i++) {
            if (tabs[i]==tab) {
                if (selectedTab == tab) selectedTab = NULL;
                if (!dontDeleteTabLabel) TabWindow::TabLabel::DestroyTabLabel(tabs[i]);
                for (int j=i;j<isz-1;j++) tabs[j] = tabs[j+1];
                tabs.pop_back();
                if (tabs.size()==0 && parent) {
                    // We must merge this with parent
                    TabNode* parent = this->parent;
                    IM_ASSERT(parent->child[0] && parent->child[1]);
                    IM_ASSERT(parent->child[0]==this || parent->child[1]==this);
                    IM_ASSERT(parent->child[0]!=parent->child[1]);

                    int id = parent->child[0]==this ? 0 : 1;
                    // delete parent->child[id]: it's empty (Hey! that's me! Am I allowed delete myself?)
                    {
                        TabNode* ch = parent->child[id];
                        IM_ASSERT(ch==this);
                        IM_ASSERT(ch->isLeafNode());
                        parent->child[id] = NULL;
                        if (pOptionalActiveTabNodeToChange && *pOptionalActiveTabNodeToChange==ch) *pOptionalActiveTabNodeToChange=parent;
                        IM_ASSERT(ch->tabs.size()==0);
                        // We defer deleting it at the bottom of this method for extended safety
                    }
                    // merge the other child with parent
                    id = (id == 1) ? 0 : 1;// other parent child
                    {
                        TabNode* ch = parent->child[id];
                        if (ch->isLeafNode())   {
                            if (pOptionalActiveTabNodeToChange && *pOptionalActiveTabNodeToChange==ch) *pOptionalActiveTabNodeToChange=parent;
                            IM_ASSERT(parent->tabs.size()==0);
                            parent->tabs.resize(ch->tabs.size());
                            for (int i=0,isz=ch->tabs.size();i<isz;i++) {
                                parent->tabs[i] = ch->tabs[i];
                            }
                            ch->tabs.clear();
                            parent->selectedTab = ch->selectedTab;
                            parent->splitterPerc = 0.5f;

                            parent->child[id] = NULL;
                        }
                        else {
                            IM_ASSERT(ch->tabs.size()==0);
                            IM_ASSERT(parent->tabs.size()==0);

                            // We must replace "parent" with "ch" and then delete "parent"
                            // Nope: it's better to "deep clone "ch" to "parent" and delete "ch"

                            if (pOptionalActiveTabNodeToChange && *pOptionalActiveTabNodeToChange==ch) *pOptionalActiveTabNodeToChange=parent;

                            if (ch->name) {ImGui::MemFree(ch->name);ch->name=NULL;}
                            parent->child[0] = ch->child[0];ch->child[0]=NULL;
                            parent->child[1] = ch->child[1];ch->child[1]=NULL;ch->parent=NULL;
                            parent->child[0]->parent = parent->child[1]->parent = parent;
                            parent->horizontal = ch->horizontal;
                            parent->selectedTab = ch->selectedTab;
                            parent->splitterPerc = ch->splitterPerc;
                            parent->assignChildNames(true);

                        }

                        // delete the other child
                        ch->~TabNode();
                        ImGui::MemFree(ch);
                        ch = NULL;
                        // delete me
                        ch = this;
                        ch->~TabNode();
                        ImGui::MemFree(ch);
                    }



                }
                return true;
            }
        }
        return false;
    }
    bool isEmpty(bool recursive=false) {
        if (tabs.size()!=0) return false;
        if (recursive) {
            for (int i=0;i<2;i++)
                if (child[i] && !child[i]->isEmpty(true)) return false;
        }
        return true;
    }
    TabNode* getFirstLeaftNode() {return isLeafNode() ? this : child[0]->getFirstLeaftNode();}
    void setName(const char* lbl)  {
        if (name) {ImGui::MemFree(name);name=NULL;}
        const char e = '\0';if (!lbl) lbl=&e;
        const int sz = strlen(lbl)+1;
        name = (char*) ImGui::MemAlloc(sz+1);strcpy(name,lbl);
    }
    void assignChildNames(bool recursive=false)  {
        const int sz = strlen(name)+8;
        for (int i=0;i<2;i++) {
            TabNode* ch = child[i];
            if (!ch) continue;
            if (ch->name) {ImGui::MemFree(ch->name);ch->name=NULL;}
            ch->name = (char*) ImGui::MemAlloc(sz);
            strcpy(ch->name,name);
            strcat(ch->name,".child");
            sprintf(&ch->name[sz-2],"%d",i);
            ch->name[sz-1]='\0';
            if (recursive) ch->assignChildNames(true);
        }
    }

    void render(const ImVec2& windowSize,struct MyTabWindowHelperStruct *ptr);
};


struct TabWindowDragData {
    TabWindow::TabLabel* draggingTabSrc;
    TabNode* draggingTabNodeSrc;
    ImGuiWindow* draggingTabImGuiWindowSrc;
    TabWindow* draggingTabWindowSrc;
    ImVec2 draggingTabSrcSize;
    ImVec2 draggingTabSrcOffset;
    bool draggingTabSrcIsSelected;

    TabWindow::TabLabel* draggingTabDst;
    TabNode* draggingTabNodeDst;
    ImGuiWindow* draggingTabImGuiWindowDst;
    TabWindow* draggingTabWindowDst;

    TabWindowDragData() {reset();}
    void resetDraggingSrc() {
        draggingTabSrc = NULL;
        draggingTabNodeSrc = NULL;
        draggingTabImGuiWindowSrc = NULL;
        draggingTabWindowSrc = NULL;
        draggingTabSrcSize = draggingTabSrcOffset = ImVec2(0,0);
        draggingTabSrcIsSelected = false;
    }
    void resetDraggingDst() {
        draggingTabDst = NULL;
        draggingTabNodeDst = NULL;
        draggingTabImGuiWindowDst = NULL;
        draggingTabWindowDst = NULL;
    }
    inline void reset() {resetDraggingSrc();resetDraggingDst();}
    inline bool isDraggingSrcValid() const {
        return (draggingTabSrc && draggingTabNodeSrc && draggingTabImGuiWindowSrc);
    }
    inline bool isDraggingDstValid() const {
        return (draggingTabDst && draggingTabNodeDst && draggingTabImGuiWindowDst);
    }
    inline int findDraggingSrcIndex(const TabWindow::TabLabel* tab=NULL) const {
        if (!tab) tab = draggingTabSrc;
        for (int i=0,isz=draggingTabNodeSrc->tabs.size();i<isz;i++) {
            if (draggingTabNodeSrc->tabs[i] == tab) return i;
        }
        return -1;
    }
    inline int findDraggingDstIndex(const TabWindow::TabLabel* tab=NULL) const {
        if (!tab) tab = draggingTabDst;
        for (int i=0,isz=draggingTabNodeDst->tabs.size();i<isz;i++) {
            if (draggingTabNodeDst->tabs[i] == tab) return i;
        }
        return -1;
    }
    inline static TabNode* FindTabNodeByName(TabNode* firstNode,const char* name,int numCharsToMatch=-1) {
        if ((numCharsToMatch==-1 && strcmp(firstNode->name,name)==0)
            || (strncmp(firstNode->name,name,numCharsToMatch)==0)) return firstNode;
        TabNode* rv = NULL;
        for (int i=0;i<2;i++)   {
            TabNode* ch = firstNode->child[i];
            if (ch && (rv=FindTabNodeByName(ch,name,numCharsToMatch))) return rv;
        }
        return NULL;
    }

    inline void drawDragButton(ImDrawList* drawList,const ImVec2& wp,const ImVec2& mp)   {
        const TabLabelStyle& tabStyle = TabLabelStyleGetMergedWithAlphaForOverlayUsage();
        ImVec2 start(wp.x+mp.x-draggingTabSrcOffset.x-draggingTabSrcSize.x*0.5f,wp.y+mp.y-draggingTabSrcOffset.y-draggingTabSrcSize.y*0.5f);
        bool mustCloseTab = false;
        ImGui::TabButton(NULL,false,draggingTabSrc->closable ? &mustCloseTab : NULL,draggingTabSrc->getLabel(),NULL,&tabStyle,draggingTabSrcIsSelected ? tabStyle.fontSelected : tabStyle.font,&start,drawList);
    }
    inline void drawProhibitionSign(ImDrawList* drawList,const ImVec2& wp,const ImVec2& pos,float size,float alpha=0.5f)   {
        ImVec2 start(wp.x+pos.x-size*0.5f,wp.y+pos.y-size*0.5f);
        const ImVec2 end(start.x+size,start.y+size);
        const ImVec4 color(1.f,1.f,1.f,alpha);
        drawList->AddImage(TabWindow::DockPanelIconTextureID,start,end,ImVec2(0.5f,0.75f),ImVec2(0.75f,1.f),ImGui::ColorConvertFloat4ToU32(color));
   }

};
static TabWindowDragData gDragData;
struct MyTabWindowHelperStruct {
    bool isRMBclicked;
    static bool isMouseDragging;
    static bool LockedDragging; // better dragging experience when trying to drag non-draggable tab labels
    bool isASplitterActive;
    static TabWindow::TabLabel* tabLabelPopup;
    static bool tabLabelPopupChanged;

    TabWindow* tabWindow;

    static ImVector<TabWindow::TabLabel*> TabsToClose;
    static ImVector<TabNode*> TabsToCloseNodes;
    static ImVector<TabWindow*> TabsToCloseParents;

    ImVec2 itemSpacing;

    ImVec4 splitterColor;
    ImVec4 splitterColorHovered;
    ImVec4 splitterColorActive;

    float textHeightWithSpacing;

    MyTabWindowHelperStruct(TabWindow* _tabWindow) {
        isMouseDragging = ImGui::IsMouseDragging(0,2.f);
        isRMBclicked = ImGui::IsMouseClicked(1);
        isASplitterActive = false;
        tabWindow = _tabWindow;

        ImGuiStyle& style = ImGui::GetStyle();
        itemSpacing =   style.ItemSpacing;

        splitterColor = ImVec4(TabWindow::SplitterColor.x,TabWindow::SplitterColor.y,TabWindow::SplitterColor.z,0.2f);
        splitterColorHovered = ImVec4(TabWindow::SplitterColor.x,TabWindow::SplitterColor.y,TabWindow::SplitterColor.z,0.35f);
        splitterColorActive = ImVec4(TabWindow::SplitterColor.x,TabWindow::SplitterColor.y,TabWindow::SplitterColor.z,0.5f);

        storeStyleVars();

        textHeightWithSpacing = ImGui::GetTextLineHeightWithSpacing();
    }
    ~MyTabWindowHelperStruct() {restoreStyleVars();}
    inline void storeStyleVars() {ImGui::GetStyle().ItemSpacing = ImVec2(1,1);}
    inline void restoreStyleVars() {ImGui::GetStyle().ItemSpacing = itemSpacing;}

    inline static void ResetTabsToClose() {
        TabsToClose.clear();TabsToCloseNodes.clear();TabsToCloseParents.clear();
    }
};
TabWindow::TabLabel* MyTabWindowHelperStruct::tabLabelPopup = NULL;
bool  MyTabWindowHelperStruct::tabLabelPopupChanged = false;
bool MyTabWindowHelperStruct::isMouseDragging = false;
bool MyTabWindowHelperStruct::LockedDragging = false;
ImVector<TabWindow::TabLabel*> MyTabWindowHelperStruct::TabsToClose;
ImVector<TabNode*> MyTabWindowHelperStruct::TabsToCloseNodes;
ImVector<TabWindow*> MyTabWindowHelperStruct::TabsToCloseParents;
ImVec4 TabWindow::SplitterColor(1,1,1,1);
float TabWindow::SplitterSize(8.f);
TabWindow::TabLabelCallback TabWindow::WindowContentDrawerCb=NULL;
void* TabWindow::WindowContentDrawerUserPtr=NULL;
TabWindow::TabLabelCallback TabWindow::TabLabelPopupMenuDrawerCb=NULL;
void* TabWindow::TabLabelPopupMenuDrawerUserPtr=NULL;
TabWindow::TabLabelClosingCallback TabWindow::TabLabelClosingCb=NULL;
void* TabWindow::TabLabelClosingUserPtr=NULL;
TabWindow::TabLabelDeletingCallback TabWindow::TabLabelDeletingCb=NULL;

void TabNode::render(const ImVec2 &windowSize, MyTabWindowHelperStruct *ptr)
{   
    const float splitterSize = TabWindow::SplitterSize;
    bool splitterActive = false;
    MyTabWindowHelperStruct& mhs = *ptr;


    IM_ASSERT(name);
    if (child[0])   {
        IM_ASSERT(child[1]);
        IM_ASSERT(tabs.size()==0);
        ImGui::BeginChild(name,windowSize,false,ImGuiWindowFlags_NoScrollbar);

        ImVec2 ws = windowSize;
        float splitterPercToPixels = 0.f,splitterDelta = 0.f;
        if (horizontal && ws.y>splitterSize) {
            ws.y-=splitterSize;
            splitterPercToPixels = ws.y*splitterPerc;
            child[0]->render(ImVec2(ws.x,splitterPercToPixels),ptr);
            // Horizontal Splitter ------------------------------------------
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
            ImGui::PushStyleColor(ImGuiCol_Button,mhs.splitterColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,mhs.splitterColorHovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,mhs.splitterColorActive);
            ImGui::PushID(this);
            ImGui::Button("##splitter0", ImVec2(ws.x,splitterSize));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            splitterActive = !mhs.isASplitterActive && ImGui::IsItemActive();
            mhs.isASplitterActive |= splitterActive;
            if (splitterActive)  splitterDelta = ImGui::GetIO().MouseDelta.y;
            else splitterDelta = 0.f;
            if (splitterActive)  {
                float& h = splitterPercToPixels;
                const float minh = splitterSize;
                const float maxh = ws.y-splitterSize - mhs.textHeightWithSpacing;//20;   Is this correct ?       // Warning: 20.f is hard-coded!
                if (h+splitterDelta>maxh)           splitterDelta = (h!=maxh) ? (maxh-h) : 0.f;
                else if (h+splitterDelta<minh)      splitterDelta = (h!=minh) ? (minh-h) : 0.f;
                h+=splitterDelta;
                splitterPerc = splitterPercToPixels/ws.y;
            }
            ImGui::PopID();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
            //------------------------------------------------------
            child[1]->render(ImVec2(ws.x,ws.y-splitterPercToPixels),ptr);
        }
        else if (!horizontal && ws.x>splitterSize) {
            ws.x-=splitterSize;
            splitterPercToPixels = ws.x*splitterPerc;
            child[0]->render(ImVec2(splitterPercToPixels,ws.y),ptr);
            // Vertical Splitter ------------------------------------------
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
            ImGui::PushStyleColor(ImGuiCol_Button,mhs.splitterColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,mhs.splitterColorHovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,mhs.splitterColorActive);
            ImGui::PushID(this);
            ImGui::SameLine(0,0);
            ImGui::Button("##splitter1", ImVec2(splitterSize,ws.y));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            splitterActive = !mhs.isASplitterActive && ImGui::IsItemActive();
            mhs.isASplitterActive |= splitterActive;
            if (splitterActive)  splitterDelta = ImGui::GetIO().MouseDelta.x;
            else splitterDelta = 0.f;
            if (splitterActive)  {
                float& w = splitterPercToPixels;
                const float minw = splitterSize;
                const float maxw = ws.x-splitterSize;
                if (w + splitterDelta>maxw)         splitterDelta = (w!=maxw) ? (maxw-w) : 0.f;
                else if (w + splitterDelta<minw)    splitterDelta = (w!=minw) ? (minw-w) : 0.f;
                w+=splitterDelta;
                splitterPerc = splitterPercToPixels/ws.x;
            }
            ImGui::SameLine(0,0);
            ImGui::PopID();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
            //------------------------------------------------------
            child[1]->render(ImVec2(ws.x-splitterPercToPixels,ws.y),ptr);
        }
        //else {/* Window too tiny: better not to draw it, otherwise the splitters overlap and may cause bad stuff */}

        ImGui::EndChild();  // name
        return;
    }

    // Leaf Node
    IM_ASSERT(!child[1]);

    ImGui::BeginChild(name,windowSize,false,ImGuiWindowFlags_NoScrollbar);

    //fprintf(stderr,"%s\n",name);

    ImGuiStyle& style = ImGui::GetStyle();
    const TabLabelStyle& tabStyle = TabLabelStyle::GetMergedWithWindowAlpha();
    ImGuiState& g = *GImGui;
    TabWindowDragData& dd = gDragData;

    //TabWindow::TabLabel* hoveredTab = NULL;
    //----------------------------------------------------------------
    {
        //ImGui::BeginGroup();
        const int numTabs = tabs.size();
        if (numTabs>0 && !selectedTab) selectedTab = tabs[0];

        float windowWidth = 0.f,sumX=0.f;
        windowWidth = ImGui::GetWindowWidth() - style.WindowPadding.x - (ImGui::GetScrollMaxY()>0 ? style.ScrollbarSize : 0.f);
        TabWindow::TabLabel* newSelectedTab = selectedTab;

        ImVec2 tabButtonSz(0,0);bool mustCloseTab = false;
        bool selection_changed = false;
        for (int i = 0; i < numTabs; i++)
        {
            TabWindow::TabLabel& tab = *tabs[i];

            if (sumX > 0.f) {
                sumX+=style.ItemSpacing.x;   // Maybe we can skip it if we use SameLine(0,0) below
                //sumX+=ImGui::CalcTextSize(tab.label).x+2.f*style.FramePadding.x;
                ImGui::TabButton(NULL,selectedTab == &tab,tab.closable ? &mustCloseTab : NULL,tab.getLabel(),&tabButtonSz,&tabStyle);
                sumX+=tabButtonSz.x;
                if (sumX>windowWidth) sumX = 0.f;
                else ImGui::SameLine();
            }

            // Draw the button
            mustCloseTab = false;
            ImGui::PushID(&tab);   // otherwise two tabs with the same name would clash.
            if (ImGui::TabButton("",selectedTab == &tab,tab.closable ? &mustCloseTab : NULL,tab.getLabel(),NULL,&tabStyle))   {
                 selection_changed = (selectedTab != &tab);
                 newSelectedTab = &tab;
                 tab.mustSelectNextFrame = false;
            }
            ImGui::PopID();

            if (sumX==0.f) sumX = style.WindowPadding.x + ImGui::GetItemRectSize().x; // First element of a line

            if (tab.mustCloseNextFrame || mustCloseTab) {
                tab.mustCloseNextFrame = false;
                mhs.TabsToClose.push_back(&tab);
                mhs.TabsToCloseNodes.push_back(this);
                mhs.TabsToCloseParents.push_back(mhs.tabWindow);
            }
            else if (ImGui::IsItemHoveredRect()) {
                //hoveredTab = &tab;
                if (tab.tooltip && strlen(tab.tooltip)>0 && &tab!=mhs.tabLabelPopup)  ImGui::SetTooltip("%s",tab.tooltip);                

                if (mhs.isMouseDragging && !mhs.LockedDragging && !mhs.isASplitterActive) {
                    if (!dd.draggingTabSrc) {
                        if (!tab.draggable) mhs.LockedDragging = true;
                        else    {
                        dd.draggingTabSrc = &tab;
                        dd.draggingTabNodeSrc = this;
                        dd.draggingTabImGuiWindowSrc = g.HoveredWindow;
                        dd.draggingTabWindowSrc = mhs.tabWindow;
                        dd.draggingTabSrcIsSelected = (selectedTab == &tab);

                        dd.draggingTabSrcSize = ImGui::GetItemRectSize();
                        const ImVec2& mp = ImGui::GetIO().MousePos;
                        const ImVec2 draggingTabCursorPos = ImGui::GetCursorPos();
                        dd.draggingTabSrcOffset=ImVec2(
                                    mp.x+dd.draggingTabSrcSize.x*0.5f-sumX+ImGui::GetScrollX(),
                                    mp.y+dd.draggingTabSrcSize.y*0.5f-draggingTabCursorPos.y+ImGui::GetScrollY()
                                    );

                        //fprintf(stderr,"Hovered Start Window:%s\n",g.HoveredWindow ? g.HoveredWindow->Name : "NULL");
                        }
                    }
                    else if (dd.draggingTabSrc && !tab.draggable) {
                        // Prohibition sign-------
                        const ImVec2& itemSize = ImGui::GetItemRectSize();
                        const ImVec2 itemPos =ImVec2(
                                sumX-itemSize.x*0.5f-ImGui::GetScrollX(),
                                ImGui::GetCursorPos().y-itemSize.y*0.5f-ImGui::GetScrollY()
                                );
                        ImDrawList* drawList = ImGui::GetWindowDrawList();
                        const ImVec2 wp = g.HoveredWindow->Pos;
                        dd.drawProhibitionSign(drawList,wp,itemPos,dd.draggingTabSrcSize.y*1.2f);
                    }
                }
                else if (dd.draggingTabSrc && dd.draggingTabSrc!=&tab){
                    dd.draggingTabDst = &tab;
                    dd.draggingTabNodeDst = this;
                    dd.draggingTabImGuiWindowDst = g.HoveredWindow;
                    dd.draggingTabWindowDst = mhs.tabWindow;
                }

                if (mhs.isRMBclicked && TabWindow::TabLabelPopupMenuDrawerCb) {
                    //ImGuiState& g = *GImGui; while (g.OpenedPopupStack.size() > 0) g.OpenedPopupStack.pop_back();   // Close all existing context-menus
                    //ImGui::OpenPopup(TabWindow::GetTabLabelPopupMenuName());
                    mhs.tabLabelPopup = (TabWindow::TabLabel*)&tab;
                    mhs.tabLabelPopupChanged = true;
                    // fprintf(stderr,"open popup\n");  // This gets actually called...
                }

            }

        }

        selectedTab = newSelectedTab;
        if (selection_changed) mhs.tabWindow->activeNode = this;

        //ImGui::EndGroup();//allTabsSize = ImGui::GetItemRectSize();
        //----------------------------------------------------------------
        mhs.restoreStyleVars();     // needs matching
        ImGui::BeginChild("user",ImVec2(0,0),false,selectedTab ? selectedTab->wndFlags : 0);
        if (/*selectedTab &&*/ TabWindow::WindowContentDrawerCb) {
            TabWindow::WindowContentDrawerCb(selectedTab,*mhs.tabWindow,TabWindow::WindowContentDrawerUserPtr);
        }
        else {
            if (selectedTab) selectedTab->render();
            else {ImGui::Text("EMPTY TAB LABEL DOCKING SPACE. PLEASE DRAG AND DROP TAB LABELS HERE! And please use ImGui::TabWindow::SetWindowContentDrawerCallback(...) to set a callback to display this text. ");}
        }
        ImGui::EndChild();  // user
        mhs.storeStyleVars();
    }
    //----------------------------------------------------------------

    ImGui::EndChild();  // name

}


void TabWindow::render()
{
    IM_ASSERT(ImGui::GetCurrentWindow());   // Call me inside a window

    if (!init) {init=true;}
    if (!activeNode) activeNode = mainNode->getFirstLeaftNode();

    ImVec2 windowSize = ImGui::GetWindowSize();   // TabWindow::render() must be called inside a Window ATM
    windowSize.x-=2.f*ImGui::GetStyle().WindowPadding.x;
    windowSize.y-=2.f*ImGui::GetStyle().WindowPadding.y;
    TabWindowDragData& dd = gDragData;

    static int frameCnt = -1;
    ImGuiState& g = *GImGui;
    if (frameCnt!=g.FrameCount) {
        frameCnt=g.FrameCount;
        //--------------------------------------------------------------
        // Some "static" actions here:
        //--------------------------------------------------------------
        // 1) Close Tabs
        //--------------------------------------------------------------
        if (MyTabWindowHelperStruct::TabsToClose.size()>0)   {
            const int sz = MyTabWindowHelperStruct::TabsToClose.size();
            ImVector<bool> dontCloseTabLabels;
            dontCloseTabLabels.resize(sz);
            for (int i=0;i<sz;i++) dontCloseTabLabels[i]=false;
            if (TabWindow::TabLabelClosingCb)   {
                TabWindow::TabLabelClosingCb(MyTabWindowHelperStruct::TabsToClose,MyTabWindowHelperStruct::TabsToCloseParents,dontCloseTabLabels,TabWindow::TabLabelClosingUserPtr);
            }
            for (int i=0;i<sz;i++) {
                if (!dontCloseTabLabels[i]) {
                    TabNode* node = MyTabWindowHelperStruct::TabsToCloseNodes[i];
                    TabLabel* tabLabel = MyTabWindowHelperStruct::TabsToClose[i];
                    TabWindow* tabWindow = MyTabWindowHelperStruct::TabsToCloseParents[i];

                    if (MyTabWindowHelperStruct::tabLabelPopup == tabLabel) MyTabWindowHelperStruct::tabLabelPopup = NULL;
                    if (dd.draggingTabSrc == tabLabel) dd.resetDraggingSrc();
                    if (dd.draggingTabDst == tabLabel) dd.resetDraggingDst();

                    if (!node->removeTabLabel(tabLabel,false,&tabWindow->activeNode))   {
                        fprintf(stderr,"Error: Can't delete TabLabel: \"%s\"\n",tabLabel->getLabel());
                    }
                }
            }
        }
        MyTabWindowHelperStruct::ResetTabsToClose();
        // 2) Display Tab Menu ------------------------------------------
        if (TabLabelPopupMenuDrawerCb && MyTabWindowHelperStruct::tabLabelPopup) {
            if (MyTabWindowHelperStruct::tabLabelPopupChanged) {
                MyTabWindowHelperStruct::tabLabelPopupChanged = false;
                ImGuiState& g = *GImGui; while (g.OpenedPopupStack.size() > 0) g.OpenedPopupStack.pop_back();   // Close all existing context-menus
                ImGui::OpenPopup(TabWindow::GetTabLabelPopupMenuName());
            }
            TabLabelPopupMenuDrawerCb(MyTabWindowHelperStruct::tabLabelPopup,*this,TabLabelPopupMenuDrawerUserPtr);
        }
        // 3) Display dragging button only if no hover window is present (otherwise we need to draw something under it before, see below)
        if (!g.HoveredWindow && dd.draggingTabSrc)  {
            const ImVec2& mp = ImGui::GetIO().MousePos;
            const ImVec2 wp = dd.draggingTabImGuiWindowSrc->Pos;
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->PushClipRectFullScreen(); // New
            dd.drawDragButton(drawList,wp,mp);
        }
        //----------------------------------------------------------------
        gDragData.resetDraggingDst();
        //----------------------------------------------------------------
    }

    MyTabWindowHelperStruct mhs(this);
    mainNode->render(windowSize,&mhs);

    // Draw dragging stuff and Apply drag logic -------------------------------------------
    if (g.HoveredRootWindow==ImGui::GetCurrentWindow())
    {
        ImGuiStyle& style = ImGui::GetStyle();
        int hoversInt = 0;  // 1 = center, 3 = center-top, 4 = center-right, 5 = center-bottom, 2 = center-left,

        // Draw tab label while mouse drags it
        if (dd.draggingTabSrc) {
            IM_ASSERT(dd.draggingTabImGuiWindowSrc);
            const ImVec2& mp = ImGui::GetIO().MousePos;
            const ImVec2 wp = dd.draggingTabImGuiWindowSrc->Pos;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            const ImGuiWindow* hoveredWindow = g.HoveredWindow;
            //const ImGuiWindow* hoveredRootWindow = g.HoveredRootWindow;
            int hoveredWindowNameSz = 0;
            // Window -----------------
            if (hoveredWindow && hoveredWindow!=dd.draggingTabImGuiWindowSrc
                    && (hoveredWindowNameSz=strlen(hoveredWindow->Name))>4 && strcmp(&hoveredWindow->Name[hoveredWindowNameSz-4],"user")==0
                    //&& strncmp(g.ActiveIdWindow->Name,hoveredWindow->Name,hoveredWindowNameSz-5)!=0 // works for g.ActiveIdWindow or g.FocusedWindow
                    )
            {

                // Background
                const ImVec2 wp = hoveredWindow->Pos;
                const ImVec2 ws = hoveredWindow->Size;
                ImVec2 start(wp.x,wp.y);
                ImVec2 end(start.x+ws.x,start.y+ws.y);
                const float draggedBtnAlpha = 0.35f;
                const ImVec4& bgColor = style.Colors[ImGuiCol_TitleBg];
                drawList->AddRectFilled(start,end,ImColor(bgColor.x,bgColor.y,bgColor.z,bgColor.w*draggedBtnAlpha),style.FrameRounding);

                // central quad
                const float defaultQuadAlpha = 0.75f;
                const ImTextureID tid = DockPanelIconTextureID;
                ImU32 quadCol = ImColor(1.f,1.f,1.f,defaultQuadAlpha);
                ImU32 quadColHovered = ImColor(0.5f,0.5f,1.f,1.f);
                const float minDim = ws.x < ws.y ? ws.x : ws.y;
                const float MIN_SIZE = 75.f;
                const float centralQuadDim =(minDim*0.25f)>=MIN_SIZE?(minDim*0.25f):
                                                                     (minDim<MIN_SIZE)?minDim:
                                                                                       (minDim*0.5f)>=MIN_SIZE?(minDim*0.5f):
                                                                                                               MIN_SIZE;
                const float singleQuadDim = centralQuadDim*0.3333333334f;
                ImVec2 uv0,uv1;bool hovers;
                // central quad top
                uv0=ImVec2(0.22916f,0.f);uv1=ImVec2(0.45834f,0.22916f);
                start.x = wp.x + (ws.x-singleQuadDim)*0.5f;
                start.y = wp.y + (ws.y-singleQuadDim)*0.5f-singleQuadDim;
                end.x = start.x+singleQuadDim;
                end.y = start.y+singleQuadDim;
                hovers = ImGui::IsMouseHoveringRect(start,end,false);
                if (hovers) hoversInt = 3;
                drawList->AddImage(tid,start,end,uv0,uv1,hovers ? quadColHovered : quadCol);
                // central quad right
                uv0=ImVec2(0.45834f,0.22916f);uv1=ImVec2(0.6875f,0.45834f);
                start.x = wp.x + (ws.x-singleQuadDim)*0.5f + singleQuadDim;
                start.y = wp.y + (ws.y-singleQuadDim)*0.5f;
                end.x = start.x+singleQuadDim;
                end.y = start.y+singleQuadDim;
                hovers = ImGui::IsMouseHoveringRect(start,end,false);
                if (hovers) hoversInt = 4;
                drawList->AddImage(tid,start,end,uv0,uv1,hovers ? quadColHovered : quadCol);
                // central quad bottom
                uv0=ImVec2(0.22916f,0.45834f);uv1=ImVec2(0.45834f,0.6875f);
                start.x = wp.x + (ws.x-singleQuadDim)*0.5f;
                start.y = wp.y + (ws.y-singleQuadDim)*0.5f+singleQuadDim;
                end.x = start.x+singleQuadDim;
                end.y = start.y+singleQuadDim;
                hovers = ImGui::IsMouseHoveringRect(start,end,false);
                if (hovers) hoversInt = 5;
                drawList->AddImage(tid,start,end,uv0,uv1,hovers ? quadColHovered : quadCol);
                // central quad left
                uv0=ImVec2(0.0f,0.22916f);uv1=ImVec2(0.22916f,0.45834f);
                start.x = wp.x + (ws.x-singleQuadDim)*0.5f - singleQuadDim;
                start.y = wp.y + (ws.y-singleQuadDim)*0.5f;
                end.x = start.x+singleQuadDim;
                end.y = start.y+singleQuadDim;
                hovers = ImGui::IsMouseHoveringRect(start,end,false);
                if (hovers) hoversInt = 2;
                drawList->AddImage(tid,start,end,uv0,uv1,hovers ? quadColHovered : quadCol);
                // central quad center
                uv0=ImVec2(0.22916f,0.22916f);uv1=ImVec2(0.45834f,0.45834f);
                start.x = wp.x + (ws.x-singleQuadDim)*0.5f;
                start.y = wp.y + (ws.y-singleQuadDim)*0.5f;
                end.x = start.x+singleQuadDim;
                end.y = start.y+singleQuadDim;
                hovers = //hoversInt==0;
                        ImGui::IsMouseHoveringRect(start,end,false);
                if (hovers) hoversInt = 1;
                drawList->AddImage(tid,start,end,uv0,uv1,hovers ? quadColHovered : quadCol);
                // Refinement: draw remaining 4 inert quads
                uv0=ImVec2(0.f,0.f);uv1=ImVec2(0.22916f,0.22916f);
                start.x = wp.x + (ws.x-singleQuadDim)*0.5f - singleQuadDim;
                start.y = wp.y + (ws.y-singleQuadDim)*0.5f - singleQuadDim;
                end.x = start.x+singleQuadDim;end.y = start.y+singleQuadDim;
                drawList->AddImage(tid,start,end,uv0,uv1,quadCol);
                uv0=ImVec2(0.45834f,0.f);uv1=ImVec2(0.6875f,0.22916f);
                start.x = wp.x + (ws.x-singleQuadDim)*0.5f + singleQuadDim;
                start.y = wp.y + (ws.y-singleQuadDim)*0.5f - singleQuadDim;
                end.x = start.x+singleQuadDim;end.y = start.y+singleQuadDim;
                drawList->AddImage(tid,start,end,uv0,uv1,quadCol);
                uv0=ImVec2(0.f,0.45834f);uv1=ImVec2(0.22916f,0.6875f);
                start.x = wp.x + (ws.x-singleQuadDim)*0.5f - singleQuadDim;
                start.y = wp.y + (ws.y-singleQuadDim)*0.5f + singleQuadDim;
                end.x = start.x+singleQuadDim;end.y = start.y+singleQuadDim;
                drawList->AddImage(tid,start,end,uv0,uv1,quadCol);
                uv0=ImVec2(0.45834f,0.45834f);uv1=ImVec2(0.6875f,0.6875f);
                start.x = wp.x + (ws.x-singleQuadDim)*0.5f + singleQuadDim;
                start.y = wp.y + (ws.y-singleQuadDim)*0.5f + singleQuadDim;
                end.x = start.x+singleQuadDim;end.y = start.y+singleQuadDim;
                drawList->AddImage(tid,start,end,uv0,uv1,quadCol);
            }
            // Button -----------------
            drawList = &g.OverlayDrawList;  // wrong, but otherwise it draws under the other tabs! [Maybe we can use ChannelsSplit(),ChannelsSetCurrent(),ChannelsMerge(), but that would require modifying code in various spots and it's more error prone]
            dd.drawDragButton(drawList,wp,mp);
            // -------------------------------------------------------------------
            ImGui::SetMouseCursor(ImGuiMouseCursor_Move);
        }

        // Drop tab label onto another
        if (dd.draggingTabDst && dd.draggingTabDst->draggable) {
            // swap draggingTabSrc and draggingTabDst
            IM_ASSERT(dd.isDraggingSrcValid());
            IM_ASSERT(dd.isDraggingDstValid());
            IM_ASSERT(dd.draggingTabSrc!=dd.draggingTabDst);

            if (dd.draggingTabNodeSrc!=dd.draggingTabNodeDst) {
                bool srcWasSelected = dd.draggingTabNodeSrc->selectedTab == dd.draggingTabSrc;
                bool dstWasSelected = dd.draggingTabNodeDst->selectedTab == dd.draggingTabDst;
                if (srcWasSelected) dd.draggingTabNodeSrc->selectedTab = dd.draggingTabDst;
                if (dstWasSelected) dd.draggingTabNodeDst->selectedTab = dd.draggingTabSrc;
            }

            const int iSrc = dd.findDraggingSrcIndex();
            IM_ASSERT(iSrc>=0);
            const int iDst = dd.findDraggingDstIndex();
            IM_ASSERT(iDst>=0);
            dd.draggingTabNodeDst->tabs[iDst] = dd.draggingTabSrc;
            dd.draggingTabNodeSrc->tabs[iSrc] = dd.draggingTabDst;

            dd.reset();
            //fprintf(stderr,"Drop tab label onto another\n");
        }

        // Reset draggingTabIndex if necessary
        if (!MyTabWindowHelperStruct::isMouseDragging) {
            if (hoversInt && dd.draggingTabSrc && dd.draggingTabImGuiWindowSrc && dd.draggingTabImGuiWindowSrc!=g.HoveredWindow)
            {
                // Drop tab label onto a window portion
                int nameSz = strlen(g.HoveredWindow->Name);
                static const char trailString[] = ".user";
                static const int trailStringSz = (int) strlen(trailString);
                IM_ASSERT(nameSz>=trailStringSz);
                IM_ASSERT(strcmp(&g.HoveredWindow->Name[nameSz-trailStringSz],trailString)==0);
                const char* startMatchCh = strstr(g.HoveredWindow->Name,".##main"),*startMatchCh2 = NULL;
                if (startMatchCh)   {
                    while ((startMatchCh2 = strstr(&g.HoveredWindow->Name[(int)(startMatchCh-g.HoveredWindow->Name)+7],".##main"))) {
                        startMatchCh = startMatchCh2;
                    }
                }
                const int startMatchIndex = startMatchCh ? ((int)(startMatchCh-g.HoveredWindow->Name)+1) : 0;
                IM_ASSERT(nameSz>=trailStringSz-startMatchIndex);

                ImVector<char> tmp;tmp.resize(nameSz);
                strncpy(&tmp[0],&g.HoveredWindow->Name[startMatchIndex],nameSz-trailStringSz-startMatchIndex);
                tmp[nameSz-trailStringSz-startMatchIndex]='\0';
                //fprintf(stderr,"\"%s\"\n",&tmp[0]);
                dd.draggingTabNodeDst = TabWindowDragData::FindTabNodeByName(mainNode,&tmp[0]);

                //fprintf(stderr,"Item: \"%s\" dragged to window:\"%s\" at pos: %d\n",dd.draggingTabSrc->label,g.HoveredWindow ? g.HoveredWindow->Name : "NULL",hoversInt);
                //if (dd.draggingTabNodeDst)  fprintf(stderr,"dd.draggingTabNodeDst->tabs.size()=%d\n",(int)dd.draggingTabNodeDst->tabs.size());
                //else fprintf(stderr,"No dd.draggingTabNodeDst.\n");
                //TODO: move dd.draggingTabSrc and delete the src node if empty------------
                // How can I find dd.draggingTabNodeDst from g.HoveredWindow->Name?
                // I must strip ".HorizontalStrip.content.user" and then seek TabNode::Name
                //-------------------------------------------------------------------------
                if (dd.draggingTabNodeDst) {
                    if (hoversInt!=1 && dd.draggingTabNodeDst->tabs.size()==0) hoversInt=1;
                    if (!(dd.draggingTabNodeDst==dd.draggingTabNodeSrc && (dd.draggingTabNodeDst->tabs.size()==0 || hoversInt==1))) {
                        // We must:

                        // 1) remove dd.draggingTabSrc from dd.draggingTabNodeSrc
                        if (!dd.draggingTabNodeSrc->removeTabLabel(dd.draggingTabSrc,false,&dd.draggingTabNodeDst,true))   {
                            //fprintf(stderr,"Error: !dd.draggingTabNodeSrc->removeTabLabel(dd.draggingTabSrc,false,&activeNode,true): \"%s\"\n",dd.draggingTabSrc->getLabel());
                        }
                        // 2) append if to dd.draggingTabNodeDst
                        activeNode = dd.draggingTabNodeDst->addTabLabel(dd.draggingTabSrc,hoversInt==1 ? -1 : hoversInt-2);
                        //----------------------------------------------------
                    }
                    //else fprintf(stderr,"Do nothing.\n");
                }

                dd.resetDraggingDst();
            }
            if (dd.draggingTabSrc) dd.resetDraggingSrc();
            MyTabWindowHelperStruct::LockedDragging = false;
        }
    }

}

void TabWindow::clearNodes() {
    if (mainNode)   {
        mainNode->~TabNode();
        ImGui::MemFree(mainNode);
        mainNode=NULL;
    }
    activeNode = NULL;
}
void TabWindow::clear() {mainNode->clear();activeNode=mainNode;}

TabWindow::TabWindow() {
    mainNode = (TabNode*) ImGui::MemAlloc(sizeof(TabNode));
    new (mainNode) TabNode();
    mainNode->name = (char*) ImGui::MemAlloc(7);strcpy(mainNode->name,"##main");
    activeNode=mainNode;
    init=false;
}
TabWindow::~TabWindow() {clearNodes();}

TabWindow::TabLabel *TabWindow::addTabLabel(const char *label, const char *tooltip,bool closable, bool draggable, void *userPtr, const char *userText,int ImGuiWindowFlagsForContent) {
    TabLabel* tab = (TabLabel*) ImGui::MemAlloc(sizeof(TabLabel));
    new (tab) TabLabel(label,tooltip,closable,draggable);
    tab->userPtr = userPtr;
    tab->setUserText(userText);
    tab->wndFlags = ImGuiWindowFlagsForContent;
    if (!activeNode) activeNode = mainNode->getFirstLeaftNode();
    activeNode = activeNode->addTabLabel(tab);
    return tab;
}
TabWindow::TabLabel *TabWindow::addTabLabel(TabLabel *tabLabel, bool checkIfAlreadyPresent) {
    if (!tabLabel) return NULL;
    if (checkIfAlreadyPresent && mainNode->findTabLabel(tabLabel,true)) return tabLabel;
    if (!activeNode) activeNode = mainNode->getFirstLeaftNode();
    activeNode = activeNode->addTabLabel(tabLabel);
    return tabLabel;
}
bool TabWindow::removeTabLabel(TabWindow::TabLabel *tab) {
    if (!tab) return false;
    if (!mainNode->removeTabLabel(tab,true,&activeNode)) {
        fprintf(stderr,"Error: cannot remove TabLabel: \"%s\"\n",tab->getLabel());
        return false;
    }
    return true;
}

TabWindow::TabLabel *TabWindow::findTabLabelFromTooltip(const char *tooltip) const  {
    return mainNode->findTabLabelFromTooltip(tooltip);
}
TabWindow::TabLabel *TabWindow::findTabLabelFromUserPtr(void *userPtr) const    {
    return mainNode->findTabLabelFromUserPtr(userPtr);
}
TabWindow::TabLabel *TabWindow::findTabLabelFromUserText(const char *userText) const    {
    return mainNode->findTabLabelFromUserText(userText);
}
TabWindow::TabLabel *TabWindow::FindTabLabelFromTooltip(const char *tooltip, const TabWindow *pTabWindows, int numTabWindows, int *pOptionalTabWindowIndexOut)    {
    IM_ASSERT(pTabWindows && numTabWindows>0);
    TabLabel* rv = NULL;
    for (int i=0;i<numTabWindows;i++)   {
        if ((rv = pTabWindows[i].findTabLabelFromTooltip(tooltip))) {
            if (pOptionalTabWindowIndexOut) *pOptionalTabWindowIndexOut=i;
            return rv;
        }
    }
    if (pOptionalTabWindowIndexOut) *pOptionalTabWindowIndexOut=-1;
    return NULL;
}
TabWindow::TabLabel *TabWindow::FindTabLabelFromUserPtr(void *userPtr, const TabWindow *pTabWindows, int numTabWindows, int *pOptionalTabWindowIndexOut)    {
    IM_ASSERT(pTabWindows && numTabWindows>0);
    TabLabel* rv = NULL;
    for (int i=0;i<numTabWindows;i++)   {
        if ((rv = pTabWindows[i].findTabLabelFromUserPtr(userPtr))) {
            if (pOptionalTabWindowIndexOut) *pOptionalTabWindowIndexOut=i;
            return rv;
        }
    }
    if (pOptionalTabWindowIndexOut) *pOptionalTabWindowIndexOut=-1;
    return NULL;
}
TabWindow::TabLabel *TabWindow::FindTabLabelFromUserText(const char *userText, const TabWindow *pTabWindows, int numTabWindows, int *pOptionalTabWindowIndexOut)    {
    IM_ASSERT(pTabWindows && numTabWindows>0);
    TabLabel* rv = NULL;
    for (int i=0;i<numTabWindows;i++)   {
        if ((rv = pTabWindows[i].findTabLabelFromUserText(userText))) {
            if (pOptionalTabWindowIndexOut) *pOptionalTabWindowIndexOut=i;
            return rv;
        }
    }
    if (pOptionalTabWindowIndexOut) *pOptionalTabWindowIndexOut=-1;
    return NULL;
}




// Based on the code by krys-spectralpixel (https://github.com/krys-spectralpixel), posted here: https://github.com/ocornut/imgui/issues/261
bool TabLabels(int numTabs, const char** tabLabels, int& selectedIndex, const char** tabLabelTooltips, bool wrapMode, int *pOptionalHoveredIndex, int* pOptionalItemOrdering, bool allowTabReorder, bool allowTabClosing, int *pOptionalClosedTabIndex, int *pOptionalClosedTabIndexInsideItemOrdering) {
    ImGuiStyle& style = ImGui::GetStyle();
    const TabLabelStyle& tabStyle = TabLabelStyle::GetMergedWithWindowAlpha();

    const ImVec2 itemSpacing =  style.ItemSpacing;
    style.ItemSpacing.x =       1;
    style.ItemSpacing.y =       1;

    if (numTabs>0 && (selectedIndex<0 || selectedIndex>=numTabs)) {
        if (!pOptionalItemOrdering)  selectedIndex = 0;
        else selectedIndex = -1;
    }
    if (pOptionalHoveredIndex) *pOptionalHoveredIndex = -1;
    if (pOptionalClosedTabIndex) *pOptionalClosedTabIndex = -1;
    if (pOptionalClosedTabIndexInsideItemOrdering) *pOptionalClosedTabIndexInsideItemOrdering = -1;

    float windowWidth = 0.f,sumX=0.f;
    if (wrapMode) windowWidth = ImGui::GetWindowWidth() - style.WindowPadding.x - (ImGui::GetScrollMaxY()>0 ? style.ScrollbarSize : 0.f);

    static int draggingTabIndex = -1;int draggingTabTargetIndex = -1;   // These are indices inside pOptionalItemOrdering
    static bool draggingTabWasSelected = false;
    static ImVec2 draggingTabSize(0,0);
    static ImVec2 draggingTabOffset(0,0);
    static bool draggingLocked = false;

    const bool isRMBclicked = ImGui::IsMouseClicked(1);
    const bool isMouseDragging = ImGui::IsMouseDragging(0,2.f);
    int justClosedTabIndex = -1,newSelectedIndex = selectedIndex;

    ImVec2 startGroupCursorPos = ImGui::GetCursorPos();
    ImGui::BeginGroup();
    ImVec2 tabButtonSz(0,0);bool mustCloseTab = false;
    bool selection_changed = false;bool noButtonDrawn = true;
    for (int j = 0,i; j < numTabs; j++)
    {
        i = pOptionalItemOrdering ? pOptionalItemOrdering[j] : j;
        if (i==-1) continue;

        if (!wrapMode) {if (!noButtonDrawn) ImGui::SameLine();}
        else if (sumX > 0.f) {
            sumX+=style.ItemSpacing.x;   // Maybe we can skip it if we use SameLine(0,0) below
            ImGui::TabButton(tabLabels[i],(i == selectedIndex),allowTabClosing ? &mustCloseTab : NULL,NULL,&tabButtonSz,&tabStyle);
            sumX+=tabButtonSz.x;
            if (sumX>windowWidth) sumX = 0.f;
            else ImGui::SameLine();
        }

        // Draw the button
        ImGui::PushID(i);   // otherwise two tabs with the same name would clash.
        if (ImGui::TabButton(tabLabels[i],i == selectedIndex,allowTabClosing ? &mustCloseTab : NULL,NULL,NULL,&tabStyle))   {
            selection_changed = (selectedIndex!=i);
            newSelectedIndex = i;
        }
        ImGui::PopID();
        noButtonDrawn = false;

        if (wrapMode) {
            if (sumX==0.f) sumX = style.WindowPadding.x + ImGui::GetItemRectSize().x; // First element of a line
        }
        else if (isMouseDragging && allowTabReorder && pOptionalItemOrdering) {
            // We still need sumX
            if (sumX==0.f) sumX = style.WindowPadding.x + ImGui::GetItemRectSize().x; // First element of a line
            else sumX+=style.ItemSpacing.x + ImGui::GetItemRectSize().x;

        }

        if (ImGui::IsItemHoveredRect() && !mustCloseTab) {
            if (pOptionalHoveredIndex) *pOptionalHoveredIndex = i;
            if (tabLabelTooltips && !isRMBclicked && tabLabelTooltips[i] && strlen(tabLabelTooltips[i])>0)  ImGui::SetTooltip("%s",tabLabelTooltips[i]);

            if (pOptionalItemOrdering)  {
                if (allowTabReorder)  {
            if (isMouseDragging) {
            if (draggingTabIndex==-1 && !draggingLocked) {
                            draggingTabIndex = j;
                            draggingTabWasSelected = (i == selectedIndex);
                            draggingTabSize = ImGui::GetItemRectSize();
                            const ImVec2& mp = ImGui::GetIO().MousePos;
                            const ImVec2 draggingTabCursorPos = ImGui::GetCursorPos();
                            draggingTabOffset=ImVec2(
                                        mp.x+draggingTabSize.x*0.5f-sumX+ImGui::GetScrollX(),
                                        mp.y+draggingTabSize.y*0.5f-draggingTabCursorPos.y+ImGui::GetScrollY()
                                        );

                        }
                    }
                    else if (draggingTabIndex>=0 && draggingTabIndex<numTabs && draggingTabIndex!=j){
                        draggingTabTargetIndex = j; // For some odd reasons this seems to get called only when draggingTabIndex < i ! (Probably during mouse dragging ImGui owns the mouse someway and sometimes ImGui::IsItemHovered() is not getting called)
                    }
                }
            }
        }
        if (mustCloseTab)   {
            justClosedTabIndex = i;
            if (pOptionalClosedTabIndex) *pOptionalClosedTabIndex = i;
            if (pOptionalClosedTabIndexInsideItemOrdering) *pOptionalClosedTabIndexInsideItemOrdering = j;
            pOptionalItemOrdering[j] = -1;
        }

    }
    selectedIndex = newSelectedIndex;
    ImGui::EndGroup();
    ImVec2 groupSize = ImGui::GetItemRectSize();

    // Draw tab label while mouse drags it
    if (draggingTabIndex>=0 && draggingTabIndex<numTabs) {
        const ImVec2 wp = ImGui::GetWindowPos();
        startGroupCursorPos.x+=wp.x;
        startGroupCursorPos.y+=wp.y;
        startGroupCursorPos.x-=ImGui::GetScrollX();
        startGroupCursorPos.y-=ImGui::GetScrollY();
        const float deltaY = ImGui::GetTextLineHeightWithSpacing()*2.5f;
        startGroupCursorPos.y-=deltaY;
        groupSize.y+=2.f*deltaY;
        if (ImGui::IsMouseHoveringRect(startGroupCursorPos,startGroupCursorPos+groupSize))  {
            const ImVec2& mp = ImGui::GetIO().MousePos;
            ImVec2 start(wp.x+mp.x-draggingTabOffset.x-draggingTabSize.x*0.5f,wp.y+mp.y-draggingTabOffset.y-draggingTabSize.y*0.5f);
            //const ImVec2 end(start.x+draggingTabSize.x,start.y+draggingTabSize.y);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const TabLabelStyle& tabStyle = TabLabelStyleGetMergedWithAlphaForOverlayUsage();
            ImGui::TabButton(tabLabels[pOptionalItemOrdering[draggingTabIndex]],false,allowTabClosing ? &mustCloseTab : NULL,NULL,NULL,&tabStyle,draggingTabWasSelected ? tabStyle.fontSelected : tabStyle.font,&start,drawList);
            ImGui::SetMouseCursor(ImGuiMouseCursor_Move);

            if (TabWindow::DockPanelIconTextureID)	{
                // Optional: draw prohibition sign when dragging too far (you can remove this if you want)
                startGroupCursorPos.y+=deltaY*.5f;
                groupSize.y-=deltaY;
                if (!ImGui::IsMouseHoveringRect(startGroupCursorPos,startGroupCursorPos+groupSize))  {
                    const float signWidth = draggingTabSize.y*1.25f;
                    start.x+=(draggingTabSize.x-signWidth)*0.5f;
                    start.y+=(draggingTabSize.y-signWidth)*0.5f;
                    const ImVec2 end(start.x+signWidth,start.y+signWidth);
                    const ImVec4 color(1.f,1.f,1.f,0.85f);
                    drawList->AddImage(TabWindow::DockPanelIconTextureID,start,end,ImVec2(0.5f,0.75f),ImVec2(0.75f,1.f),ImGui::ColorConvertFloat4ToU32(color));
                }
            }
        }
        else {
            draggingTabIndex = -1;draggingTabTargetIndex=-1;
            draggingLocked = true;// consume one mouse release
        }
    }

    // Drop tab label
    if (draggingTabTargetIndex!=-1) {
        // swap draggingTabIndex and draggingTabTargetIndex in pOptionalItemOrdering
        const int tmp = pOptionalItemOrdering[draggingTabTargetIndex];
        pOptionalItemOrdering[draggingTabTargetIndex] = pOptionalItemOrdering[draggingTabIndex];
        pOptionalItemOrdering[draggingTabIndex] = tmp;
        //fprintf(stderr,"%d %d\n",draggingTabIndex,draggingTabTargetIndex);
        draggingTabTargetIndex = draggingTabIndex = -1;
    }

    // Reset draggingTabIndex if necessary
    if (!isMouseDragging) {draggingTabIndex = -1;draggingLocked=false;}

    // Change selected tab when user closes the selected tab
    if (selectedIndex == justClosedTabIndex && selectedIndex>=0)    {
        selectedIndex = -1;
        for (int j = 0,i; j < numTabs; j++) {
            i = pOptionalItemOrdering ? pOptionalItemOrdering[j] : j;
            if (i==-1) continue;
            selectedIndex = i;
            break;
        }
    }

    // Restore the style
    style.ItemSpacing =                     itemSpacing;

    return selection_changed;
}


ImTextureID TabWindow::DockPanelIconTextureID = NULL;
const unsigned char* TabWindow::GetDockPanelIconImagePng(int* bufferSizeOut) {
    // I have drawn all the icons that compose this image myself.
    // I took inspiration from the icons bundled with: https://github.com/dockpanelsuite/dockpanelsuite (that is MIT licensed).
    // So no copyright issues for this AFAIK.
    static const unsigned char png[] =
{
 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,128,0,0,0,128,8,3,0,0,0,244,224,145,249,0,0,0,192,80,76,84,69,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,1,2,3,0,4,5,1,5,6,2,5,5,12,7,4,30,10,2,77,16,0,255,255,255,255,255,255,255,255,255,255,255,255,252,253,254,221,
 234,248,226,234,245,227,228,229,229,229,233,227,228,229,204,228,253,200,226,253,194,222,251,221,221,222,189,221,253,220,220,220,219,219,219,180,216,252,217,217,217,218,217,221,177,214,252,213,
 213,213,212,212,212,212,212,213,210,210,219,205,205,206,164,204,250,199,199,199,200,199,200,198,197,201,189,189,189,181,181,184,181,181,181,180,178,181,168,164,192,96,151,230,148,141,190,144,
 138,198,65,135,229,44,132,235,122,126,174,62,108,205,83,106,175,102,102,104,91,97,152,113,67,50,142,58,26,186,36,1,186,35,0,164,31,0,130,25,0,170,71,225,164,0,0,0,
 13,116,82,78,83,0,0,0,0,1,0,37,84,119,159,204,223,252,116,102,245,63,0,0,0,1,98,75,71,68,9,241,217,165,236,0,0,8,42,73,68,65,84,120,218,213,155,91,
 147,162,58,16,128,183,106,166,240,186,15,72,49,186,140,23,70,23,180,194,40,171,179,140,98,237,209,255,255,175,78,46,92,18,210,9,1,29,183,182,31,144,82,233,254,236,116,154,
 238,16,191,61,233,100,171,147,39,51,169,209,241,77,127,49,66,104,163,16,99,128,72,169,98,99,0,176,217,132,10,49,6,64,97,184,82,136,9,64,184,80,136,49,0,86,49,83,
 72,61,0,190,248,40,73,146,164,203,169,103,14,176,154,193,58,102,158,25,192,111,34,31,88,126,229,178,78,162,197,75,35,0,72,7,90,222,0,176,93,221,14,16,173,204,0,14,
 107,89,14,13,1,64,29,198,0,115,42,111,188,52,6,128,116,24,3,188,202,178,111,10,0,233,48,6,152,200,210,24,0,210,97,12,48,146,165,241,16,64,58,254,25,128,189,45,
 75,220,16,0,212,97,236,1,199,117,70,46,57,248,142,235,58,62,57,109,236,1,72,135,41,64,226,135,190,27,250,1,57,248,190,139,95,3,183,41,0,168,195,120,8,190,203,210,
 120,8,32,29,255,30,128,29,219,55,3,240,58,154,2,216,135,180,184,186,45,128,160,163,33,128,29,167,105,113,117,75,0,81,71,51,0,59,166,117,196,187,125,3,64,69,71,35,
 0,27,37,76,252,246,0,85,29,6,0,219,18,192,15,152,184,37,192,182,41,64,69,71,61,0,46,202,53,211,208,155,110,191,120,26,210,166,224,231,84,5,240,226,153,21,198,173,
 1,200,239,199,14,240,148,0,134,4,109,1,168,255,177,253,151,244,32,203,145,2,152,17,16,0,80,135,30,128,141,63,182,255,178,136,128,158,14,151,229,166,4,4,96,137,34,89,
 178,178,92,213,54,82,0,143,26,1,122,42,250,9,35,168,107,93,9,192,108,9,232,200,26,19,117,255,25,230,86,116,226,45,194,154,214,149,0,120,10,97,0,138,254,243,167,137,
 125,66,240,83,223,186,238,116,194,0,224,254,115,202,236,227,236,157,86,219,58,220,216,149,116,222,244,150,214,21,3,176,254,51,11,205,68,136,83,98,133,117,85,149,182,138,54,135,
 248,51,238,203,249,149,9,86,198,183,174,187,173,94,190,101,237,223,250,237,117,130,75,213,192,230,102,234,129,88,81,0,208,121,120,224,190,236,59,246,104,52,121,157,175,127,255,62,
 242,173,235,110,163,17,20,85,1,144,0,64,172,36,147,81,224,227,90,210,199,135,17,57,224,83,199,63,72,0,200,199,0,99,6,192,223,170,118,43,181,132,33,210,123,128,2,172,
 215,113,188,142,217,97,77,15,248,69,239,1,1,96,166,22,60,131,42,0,142,43,3,196,235,55,73,214,178,7,220,145,2,192,155,45,85,177,50,91,109,84,67,96,219,165,7,222,
 0,41,1,236,236,138,208,87,1,44,145,202,83,37,192,199,135,56,4,118,236,232,0,230,220,16,184,168,162,248,227,163,2,176,138,84,177,82,0,124,86,0,236,125,194,1,204,101,
 225,134,32,72,88,145,23,184,102,0,160,7,42,0,206,254,88,2,28,70,243,249,235,235,92,236,237,5,128,35,37,240,13,1,248,88,145,1,104,16,58,73,122,228,61,0,44,46,
 136,0,233,129,92,53,210,3,64,177,82,198,0,206,45,69,16,226,218,93,4,248,33,203,155,8,144,198,133,226,183,245,175,95,32,0,20,43,0,0,25,2,87,244,64,32,47,110,
 140,185,32,36,0,100,12,178,33,80,2,64,177,162,0,192,49,120,76,220,2,0,77,198,85,25,33,101,12,104,0,228,88,81,1,224,89,200,1,140,129,213,141,137,114,22,232,0,
 164,88,145,1,242,76,104,35,110,8,0,128,184,4,112,125,155,143,110,45,64,53,86,224,32,172,166,98,4,0,132,114,42,14,252,122,128,106,172,40,135,64,0,112,168,73,52,14,
 198,147,96,52,249,49,14,70,227,31,72,6,104,24,3,34,64,82,12,129,47,3,208,33,136,125,244,142,80,28,210,3,138,3,249,110,232,58,133,226,196,96,22,40,0,32,15,176,
 24,112,19,92,188,70,239,40,69,239,113,28,191,203,0,156,98,16,0,138,149,18,0,223,226,231,74,15,144,49,64,110,204,222,75,184,143,96,15,172,215,137,46,21,7,80,16,166,
 9,187,75,199,113,18,115,66,127,102,18,142,162,216,143,237,61,0,192,127,155,92,205,245,77,234,155,17,52,4,203,8,46,24,105,77,152,211,176,151,148,189,28,105,81,186,80,95,
 167,190,25,113,177,82,0,76,23,112,205,70,170,98,47,204,173,68,40,42,143,161,178,105,98,215,169,135,192,5,0,60,125,231,145,171,165,156,217,177,174,105,169,247,0,142,21,6,
 160,16,163,182,136,88,170,233,13,9,128,42,86,40,128,170,167,157,154,17,212,118,64,164,38,140,64,65,75,29,192,211,214,168,53,172,239,192,72,85,12,71,202,114,166,5,40,8,
 142,192,226,66,146,125,102,208,1,238,60,181,232,1,114,130,3,184,188,178,48,180,95,223,27,62,213,17,28,148,107,68,198,29,176,182,59,126,170,35,80,2,220,197,126,221,74,41,
 38,160,0,182,227,50,113,26,175,148,214,45,80,232,1,112,146,58,8,203,172,97,243,165,90,93,8,214,175,21,255,156,178,33,192,133,42,145,125,139,197,106,79,177,68,101,6,16,
 46,178,24,176,247,105,154,238,157,22,203,245,158,98,145,174,33,0,105,152,146,86,15,44,112,46,6,115,73,83,0,220,50,185,223,91,2,128,51,169,49,64,219,135,86,255,62,192,
 173,143,110,115,128,106,46,49,5,72,2,228,187,40,8,3,124,8,200,193,15,252,67,43,128,74,46,49,246,128,208,19,209,54,197,105,55,4,149,92,210,14,32,235,13,219,197,128,
 152,75,254,2,128,152,75,140,1,110,221,194,193,207,2,62,151,24,3,0,107,68,135,135,78,67,96,157,240,177,0,183,110,100,34,0,80,46,49,5,184,121,43,23,105,12,160,92,
 242,72,0,48,151,24,3,48,249,252,252,44,31,217,164,55,1,100,83,217,20,96,25,213,244,191,95,13,48,93,212,244,191,38,0,80,46,49,5,240,90,119,101,28,0,148,75,140,
 0,238,177,173,151,0,64,185,196,4,224,30,66,1,160,92,242,72,0,112,42,63,14,96,137,254,50,192,108,9,230,146,135,1,232,158,26,62,4,64,247,220,244,33,0,186,39,199,
 15,1,184,101,123,255,3,228,126,0,207,86,183,135,165,219,121,54,119,192,118,187,187,23,128,213,237,15,134,87,44,195,65,191,103,85,54,209,104,22,202,238,5,208,237,15,47,127,
 50,185,12,7,221,231,250,77,52,108,31,205,125,0,172,94,105,158,33,244,173,218,109,68,108,39,209,93,0,172,254,240,79,69,174,5,65,246,255,6,112,143,74,180,106,0,16,239,
 113,250,60,49,57,159,207,124,244,149,246,255,147,9,178,191,23,192,123,84,154,0,44,208,110,87,16,156,78,220,39,189,204,62,142,191,193,96,152,15,198,181,255,12,0,160,246,0,
 241,134,7,224,60,208,25,48,131,131,126,199,178,172,78,175,159,17,12,187,173,60,16,47,227,252,108,17,87,87,28,11,128,21,247,118,255,202,126,112,135,141,71,238,143,63,23,54,
 8,2,64,101,143,10,4,176,71,40,59,219,160,125,21,46,7,224,223,100,14,200,135,188,180,159,187,0,30,2,182,71,5,2,72,232,250,41,91,97,77,76,0,122,212,1,3,241,
 247,103,78,41,0,20,123,84,64,128,252,209,138,33,192,51,29,242,97,79,176,63,164,111,254,55,176,50,0,213,30,21,16,128,23,3,0,139,142,0,53,197,217,103,39,195,14,12,
 80,236,81,225,0,246,185,156,56,251,39,3,128,206,144,198,219,179,96,223,234,12,139,32,16,0,42,123,84,10,128,83,150,25,177,156,120,169,204,130,184,156,134,34,192,181,39,218,
 207,252,82,0,168,246,168,100,0,103,242,107,11,15,168,0,98,196,3,156,37,0,222,190,26,64,220,163,146,123,224,204,187,93,5,176,20,0,78,34,64,95,180,175,7,40,247,168,
 148,49,112,174,7,80,220,11,168,169,203,192,18,236,63,117,196,32,84,237,81,129,102,129,38,6,104,154,218,87,230,70,54,13,251,130,253,60,57,88,85,128,202,30,149,123,0,48,
 91,87,209,62,203,142,108,110,72,65,88,147,138,207,185,93,241,142,163,6,232,112,185,55,179,111,177,219,3,151,138,21,59,52,180,0,43,67,128,103,102,77,182,127,25,20,55,35,
 213,30,21,24,224,44,157,113,0,145,4,240,212,29,10,246,173,238,128,17,101,233,89,0,184,165,32,81,2,100,17,135,227,160,143,171,114,92,27,95,132,146,136,2,40,246,168,52,
 175,9,65,0,43,31,132,11,41,203,179,114,228,146,221,31,217,230,122,197,30,149,230,85,49,8,80,18,112,117,113,110,95,251,175,95,44,141,1,18,0,64,46,139,175,133,125,237,
 255,158,177,52,45,203,49,192,25,108,12,6,156,19,174,67,174,55,210,253,243,27,75,83,128,19,228,0,154,14,122,164,51,187,92,46,164,55,235,124,97,111,8,58,128,121,1,23,
 196,88,122,93,235,75,187,227,179,190,67,126,110,220,86,253,15,153,69,4,221,29,74,239,182,0,0,0,0,73,69,78,68,174,66,96,130
};

    if (bufferSizeOut) *bufferSizeOut = (int) (sizeof(png)/sizeof(png[0]));
    return png;
}


} // namespace ImGui
