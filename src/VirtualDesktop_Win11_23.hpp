#pragma once
#include "VirtualDesktopGlobals.hpp"
#include <HString.h>

namespace Win11_23
{
    const CLSID CLSID_ImmersiveShell = { 0xC2F03A33, 0x21F5, 0x47FA, 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39 };
    const CLSID CLSID_IApplicationViewCollection = { 0x1841C6D7, 0x4F9D, 0x42C0, 0xAF, 0x41, 0x87, 0x47, 0x53, 0x8F, 0x10, 0xE5 };
    const CLSID CLSID_IVirtualDesktopPinnedApps = { 0xB5A399E7, 0x1C87, 0x46B8, 0x88, 0xE9, 0xFC, 0x57, 0x47, 0xB1, 0x71, 0xBD };
    const CLSID CLSID_IVirtualDesktopManagerInternal = { 0xC5E0CDCA, 0x7B6E, 0x41B2, 0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B };

    MIDL_INTERFACE("3F07F4BE-B107-441A-AF0F-39D82529072C")
        IVirtualDesktopInterface : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsViewVisible(IApplicationView*, BOOL* result);
        virtual GUID STDMETHODCALLTYPE GetID();
        virtual HRESULT STDMETHODCALLTYPE GetName(HSTRING*);
    };


    MIDL_INTERFACE("1841C6D7-4F9D-42C0-AF41-8747538F10E5")
        IApplicationViewCollection : public IUnknown
    {
    public:
        virtual void uselessFunction1();
        virtual void uselessFunction2();
        virtual void uselessFunction3();
        virtual HRESULT STDMETHODCALLTYPE GetViewForHwnd(HWND hwnd, IApplicationView** pView);
    };

    MIDL_INTERFACE("4CE81583-1E4C-4632-A621-07A53543148F")
        IVirtualDesktopPinnedApps  : public IUnknown
    {
    public:
        virtual void uselessfunction1();
        virtual void uselessfunction2();
        virtual void uselessfunction3();
        virtual HRESULT STDMETHODCALLTYPE IsViewPinned(IApplicationView*, BOOL*);
        virtual HRESULT STDMETHODCALLTYPE PinView(IApplicationView* view);
        virtual HRESULT STDMETHODCALLTYPE UnpinView(IApplicationView* view);
    };

    MIDL_INTERFACE("A3175F2D-239C-4BD2-8AA0-EEBA8B0B138E")
        IVirtualDesktopManagerInternal  : public IUnknown
    {
    public:
        virtual void uselessfunction1();
        virtual void STDMETHODCALLTYPE MoveViewToDesktop(IApplicationView*, IVirtualDesktop*);
        virtual void uselessfunction3();
        virtual void STDMETHODCALLTYPE GetCurrentDesktop(IVirtualDesktop**);
        virtual void uselessfunction5();
        virtual int STDMETHODCALLTYPE GetAdjacentDesktop(IVirtualDesktop*, int, IVirtualDesktop**);
        virtual void STDMETHODCALLTYPE SwitchDesktop(IVirtualDesktop*);
        virtual void uselessfunction8();
        virtual void uselessfunction9();
        virtual void uselessfunction10();
        virtual void uselessfunction11();
        virtual void uselessfunction12();
        virtual void uselessfunction13();
        virtual void uselessfunction14();
        virtual void uselessfunction15();
        virtual void uselessfunction16();
        virtual void uselessfunction17();
        virtual void uselessfunction18();
        virtual void STDMETHODCALLTYPE SwitchDesktopWithAnimation(IVirtualDesktop*);
        virtual void STDMETHODCALLTYPE GetLastActiveDesktop(IVirtualDesktop**);
        virtual void uselessfunction21();
    };

}
