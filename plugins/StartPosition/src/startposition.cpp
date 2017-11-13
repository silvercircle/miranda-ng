#include "stdafx.h"


StartPositionPlugin::StartPositionPlugin() :
    PLUGIN<StartPositionPlugin>(MODULE_NAME)
{
    HookPluginEvent(ME_OPT_INITIALISE, &StartPositionPlugin::OnOptionsInit);
}

void StartPositionPlugin::positionClist()
{
    ClistOptions clOptions;

    if (spOptions.setClistStartState)
        clOptions.state = static_cast<BYTE>(spOptions.clistState);

    if (spOptions.setClistWidth && spOptions.clistWidth > 0)
        clOptions.width = static_cast<DWORD>(spOptions.clistWidth);
    else
        spOptions.clistWidth = static_cast<DWORD>(clOptions.width);

    if (spOptions.setTopPosition || spOptions.setBottomPosition || spOptions.setSidePosition)
        clOptions.isDocked = false;

    if (spOptions.setTopPosition)
        clOptions.y = static_cast<DWORD>(spOptions.pixelsFromTop);

    RECT WorkArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0);

    if (spOptions.setBottomPosition) {
        if (spOptions.setTopPosition)
            clOptions.height = WorkArea.bottom - WorkArea.top - spOptions.pixelsFromTop - spOptions.pixelsFromBottom;
        else
            clOptions.y = WorkArea.bottom - spOptions.pixelsFromBottom - clOptions.height;
    }

    if (spOptions.setSidePosition) {
        if (spOptions.clistAlign == ClistAlign::right)
            clOptions.x = WorkArea.right - spOptions.clistWidth - spOptions.pixelsFromSide;
        else
            clOptions.x = WorkArea.left + spOptions.pixelsFromSide;
    }
}

int StartPositionPlugin::OnOptionsInit(WPARAM wParam, LPARAM)
{
    OPTIONSDIALOGPAGE odp = {};
    odp.hInstance = g_hInst;
    odp.szGroup.a = LPGEN("Contact list");
    odp.szTitle.a = LPGEN("Start position");
    odp.pDialog = new COptionsDlg(this);
    odp.flags = ODPF_BOLDGROUPS;
    Options_AddPage(wParam, &odp);

    return 0;
}
