#pragma once

#include <string>

// 크로스플랫폼 클립보드 복사.
//   Windows : Win32 클립보드 API
//   Android : AppActivity.copyToClipboard (JNI)
//   iOS     : UIPasteboard          (Clipboard_apple.mm)
//   Mac     : NSPasteboard          (Clipboard_apple.mm)
//   그 외   : no-op
class Clipboard
{
public:
	static void copy(const std::string& text);
};
