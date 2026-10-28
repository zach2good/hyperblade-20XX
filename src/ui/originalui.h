// HYPERX.EXE's own menu bar and dialogs, recreated in ImGui from the templates in the player's copy,
// and drawn in the Windows 95 look the game shipped with. The menu bar sits at the top of the
// screen while the debug menu is open or the pointer is there. Its commands do what the original's
// MainWindow::OnCommand did. The dialogs draw at their original layout, and the ones the original
// wired to the game read and write the same state.

#ifndef HYPERBLADE_ORIGINALUI_H
#define HYPERBLADE_ORIGINALUI_H

namespace OriginalUI
{

// The game's menus, then a list that opens any of its dialogs, into the open menu bar. Draw it
// inside a Theme.
void DrawMenus();

// A menu in the Windows 95 look, on the bar or inside another menu. Close an open one with
// ImGui::EndMenu.
bool BeginMenu(const char* pszLabel, bool bOnBar);

// A menu item in the Windows 95 look. True when chosen.
bool MenuItem(const char* pszLabel, const char* pszShortcut, bool bChecked, bool bEnabled);

// A raised push button, sized to its label when cx and cy are nought. A default button has the
// extra black frame. True when clicked.
bool Button(const char* pszLabel, float cx = 0.0f, float cy = 0.0f, bool bDefault = false);

// A check box with a sunken box. True when clicked.
bool Checkbox(const char* pszLabel, bool* pbChecked);

// A radio button with a sunken ring. True when clicked.
bool RadioButton(const char* pszLabel, bool bActive);

// A heading over a run of controls, on an etched line.
void SeparatorText(const char* pszLabel);

// An etched line across the window or menu.
void Separator();

// Tabs on a raised panel, as a Windows 95 property sheet. A tab's contents go between
// BeginTabItem and EndTabItem, and EndTabBar draws the tabs and the panel. bSelect picks a tab.
bool BeginTabBar(const char* pszID);
bool BeginTabItem(const char* pszLabel, bool bSelect);
void EndTabItem();
void EndTabBar();

// A row of a list: the selected row white on navy, and no highlight under the pointer alone.
bool Selectable(const char* pszLabel, bool bSelected, int nFlags = 0);

// The last item's tooltip, black on pale yellow in a black frame, as printf formats it.
void SetItemTooltip(const char* pszFormat, ...);

void DrawDialogs();
bool HasOpenDialogs();

// The Windows 95 look, pushed for as long as it lives.
class Theme
{
public:
    Theme();
    ~Theme();

    Theme(const Theme&)            = delete;
    Theme& operator=(const Theme&) = delete;

private:
    int m_nVars    = 0;
    int m_nColours = 0;
};

// The current window's raised edge and Windows 98 caption. False once its close box is pressed.
bool DrawWindowFrame(const char* pszTitle);

constexpr unsigned short kDialogAnimationPeek = 462;

// Opens one of the game's dialogs by resource id, as the original's code did.
void OpenDialog(unsigned short nDialog);

} // namespace OriginalUI

#endif
