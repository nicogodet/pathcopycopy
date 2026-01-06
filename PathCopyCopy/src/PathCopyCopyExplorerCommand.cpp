// PathCopyCopyExplorerCommand.cpp
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

#include <stdafx.h>
#include <AllPluginsProvider.h>
#include <PathCopyCopyExplorerCommand.h>
#include <DefaultPlugin.h>
#include <dllmain.h>
#include <PathCopyCopyPluginsRegistry.h>
#include <PathCopyCopySettings.h>
#include <PluginUtils.h>
#include <PathAction.h>
#include <StGdiplusStartup.h>

#include <algorithm>
#include <functional>

#include <gdiplus.h>
#include <shlwapi.h>

#pragma warning(disable: 26426) // Globals are only used by COM objects created later, so we're OK here
#pragma warning(disable: 26490) // Need reinterpret_cast to use some weird Win32 APIs

namespace {

const int32_t DEFAULT_ICON_SIZE = 16;  // Default width & height for loaded icons.

} // anonymous namespace

// CPathCopyCopyExplorerCommand

//
// Constructor.
//
CPathCopyCopyExplorerCommand::CPathCopyCopyExplorerCommand() noexcept(false)
    : m_spSettings(),
      m_vspPluginsInDefaultOrder(),
      m_spPluginProvider(),
      m_vFiles(),
      m_spSite(),
      m_spPCCIcon(),
      m_EnumIndex(0),
      m_vspSubmenuPlugins(),
      m_spQuickPlugin(),
      m_IsSubCommand(false),
      m_spCommandPlugin()
{
}

//
// Destructor.
//
CPathCopyCopyExplorerCommand::~CPathCopyCopyExplorerCommand()
{
}

//
// Returns a reference to the Settings object, creating it if needed.
//
// @return Reference to Settings object.
//
PCC::Settings& CPathCopyCopyExplorerCommand::GetSettings()
{
    if (m_spSettings == nullptr) {
        m_spSettings = std::make_shared<PCC::Settings>();
    }
    return *m_spSettings;
}

//
// Initializes the plugin provider and loads all plugins.
//
void CPathCopyCopyExplorerCommand::InitializePlugins()
{
    if (m_spPluginProvider == nullptr) {
        // Create plugin provider and load all plugins.
        m_spPluginProvider = std::make_shared<PCC::AllPluginsProvider>(
            std::static_pointer_cast<PCC::COMPluginProvider>(m_spSettings),
            std::static_pointer_cast<PCC::PipelinePluginProvider>(m_spSettings),
            true);
        m_vspPluginsInDefaultOrder = m_spPluginProvider->GetPlugins();
    }
}

//
// Extracts files from a shell item array.
//
// @param p_psiItemArray Shell item array containing selected files.
//
void CPathCopyCopyExplorerCommand::ExtractFilesFromItemArray(
    IShellItemArray* p_psiItemArray)
{
    m_vFiles.clear();

    if (p_psiItemArray != nullptr) {
        DWORD itemCount = 0;
        if (SUCCEEDED(p_psiItemArray->GetCount(&itemCount))) {
            for (DWORD i = 0; i < itemCount; ++i) {
                ATL::CComPtr<IShellItem> spItem;
                if (SUCCEEDED(p_psiItemArray->GetItemAt(i, &spItem))) {
                    PWSTR pszName = nullptr;
                    if (SUCCEEDED(spItem->GetDisplayName(SIGDN_FILESYSPATH, &pszName))) {
                        m_vFiles.emplace_back(pszName);
                        ::CoTaskMemFree(pszName);
                    }
                }
            }
        }
    }
}

//
// Returns the parent path of the selected files.
//
// @return Parent path, or empty string if none.
//
std::wstring CPathCopyCopyExplorerCommand::GetParentPath() const
{
    std::wstring parentPath;
    if (!m_vFiles.empty()) {
        parentPath = m_vFiles.front();
        const size_t lastSlash = parentPath.find_last_of(L"\\");
        if (lastSlash != std::wstring::npos) {
            parentPath = parentPath.substr(0, lastSlash);
        }
    }
    return parentPath;
}

//
// Performs the plugin action on the selected files.
//
// @param p_spPlugin Plugin to use for action.
// @param p_hWnd Parent window handle.
// @return S_OK if successful, otherwise an error code.
//
HRESULT CPathCopyCopyExplorerCommand::ActOnFiles(
    const PCC::PluginSP& p_spPlugin,
    HWND p_hWnd)
{
    HRESULT hRes = S_OK;

    try {
        // Get settings for plugin behavior.
        PCC::Settings& settings = GetSettings();
        
        // Perform the action using the plugin.
        PCC::PathActionSP spAction = std::make_shared<PCC::PathAction>(
            p_spPlugin,
            settings.GetUseHiddenShares(),
            settings.GetUseFQDN(),
            settings.GetAddQuotesAroundPaths(),
            settings.GetAreQuotesOptional(),
            settings.GetMakePathsIntoEmailLinks(),
            settings.GetEncodeParam(),
            settings.GetAppendSeparatorForDirectories(),
            settings.GetTrueLnkPaths(),
            settings.GetCopyPathsRecursively(),
            settings.GetPathsSeparator());

        std::wstring paths;
        spAction->Act(m_vFiles, GetParentPath(), paths);

        // Copy to clipboard.
        if (!paths.empty()) {
            if (::OpenClipboard(p_hWnd)) {
                ::EmptyClipboard();
                
                const SIZE_T size = (paths.size() + 1) * sizeof(wchar_t);
                HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, size);
                if (hGlobal != nullptr) {
                    void* pData = ::GlobalLock(hGlobal);
                    if (pData != nullptr) {
                        ::memcpy(pData, paths.c_str(), size);
                        ::GlobalUnlock(hGlobal);
                        ::SetClipboardData(CF_UNICODETEXT, hGlobal);
                    } else {
                        ::GlobalFree(hGlobal);
                        hRes = E_FAIL;
                    }
                } else {
                    hRes = E_OUTOFMEMORY;
                }
                ::CloseClipboard();
            } else {
                hRes = E_FAIL;
            }
        }
    } catch (...) {
        hRes = E_FAIL;
    }

    return hRes;
}

//
// Gets the PCC icon as an HBITMAP.
//
// @return Handle to PCC icon bitmap.
//
HBITMAP CPathCopyCopyExplorerCommand::GetPCCIcon()
{
    if (m_spPCCIcon == nullptr) {
        try {
            StGdiplusStartup stGdiplus;
            
            // Load bitmap from resources.
            HRSRC hResource = ::FindResourceW(ATL::_AtlBaseModule.GetModuleInstance(),
                MAKEINTRESOURCEW(IDB_PCCICON2), RT_BITMAP);
            if (hResource != nullptr) {
                HGLOBAL hGlobal = ::LoadResource(ATL::_AtlBaseModule.GetModuleInstance(), hResource);
                if (hGlobal != nullptr) {
                    const void* pData = ::LockResource(hGlobal);
                    const DWORD dataSize = ::SizeofResource(ATL::_AtlBaseModule.GetModuleInstance(), hResource);
                    
                    if (pData != nullptr && dataSize > 0) {
                        IStream* pStream = nullptr;
                        if (SUCCEEDED(::CreateStreamOnHGlobal(nullptr, TRUE, &pStream))) {
                            ULONG written = 0;
                            if (SUCCEEDED(pStream->Write(pData, dataSize, &written))) {
                                Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
                                if (pBitmap != nullptr) {
                                    HBITMAP hBitmap = nullptr;
                                    if (pBitmap->GetHBITMAP(Gdiplus::Color(255, 255, 255), &hBitmap) == Gdiplus::Ok) {
                                        m_spPCCIcon = std::make_shared<StImage>(hBitmap);
                                    }
                                    delete pBitmap;
                                }
                            }
                            pStream->Release();
                        }
                    }
                }
            }
        } catch (...) {
            // Ignore errors
        }
    }
    
    return m_spPCCIcon != nullptr ? m_spPCCIcon->Get() : nullptr;
}

//
// IExplorerCommand::GetTitle
//
// Returns the title of the command.
//
// @param p_psiItemArray Shell item array (unused).
// @param p_ppszName Receives the command title.
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::GetTitle(
    IShellItemArray* /*p_psiItemArray*/,
    LPWSTR* p_ppszName)
{
    if (p_ppszName == nullptr) {
        return E_POINTER;
    }

    HRESULT hRes = S_OK;
    try {
        std::wstring title;
        
        if (m_IsSubCommand && m_spCommandPlugin != nullptr) {
            // This is a subcommand - use plugin description.
            title = m_spCommandPlugin->Description();
        } else {
            // This is the main command - use "Path Copy Copy".
            title = L"Path Copy Copy";
        }

        *p_ppszName = static_cast<LPWSTR>(::CoTaskMemAlloc((title.size() + 1) * sizeof(wchar_t)));
        if (*p_ppszName != nullptr) {
            ::wcscpy_s(*p_ppszName, title.size() + 1, title.c_str());
        } else {
            hRes = E_OUTOFMEMORY;
        }
    } catch (...) {
        hRes = E_FAIL;
    }

    return hRes;
}

//
// IExplorerCommand::GetIcon
//
// Returns the icon for the command.
//
// @param p_psiItemArray Shell item array (unused).
// @param p_ppszIcon Receives the icon path.
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::GetIcon(
    IShellItemArray* /*p_psiItemArray*/,
    LPWSTR* p_ppszIcon)
{
    if (p_ppszIcon == nullptr) {
        return E_POINTER;
    }

    // Return empty string - Windows will use default icon.
    *p_ppszIcon = static_cast<LPWSTR>(::CoTaskMemAlloc(sizeof(wchar_t)));
    if (*p_ppszIcon != nullptr) {
        (*p_ppszIcon)[0] = L'\0';
        return S_OK;
    }
    
    return E_OUTOFMEMORY;
}

//
// IExplorerCommand::GetToolTip
//
// Returns the tooltip for the command.
//
// @param p_psiItemArray Shell item array (unused).
// @param p_ppszInfotip Receives the tooltip.
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::GetToolTip(
    IShellItemArray* /*p_psiItemArray*/,
    LPWSTR* p_ppszInfotip)
{
    if (p_ppszInfotip == nullptr) {
        return E_POINTER;
    }

    // Return empty string - no tooltip needed.
    *p_ppszInfotip = static_cast<LPWSTR>(::CoTaskMemAlloc(sizeof(wchar_t)));
    if (*p_ppszInfotip != nullptr) {
        (*p_ppszInfotip)[0] = L'\0';
        return S_OK;
    }
    
    return E_OUTOFMEMORY;
}

//
// IExplorerCommand::GetCanonicalName
//
// Returns the canonical name (GUID) of the command.
//
// @param p_pguidCommandName Receives the command GUID.
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::GetCanonicalName(
    GUID* p_pguidCommandName)
{
    if (p_pguidCommandName == nullptr) {
        return E_POINTER;
    }

    *p_pguidCommandName = CLSID_PathCopyCopyExplorerCommand;
    return S_OK;
}

//
// IExplorerCommand::GetState
//
// Returns the state of the command.
//
// @param p_psiItemArray Shell item array.
// @param p_fOkToBeSlow Whether it's OK to take time to compute state.
// @param p_pCmdState Receives the command state.
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::GetState(
    IShellItemArray* p_psiItemArray,
    BOOL /*p_fOkToBeSlow*/,
    EXPCMDSTATE* p_pCmdState)
{
    if (p_pCmdState == nullptr) {
        return E_POINTER;
    }

    HRESULT hRes = S_OK;
    try {
        // Check if Windows 11 menu is enabled in settings.
        if (!GetSettings().GetWindows11MenuEnabled()) {
            *p_pCmdState = ECS_HIDDEN;
            return S_OK;
        }

        // Extract files from item array.
        ExtractFilesFromItemArray(p_psiItemArray);

        // Enable command if we have files.
        *p_pCmdState = m_vFiles.empty() ? ECS_DISABLED : ECS_ENABLED;
    } catch (...) {
        *p_pCmdState = ECS_DISABLED;
        hRes = E_FAIL;
    }

    return hRes;
}

//
// IExplorerCommand::Invoke
//
// Invokes the command.
//
// @param p_psiItemArray Shell item array.
// @param p_pbc Bind context (unused).
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::Invoke(
    IShellItemArray* p_psiItemArray,
    IBindCtx* /*p_pbc*/)
{
    HRESULT hRes = S_OK;

    try {
        // Extract files from item array.
        ExtractFilesFromItemArray(p_psiItemArray);

        if (!m_vFiles.empty()) {
            // Initialize plugins.
            InitializePlugins();

            // Determine which plugin to use.
            PCC::PluginSP spPlugin;
            
            if (m_IsSubCommand && m_spCommandPlugin != nullptr) {
                // This is a subcommand - use the specific plugin.
                spPlugin = m_spCommandPlugin;
            } else if (m_spQuickPlugin != nullptr) {
                // Use quick access plugin.
                spPlugin = m_spQuickPlugin;
            }

            // Perform action if we have a plugin.
            if (spPlugin != nullptr) {
                hRes = ActOnFiles(spPlugin, nullptr);
            }
        }
    } catch (...) {
        hRes = E_FAIL;
    }

    return hRes;
}

//
// IExplorerCommand::GetFlags
//
// Returns flags for the command.
//
// @param p_pFlags Receives the command flags.
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::GetFlags(
    EXPCMDFLAGS* p_pFlags)
{
    if (p_pFlags == nullptr) {
        return E_POINTER;
    }

    // Check if we should show a submenu.
    *p_pFlags = (m_IsSubCommand || m_vspSubmenuPlugins.empty()) ? ECF_DEFAULT : ECF_HASSUBCOMMANDS;
    
    return S_OK;
}

//
// IExplorerCommand::EnumSubCommands
//
// Enumerates subcommands.
//
// @param p_ppEnum Receives the enumerator.
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::EnumSubCommands(
    IEnumExplorerCommand** p_ppEnum)
{
    if (p_ppEnum == nullptr) {
        return E_POINTER;
    }

    HRESULT hRes = S_OK;

    try {
        // Initialize plugins if needed.
        InitializePlugins();

        // Get submenu plugins from settings.
        PCC::GUIDV vSubmenuPluginIds;
        if (GetSettings().GetWindows11SubmenuPlugins(vSubmenuPluginIds)) {
            // Filter plugins based on IDs.
            m_vspSubmenuPlugins.clear();
            for (const auto& pluginId : vSubmenuPluginIds) {
                auto it = std::find_if(m_vspPluginsInDefaultOrder.begin(), 
                                       m_vspPluginsInDefaultOrder.end(),
                                       [&pluginId](const PCC::PluginSP& spPlugin) {
                                           return spPlugin->Id() == pluginId;
                                       });
                if (it != m_vspPluginsInDefaultOrder.end()) {
                    m_vspSubmenuPlugins.push_back(*it);
                }
            }
        } else {
            // Use all plugins.
            m_vspSubmenuPlugins = m_vspPluginsInDefaultOrder;
        }

        // Return this object as the enumerator.
        m_EnumIndex = 0;
        hRes = QueryInterface(IID_IEnumExplorerCommand, reinterpret_cast<void**>(p_ppEnum));
    } catch (...) {
        hRes = E_FAIL;
    }

    return hRes;
}

//
// IExplorerCommandState::GetState
//
// Returns the state of the command (duplicate method for IExplorerCommandState).
//
// @param p_psiItemArray Shell item array.
// @param p_fOkToBeSlow Whether it's OK to take time to compute state.
// @param p_pCmdState Receives the command state.
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::GetState(
    IShellItemArray* p_psiItemArray,
    BOOL p_fOkToBeSlow,
    EXPCMDSTATE* p_pCmdState)
{
    // Forward to IExplorerCommand::GetState.
    return GetState(p_psiItemArray, p_fOkToBeSlow, p_pCmdState);
}

//
// IInitializeCommand::Initialize
//
// Initializes the command with its name.
//
// @param p_pszCommandName Command name.
// @param p_ppb Property bag (unused).
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::Initialize(
    PCWSTR /*p_pszCommandName*/,
    IPropertyBag* /*p_ppb*/)
{
    // Initialize settings.
    try {
        GetSettings();
        
        // Check for quick plugin.
        GUID quickPluginId;
        if (GetSettings().GetWindows11QuickPlugin(quickPluginId)) {
            InitializePlugins();
            
            // Find the quick plugin.
            auto it = std::find_if(m_vspPluginsInDefaultOrder.begin(),
                                   m_vspPluginsInDefaultOrder.end(),
                                   [&quickPluginId](const PCC::PluginSP& spPlugin) {
                                       return spPlugin->Id() == quickPluginId;
                                   });
            if (it != m_vspPluginsInDefaultOrder.end()) {
                m_spQuickPlugin = *it;
            }
        }
    } catch (...) {
        return E_FAIL;
    }

    return S_OK;
}

//
// IObjectWithSite::SetSite
//
// Sets the site pointer.
//
// @param p_pUnkSite Site pointer.
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::SetSite(
    IUnknown* p_pUnkSite)
{
    m_spSite = p_pUnkSite;
    return S_OK;
}

//
// IObjectWithSite::GetSite
//
// Gets the site pointer.
//
// @param p_riid Interface ID.
// @param p_ppvSite Receives the site pointer.
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::GetSite(
    REFIID p_riid,
    void** p_ppvSite)
{
    if (p_ppvSite == nullptr) {
        return E_POINTER;
    }

    if (m_spSite != nullptr) {
        return m_spSite->QueryInterface(p_riid, p_ppvSite);
    }

    return E_FAIL;
}

//
// IEnumExplorerCommand::Next
//
// Gets the next subcommand(s).
//
// @param p_celt Number of commands to retrieve.
// @param p_pUICommand Array to receive commands.
// @param p_pceltFetched Receives number of commands retrieved.
// @return S_OK if successful, S_FALSE if no more commands, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::Next(
    ULONG p_celt,
    IExplorerCommand** p_pUICommand,
    ULONG* p_pceltFetched)
{
    if (p_pUICommand == nullptr) {
        return E_POINTER;
    }

    ULONG fetched = 0;
    HRESULT hRes = S_OK;

    try {
        while (fetched < p_celt && m_EnumIndex < m_vspSubmenuPlugins.size()) {
            // Create a new instance for this subcommand.
            ATL::CComObject<CPathCopyCopyExplorerCommand>* pCommand = nullptr;
            hRes = ATL::CComObject<CPathCopyCopyExplorerCommand>::CreateInstance(&pCommand);
            if (SUCCEEDED(hRes)) {
                pCommand->AddRef();
                
                // Configure the subcommand.
                pCommand->m_IsSubCommand = true;
                pCommand->m_spCommandPlugin = m_vspSubmenuPlugins[m_EnumIndex];
                pCommand->m_spSettings = m_spSettings;
                pCommand->m_spPluginProvider = m_spPluginProvider;
                pCommand->m_vspPluginsInDefaultOrder = m_vspPluginsInDefaultOrder;
                pCommand->m_vFiles = m_vFiles;
                
                hRes = pCommand->QueryInterface(IID_IExplorerCommand,
                    reinterpret_cast<void**>(&p_pUICommand[fetched]));
                
                pCommand->Release();
                
                if (SUCCEEDED(hRes)) {
                    ++fetched;
                    ++m_EnumIndex;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
    } catch (...) {
        hRes = E_FAIL;
    }

    if (p_pceltFetched != nullptr) {
        *p_pceltFetched = fetched;
    }

    return (fetched == p_celt) ? S_OK : S_FALSE;
}

//
// IEnumExplorerCommand::Skip
//
// Skips the specified number of commands.
//
// @param p_celt Number of commands to skip.
// @return S_OK if successful, S_FALSE if end reached, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::Skip(
    ULONG p_celt)
{
    m_EnumIndex += p_celt;
    return (m_EnumIndex <= m_vspSubmenuPlugins.size()) ? S_OK : S_FALSE;
}

//
// IEnumExplorerCommand::Reset
//
// Resets the enumeration.
//
// @return S_OK if successful, otherwise an error code.
//
STDMETHODIMP CPathCopyCopyExplorerCommand::Reset()
{
    m_EnumIndex = 0;
    return S_OK;
}

//
// IEnumExplorerCommand::Clone
//
// Clones the enumerator.
//
// @param p_ppenum Receives the cloned enumerator.
// @return E_NOTIMPL (not implemented).
//
STDMETHODIMP CPathCopyCopyExplorerCommand::Clone(
    IEnumExplorerCommand** /*p_ppenum*/)
{
    // Not implemented.
    return E_NOTIMPL;
}
