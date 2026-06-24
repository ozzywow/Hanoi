# Hanoi Project

앱: Tower of Hanoi Olympic | 번들ID: `com.ozzywow.TowerOfHanoiOlympic` | 엔진: Cocos2d-x (C++)
목표: iOS(원본) → Windows / Mac / iOS 멀티플랫폼 빌드

## 현재 상태 (2026-06-24 기준)
- Android 빌드: `proj.android/` — CMakeLists.txt 기반, 정상 빌드 확인
- iOS/Mac 빌드: `proj.ios_mac/` — `.xcodeproj`는 Mac에만 존재 (git 미포함)
- iOS 전용 파일: `MKStoreManager.*`, `HanoiAppDelegate.m`, `RootViewController.m`, `PlayScene.m`, `SoundFactory.m`

## 아키텍처 — Classes/ 구조

```
common_define.h   전역 상수, enum, 타이밍 유틸, countryToFlag 등
stdafx.h          공통 precompiled header → common_define.h → DrawUtils.h 순 포함
DrawUtils.h/cpp   LED 패널, 키캡, 벡터 아이콘, BGM 카세트/스피커/전송, hsv2rgb
PixelFont.h/cpp   5×7 픽셀 폰트 렌더링 (pixelGlyph, drawPixelGlyphs, makePixelText)
MainScene         타이틀 / 랭킹보드 / BGM 플레이어
PlayScene         게임 플레이 / 도크 UI
```

Include 체인: `stdafx.h → common_define.h → DrawUtils.h → cocos2d.h`

## 플랫폼 전략
- iOS 전용 코드 → `#ifdef TARGET_OS_IOS` 가드 또는 `platform/ios/` 디렉터리 분리
- Windows/Mac IAP → 스텁 구현

## Xcode 빌드 주의 (Mac 환경)

`.xcodeproj`는 git에 포함되지 않으므로, 새 파일 추가 시 **Xcode에서 수동으로 타겟에 추가**해야 함.

현재 git에는 있으나 `.xcodeproj`에 미등록 가능성이 있는 파일:
- `Classes/DrawUtils.h` / `Classes/DrawUtils.cpp`
- `Classes/PixelFont.h` / `Classes/PixelFont.cpp`

→ Xcode에서 "Add Files to HanoiOlympic..." 로 위 4개 파일을 타겟에 추가할 것.
→ `HelloWorldScene.h/cpp`는 삭제됨 — Xcode 프로젝트에 참조가 남아있으면 제거 필요.

## 코드 규칙
- 드로우 함수는 `DrawUtils.cpp`에, 픽셀 폰트는 `PixelFont.cpp`에 추가
- `common_define.h`에 드로우 로직 추가 금지 (상수/enum/유틸만 유지)
- 해상도: 논리 960×640 / 리소스 480×320
