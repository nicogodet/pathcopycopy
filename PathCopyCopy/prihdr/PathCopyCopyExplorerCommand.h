// PathCopyCopyExplorerCommand.h
// (c) 2008-2021, Charles Lechasseur
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <PathCopyCopy_i.h>
#include "PathCopyCopyPrivateTypes.h"
#include "Plugin.h"
#include "resource.h"
#include "StImage.h"

#include <memory>
#include <string>
#include <vector>

#include <atlbase.h>
#include <atlcom.h>
#include <shobjidl_core.h>
#include <windows.h>

// CLSID for PathCopyCopyExplorerCommand (defined in PathCopyCopy.idl)
// {E16C019E-8B4E-4F4D-9C3A-F5F7D8E9B2A1}
DEFINE_GUID(CLSID_PathCopyCopyExplorerCommand, 
    0xE16C019E, 0x8B4E, 0x4F4D, 0x9C, 0x3A, 0xF5, 0xF7, 0xD8, 0xE9, 0xB2, 0xA1);


//
// CPathCopyCopyExplorerCommand
//
// Windows 11 modern context menu extension that implements IExplorerCommand.
// This allows Path Copy Copy commands to appear in the modern context menu
// without requiring users to click "Show more options".
//
class ATL_NO_VTABLE CPathCopyCopyExplorerCommand :
    public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
    public ATL::CComCoClass<CPathCopyCopyExplorerCommand, &CLSID_PathCopyCopyExplorerCommand>,
    public IExplorerCommand,
    public IExplorerCommandState,
    public IInitializeCommand,
    public IObjectWithSite,
    public IEnumExplorerCommand
{
public:
    CPathCopyCopyExplorerCommand() noexcept(false);
    CPathCopyCopyExplorerCommand(const CPathCopyCopyExplorerCommand&) = delete;
    CPathCopyCopyExplorerCommand(CPathCopyCopyExplorerCommand&&) = delete;
    CPathCopyCopyExplorerCommand& operator=(const CPathCopyCopyExplorerCommand&) = delete;
    CPathCopyCopyExplorerCommand& operator=(CPathCopyCopyExplorerCommand&&) = delete;
    virtual ~CPathCopyCopyExplorerCommand();

#pragma warning(push)
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

    DECLARE_REGISTRY_RESOURCEID(IDR_PATHCOPYCOPYEXPLORERCOMMAND)

    DECLARE_NOT_AGGREGATABLE(CPathCopyCopyExplorerCommand)

    BEGIN_COM_MAP(CPathCopyCopyExplorerCommand)
        COM_INTERFACE_ENTRY(IExplorerCommand)
        COM_INTERFACE_ENTRY(IExplorerCommandState)
        COM_INTERFACE_ENTRY(IInitializeCommand)
        COM_INTERFACE_ENTRY(IObjectWithSite)
        COM_INTERFACE_ENTRY(IEnumExplorerCommand)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

#pragma warning(pop)

    [[gsl::suppress(c.128)]]
    HRESULT FinalConstruct() noexcept
    {
        return S_OK;
    }

    [[gsl::suppress(c.128)]]
    void FinalRelease() noexcept
    {
    }

public:
    // IExplorerCommand methods
    STDMETHOD(GetTitle)(IShellItemArray* p_psiItemArray, LPWSTR* p_ppszName);
    STDMETHOD(GetIcon)(IShellItemArray* p_psiItemArray, LPWSTR* p_ppszIcon);
    STDMETHOD(GetToolTip)(IShellItemArray* p_psiItemArray, LPWSTR* p_ppszInfotip);
    STDMETHOD(GetCanonicalName)(GUID* p_pguidCommandName);
    STDMETHOD(GetState)(IShellItemArray* p_psiItemArray, BOOL p_fOkToBeSlow, EXPCMDSTATE* p_pCmdState);
    STDMETHOD(Invoke)(IShellItemArray* p_psiItemArray, IBindCtx* p_pbc);
    STDMETHOD(GetFlags)(EXPCMDFLAGS* p_pFlags);
    STDMETHOD(EnumSubCommands)(IEnumExplorerCommand** p_ppEnum);

    // IExplorerCommandState methods - uses same GetState as IExplorerCommand

    // IInitializeCommand methods
    STDMETHOD(Initialize)(PCWSTR p_pszCommandName, IPropertyBag* p_ppb);

    // IObjectWithSite methods
    STDMETHOD(SetSite)(IUnknown* p_pUnkSite);
    STDMETHOD(GetSite)(REFIID p_riid, void** p_ppvSite);

    // IEnumExplorerCommand methods
    STDMETHOD(Next)(ULONG p_celt, IExplorerCommand** p_pUICommand, ULONG* p_pceltFetched);
    STDMETHOD(Skip)(ULONG p_celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumExplorerCommand** p_ppenum);

private:
    typedef std::shared_ptr<StImage>    StImageSP;  // Shared pointer to a Win32 image wrapper.

    PCC::SettingsSP         m_spSettings;           // Object to access program settings.
    PCC::PluginSPV          m_vspPluginsInDefaultOrder; // Vector of all plugins in default order.
    PCC::PluginProviderSP   m_spPluginProvider;     // Plugin provider object.

    PCC::FilesV             m_vFiles;               // Files selected in Shell.
    ATL::CComPtr<IUnknown>  m_spSite;               // Site pointer for IObjectWithSite.

    StImageSP               m_spPCCIcon;            // Holds the PCC icon.
    
    ULONG                   m_EnumIndex;            // Current enumeration index for IEnumExplorerCommand.
    PCC::PluginSPV          m_vspSubmenuPlugins;    // Plugins to show in submenu.

    PCC::PluginSP           m_spQuickPlugin;        // Optional quick access plugin.
    bool                    m_IsSubCommand;         // Whether this is a subcommand instance.
    PCC::PluginSP           m_spCommandPlugin;      // Plugin for this specific command (for subcommands).

    PCC::Settings&          GetSettings();
    void                    InitializePlugins();
    void                    ExtractFilesFromItemArray(IShellItemArray* p_psiItemArray);
    std::wstring            GetParentPath() const;
    HRESULT                 ActOnFiles(const PCC::PluginSP& p_spPlugin, HWND p_hWnd);
    HBITMAP                 GetPCCIcon();
};

#pragma warning(suppress: ALL_CPPCORECHECK_WARNINGS)
OBJECT_ENTRY_AUTO(__uuidof(PathCopyCopyExplorerCommand), CPathCopyCopyExplorerCommand)
