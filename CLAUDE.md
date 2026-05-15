# Hanoi Project

앱: Tower of Hanoi Olympic | 번들ID: `com.ozzywow.TowerOfHanoiOlympic` | 엔진: Cocos2d-x (C++)
목표: iOS(원본) → Windows / Mac / iOS 멀티플랫폼 빌드

## 현재 상태
- `Classes/` 소스만 존재, CMakeLists.txt·플랫폼 프로젝트 파일 없음
- Cocos2d-x 버전 미확정 (axmol fork 또는 4.0 검토)
- iOS 전용 파일: `MKStoreManager.*`, `HanoiAppDelegate.m`, `RootViewController.m`, `PlayScene.m`, `SoundFactory.m`

## 플랫폼 전략
- iOS 전용 코드 → `#ifdef TARGET_OS_IOS` 가드 또는 `platform/ios/` 디렉터리 분리
- Windows/Mac IAP → 스텁 구현
- CMakeLists.txt 작성 필요

## 주의
- `common_define.h`: `static` 함수 다수 → 다중 include 시 중복 경고
- `PlayScene.m`: Objective-C → 크로스플랫폼 빌드에서 제외 필요
- 해상도: 논리 960×640 / 리소스 480×320
가능한 