# Hanoi Project

## 프로젝트 개요
- **앱명**: Tower of Hanoi Olympic
- **번들 ID**: `com.ozzywow.TowerOfHanoiOlympic`
- **엔진**: Cocos2d-x (C++)
- **원래 타겟**: iOS (App Store: id504138737)
- **목표**: Windows / Mac / iOS 멀티플랫폼 빌드 가능하게 재구성

## 현재 상태
- `Classes/` 소스만 존재 (CMakeLists.txt, 플랫폼 프로젝트 파일 없음)
- Cocos2d-x 버전 미확인 → 추후 결정 필요
- iOS 전용 코드가 섞여 있음 (`.m`, `.mm` 파일)

## 디렉터리 구조
```
Hanoi/
├── Classes/          # 전체 게임 소스
│   ├── AppDelegate.cpp/.h           # Cocos2d-x 앱 진입점
│   ├── MainScene.cpp/.h             # 메인 씬
│   ├── PlayScene.cpp/.h/.m          # 플레이 씬
│   ├── PlaySceneTouchHandlerLayer   # 터치 핸들러
│   ├── Discus.cpp/.h                # 원판(디스크) 오브젝트
│   ├── HelloWorldScene.cpp/.h       # 기본 씬 (미사용 추정)
│   ├── SoundFactory.cpp/.h/.m       # 사운드 관리
│   ├── UserDataManager.cpp/.h       # 유저 데이터 (저장)
│   ├── MKStoreManager.*             # iOS IAP (결제) - iOS 전용
│   ├── MKStoreObserver.*            # iOS IAP 옵저버 - iOS 전용
│   ├── HanoiAppDelegate.h/.m        # iOS AppDelegate - iOS 전용
│   ├── RootViewController.h/.m      # iOS ViewController - iOS 전용
│   ├── common_define.h              # 전역 상수, 유틸 함수
│   ├── GameConfig.h                 # 빌드 플래그 (KR_VERSION, AD_VER, LITE_VER)
│   ├── Singleton.h                  # 싱글톤 템플릿
│   └── stdafx.cpp/.h                # 프리컴파일드 헤더
├── Icon/             # 앱 아이콘 (iOS 사이즈)
└── ScreenShot/       # 앱스토어 스크린샷
```

## 핵심 상수 (common_define.h)
- 해상도: 960×640 (논리), 리소스: 480×320
- 최대 레벨: 10 (LITE 버전: 4레벨 제한)
- IAP: `com.ozzywow.TowerOfHanoiOlympic.FullVersion`

## 플랫폼 분리 전략 (작업 방향)
- iOS 전용 코드 (`MKStoreManager`, `HanoiAppDelegate`, `RootViewController`, `.m/.mm`)
  → `#ifdef __APPLE__` / `#ifdef TARGET_OS_IOS` 가드 또는 별도 디렉터리로 분리
- Windows/Mac IAP → 스텁(stub) 구현 또는 플랫폼별 대체
- CMakeLists.txt 작성 필요 (Cocos2d-x CMake 기반)

## 빌드 계획
1. Cocos2d-x 버전 선택 (권장: 4.0 or axmol fork)
2. CMakeLists.txt 작성
3. 플랫폼별 코드 가드 처리
4. Windows: Visual Studio / CMake
5. Mac: Xcode 또는 CMake
6. iOS: Xcode

## 주의사항
- `common_define.h`에 `static` 함수/변수가 여러 개 → 다중 include 시 중복 경고 가능
- `PlayScene.m` 존재 (Objective-C) → iOS 전용, 크로스플랫폼 빌드 시 제외 처리 필요
