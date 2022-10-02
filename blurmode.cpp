#include "blurmode.h"

#include <QDebug>

BlurMode::BlurMode() {}

BlurMode::~BlurMode() {}

RTL_OSVERSIONINFOW BlurMode::GetWindowsVersion() {
  HMODULE hmodule = ::GetModuleHandleW(L"ntdll.dll");
  if (hmodule) {
    RtlGetVersionPtr rtl_get_version_ptr =
        (RtlGetVersionPtr)::GetProcAddress(hmodule, "RtlGetVersion");
    if (rtl_get_version_ptr != nullptr) {
      RTL_OSVERSIONINFOW rovi = {0};
      rovi.dwOSVersionInfoSize = sizeof(rovi);
      if (STATUS_SUCCESS == rtl_get_version_ptr(&rovi)) {
        return rovi;
      }
    }
  }
  RTL_OSVERSIONINFOW rovi = {0};
  return rovi;
}

HWND BlurMode::GetParentWindow(QMainWindow *window) {
  return reinterpret_cast<HWND>(window->winId());
}

void BlurMode::HandleMethodCall(QMainWindow *window, int effect, BlurModeEdit mode, bool dark) {
  if (mode == BlurModeEdit::kInitialize) {
    if (!is_initialized_) {
      user32 = ::GetModuleHandleA("user32.dll");
      if (user32) {
        set_window_composition_attribute_ =
            reinterpret_cast<SetWindowCompositionAttribute>(
                ::GetProcAddress(user32, "SetWindowCompositionAttribute"));
        if (set_window_composition_attribute_) {
          is_initialized_ = true;
          //result->Success();
          qDebug() << "Success";
        } else
          //result->Error("-2", "FAIL_LOAD_METHOD");
          qDebug() << "FAIL_LOAD_METHOD";
      } else
        //result->Error("-1", "FAIL_LOAD_DLL");
        qDebug() << "FAIL_LOAD_DLL";
    } else
      //result->Success();
      qDebug() << "Success";
  } else if (mode == kSetEffect) {
    // Set [ACCENT_DISABLED] as [ACCENT_POLICY] in
    // [SetWindowCompositionAttribute] to apply styles properly.
    ACCENT_POLICY accent = {ACCENT_DISABLED, 2, static_cast<DWORD>(0), 0};
    WINDOWCOMPOSITIONATTRIBDATA data;
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &accent;
    data.cbData = sizeof(accent);
    set_window_composition_attribute_(BlurMode::GetParentWindow(window), &data);
    // Only on later Windows 11 versions and if effect is WindowEffect.mica,
    // WindowEffect.acrylic or WindowEffect.tabbed, otherwise fallback to old
    // approach.
    if (GetWindowsVersion().dwBuildNumber >= 22523 && effect > 3) {
      BOOL enable = TRUE, dark_bool = dark;
      MARGINS margins = {-1};
      ::DwmExtendFrameIntoClientArea(BlurMode::GetParentWindow(window), &margins);
      ::DwmSetWindowAttribute(BlurMode::GetParentWindow(window), 20, &dark_bool,
                              sizeof(dark_bool));
      INT effect_value = effect == 4 ? 3 : effect == 5 ? 2 : 4;
      ::DwmSetWindowAttribute(BlurMode::GetParentWindow(window), 38, &effect_value,
                              sizeof(enable));
    } else {
      if (effect == 5) {
        // Check for Windows 11.
        if (GetWindowsVersion().dwBuildNumber >= 22000) {
          BOOL enable = TRUE, dark_bool = dark;
          MARGINS margins = {-1};
          // Mica effect requires [DwmExtendFrameIntoClientArea & "sheet of
          // glass"
          // effect with negative margins.
          ::DwmExtendFrameIntoClientArea(BlurMode::GetParentWindow(window), &margins);
          ::DwmSetWindowAttribute(BlurMode::GetParentWindow(window), 20, &dark_bool,
                                  sizeof(dark_bool));
          ::DwmSetWindowAttribute(BlurMode::GetParentWindow(window), 1029, &enable,
                                  sizeof(enable));
        }
      } else {
        // Restore original window style & [DwmExtendFrameIntoClientArea] margin
        // if the last set effect was [WindowEffect.mica], since it sets
        // negative margins to the window.
        if ((GetWindowsVersion().dwBuildNumber >= 22000 &&
                window_effect_last_ == 5) ||
            ((GetWindowsVersion().dwBuildNumber >= 22523 &&
             window_effect_last_ > 3))) {
          BOOL enable = FALSE;
          // Atleast one margin should be non-negative in order to show the DWM
          // window shadow created by handling [WM_NCCALCSIZE].
          //
          // Matching value with bitsdojo_window.
          // https://github.com/bitsdojo/bitsdojo_window/blob/adad0cd40be3d3e12df11d864f18a96a2d0fb4fb/bitsdojo_window_windows/windows/bitsdojo_window.cpp#L149
          MARGINS margins = {0, 0, 1, 0};
          ::DwmExtendFrameIntoClientArea(BlurMode::GetParentWindow(window), &margins);
          ::DwmSetWindowAttribute(BlurMode::GetParentWindow(window), 20, &enable,
                                  sizeof(enable));
          ::DwmSetWindowAttribute(BlurMode::GetParentWindow(window), 1029, &enable,
                                  sizeof(enable));
        }
        accent = {
            static_cast<ACCENT_STATE>(effect), 2,
            static_cast<DWORD>(
                (this->color.alpha() << 24) +
                (this->color.blue() << 16) +
                (this->color.green() << 8) +
                (this->color.red())),
            0};
        data.Attrib = WCA_ACCENT_POLICY;
        data.pvData = &accent;
        data.cbData = sizeof(accent);
        set_window_composition_attribute_(BlurMode::GetParentWindow(window), &data);
      }
    }
    window_effect_last_ = effect;
    //result->Success();
  } else if (mode == BlurModeEdit::kHideWindowControls) {
    ::SetWindowLongW(BlurMode::GetParentWindow(window), GWL_STYLE,
                    ::GetWindowLongW(BlurMode::GetParentWindow(window), GWL_STYLE) &
                        (0xFFFFFFFF ^ WS_SYSMENU));
    //result->Success();
  } else if (mode == BlurModeEdit::kShowWindowControls) {
    ::SetWindowLongW(BlurMode::GetParentWindow(window), GWL_STYLE,
                    ::GetWindowLongW(BlurMode::GetParentWindow(window), GWL_STYLE) | WS_SYSMENU);
    //result->Success();
  } /*else if (mode == BlurModeEdit::kEnterFullscreen) {
    if (!is_fullscreen_) {
      is_fullscreen_ = true;
      HWND window = BlurMode::GetParentWindow(window);
      HMONITOR monitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
      MONITORINFO info;
      info.cbSize = sizeof(MONITORINFO);
      GetMonitorInfo(monitor, &info);
      SetWindowLongPtr(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
      ::GetWindowRect(window, &last_rect_);
      ::SetWindowPos(
          window, HWND_TOPMOST, info.rcMonitor.left, info.rcMonitor.top,
          info.rcMonitor.right - info.rcMonitor.left,
          info.rcMonitor.bottom - info.rcMonitor.top, SWP_SHOWWINDOW);
      ::ShowWindow(window, SW_MAXIMIZE);
    }
    //result->Success();
  } else if (mode == BlurModeEdit::kExitFullscreen) {
    if (is_fullscreen_) {
      is_fullscreen_ = false;
      HWND window = BlurMode::GetParentWindow(window);
      ::SetWindowLongPtr(window, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
      ::SetWindowPos(window, HWND_NOTOPMOST, last_rect_.left, last_rect_.top,
                     last_rect_.right - last_rect_.left,
                     last_rect_.bottom - last_rect_.top, SWP_SHOWWINDOW);
      ::ShowWindow(window, SW_RESTORE);
    }*/
    //result->Success();
  qDebug() << "Success";
}

void BlurMode::setColorBackground(QColor color)
{
    this->color = color;
}

