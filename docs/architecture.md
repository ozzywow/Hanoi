# Tower of Hanoi Olympic — 프로젝트 아키텍처

> 앱: **Tower of Hanoi Olympic** · 번들ID `com.ozzywow.TowerOfHanoiOlympic`
> 엔진: **Cocos2d-x 4.0 (C++)** · 빌드: **CMake** · 논리 해상도 960×640 / 리소스 480×320
> 대상: iOS(원본) → **Windows / Mac / iOS / Android** 멀티플랫폼
>
> 이 문서는 2026-07-20 리팩토링(아래 §7) 이후의 구조를 정리한 것이다.

---

## 1. 최상위 디렉터리

| 경로 | 내용 |
|------|------|
| `Classes/` | 크로스플랫폼 게임 소스 (C++) + 플랫폼 네이티브 브리지 |
| `Resources/` | 리소스 (이미지 480×320, plist, 사운드) |
| `cocos2d/` | Cocos2d-x 4.0 엔진 (git clone, `download-deps.py` 필요) |
| `proj.win32/` `proj.ios_mac/` `proj.android/` | 플랫폼별 진입점/프로젝트 |
| `build_win32/` | Windows CMake 빌드 산출물 (git 미추적) |
| `docs/` | 설계 문서 (본 문서 포함) |
| `server/` | PlayFab CloudScript·banned words 등 서버 자산 |
| `tools/` | BGM/사운드 생성 파이썬, PlayFab 정리 도구 |
| `legacy_ios/` | **iOS 오리지널 Objective-C(.m)** — 참조용, **빌드 제외** (§7 P0) |

빌드 진실원(source of truth)은 루트 `CMakeLists.txt`. Xcode `.xcodeproj`는 Mac에서 CMake로 생성하며 git에 미포함.

---

## 2. Classes/ 파일 맵 (기능별)

### 앱 진입점 / 전역
| 파일 | 역할 | 플랫폼 |
|------|------|--------|
| `AppDelegate.cpp/.h` | 크로스플랫폼 cocos2d 앱 델리게이트 (씬 부트) | 전체 |
| `stdafx.h/.cpp` | precompiled header (`cocos2d.h` + `common_define.h`) | 전체 |
| `common_define.h/.cpp` | 전역 상수·enum·유틸 함수(`getMilliCount`/`countryToFlag`/`getLocalGreeting`/`getRecordTime`) | 전체 |
| `GameConfig.h` `Singleton.h` `SimulatorConfig.h` | 설정 매크로 / 싱글턴 템플릿 / 시뮬레이터 구성 | 전체 |
| `HanoiAppDelegate.h/.mm` `RootViewController.h/.mm` | iOS `UIApplicationDelegate` / 루트 뷰컨트롤러 | **iOS 전용** |

### 씬 (Scenes)
| 파일 | 역할 |
|------|------|
| **`MainScene`** (`.h` + 4 TU) | 타이틀 / 랭킹보드 / BGM 플레이어 — §3 |
| **`PlayScene`** (`.h` + 4 TU) | 게임 플레이 / 도크 UI — §4 |
| `PlaySceneTouchHandlerLayer.cpp/.h` | 폴 터치 입력 레이어 |
| `Discus.cpp/.h` | 원반(디스크) 노드 |

### 매니저 (싱글턴, `Singleton<T>`)
| 파일 | 역할 |
|------|------|
| `LeaderboardManager.cpp/.h` | PlayFab: 랭킹·리플레이·수상소감·배틀 낙인·버전 게이트·이름 필터 (1,300줄+) |
| `UserDataManager.cpp/.h` | 로컬 유저 데이터(최고기록·레벨·사운드 옵션·pending submit) |
| `SoundFactory.cpp/.h` | 효과음/BGM 재생·크로스페이드·페이드 (`SoundFactory.m`은 legacy) |
| `ReviewManager.cpp/.h` (+ `ReviewBridge_ios.mm`) | 인앱 리뷰 게이팅(신기록·누적3회·버전당1회) |

### IAP (인앱결제) — §5
| 파일 | 역할 | 플랫폼 |
|------|------|--------|
| `IAPManager.h/.cpp` | 크로스플랫폼 파사드 `IAPManager`(싱글턴) + 내부 `IAPPlatform`·`IAPIndicator` | 전체 |
| `IAPDelegate.h` | 결제 콜백 인터페이스 (씬이 구현) | 전체 |
| `IAPManager_ios.mm` | iOS StoreKit 구현 (`IAPPlatform`/`IAPIndicator`) | **iOS 전용** |
| `MKStoreManager.h/.mm` `MKStoreObserver.h/.mm` `MKDelegateCPP.h/.mm` | iOS 네이티브 StoreKit 매니저·옵저버·ObjC↔C++ 브리지 | **iOS 전용** |
| `AndroidBilling.cpp/.h` | Android Play Billing JNI 브리지 | **Android 전용** |

### 드로잉 / UI
| 파일 | 역할 |
|------|------|
| `DrawUtils.cpp/.h` | LED 패널·키캡·벡터 아이콘·팝업 프레임/칩버튼·BGM 카세트·`hsv2rgb` |
| `PixelFont.cpp/.h` | 5×7 픽셀 폰트 렌더링(`makePixelText`, `drawPixelGlyphs`) |

---

## 3. MainScene — 4개 번역단위(TU)

`MainScene` **클래스 하나**를 4개 `.cpp`로 나눠 구현(인터페이스·동작 무변경). 헤더 `MainScene.h`는 그대로.

| 파일 | 담당 |
|------|------|
| `MainScene.cpp` | 코어 — 씬 생명주기 / `init` / 타이틀 / START 픽셀 애니 / IAP 델리게이트 / nameplate |
| `MainScene_Rank.cpp` | 온라인 랭킹보드 `drawOnlineRank` + 페이지 콜백 |
| `MainScene_Dialog.cpp` | 다이얼로그 — 이름 입력·설정·복수(revenge)·업데이트·수상소감·버전 게이트 |
| `MainScene_Bgm.cpp` | BGM 플레이어(카세트/재생목록) + 상·하단 티커 |
| `MainSceneInternal.h` | 파일 간 공유 심볼: `appStoreUrl()`, `utf8TruncateCP()` |

## 4. PlayScene — 4개 번역단위(TU)

동일 패턴. 헤더 `PlayScene.h`는 그대로.

| 파일 | 담당 |
|------|------|
| `PlayScene.cpp` | 코어 — 씬 생명주기 / 게임플레이(디스크·폴·판정) / 카운트다운 / 도크 콜백 / IAP / 결과 팝업 |
| `PlayScene_Replay.cpp` | 리플레이 · 고스트 레이스 · 관전 + blob 직렬화(base64/varint) |
| `PlayScene_Hud.cpp` | 하단 패널 / 랭크 티커 / RPM·이퀄라이저 / 결과 HUD |
| `PlayScene_Fx.cpp` | idle · cheer · guide 연출 + 포디움 |
| `PlaySceneInternal.h` | 공유 심볼: `arrPosOfPole[3]`, 리플레이 직렬화(`encodeReplayBlob`/`replayKey`), 공유 상수(`REPLAY_MAX_DELTA_MS`·`REPLAY_BLOB_MAX_CHARS`·`BOTTOM_PANEL_Y`·`TAG_PODIUM_BASE`) |

> **TU 분할 규약**: 여러 `.cpp`가 같은 클래스를 구현하므로, **둘 이상의 파일이 참조하는 파일 로컬 심볼**(전역 변수·static 함수·상수)만 `*Internal.h`로 승격한다. 한 파일에서만 쓰는 `static`은 해당 `.cpp`에 그대로 둔다. 새 멤버 함수는 해당 담당 파일에 정의.

---

## 5. IAP 계층 구조

```
씬(MainScene/PlayScene)  ──implements──▶  IAPDelegate            (결제 콜백 인터페이스)
        │ 호출
        ▼
   IAPManager (싱글턴 파사드)
        │ 위임
        ▼
   IAPPlatform (플랫폼 브리지)           IAPIndicator (로딩 인디케이터)
   ├─ Win/Mac/Linux : 인라인 스텁(no-op)
   ├─ Android        : AndroidBilling_* (JNI → Play Billing)
   └─ iOS            : IAPManager_ios.mm → MKStoreManager(StoreKit)
```

- **크로스플랫폼**은 `IAPManager.h/.cpp` + `IAPDelegate.h` 3개 파일로 완결.
- iOS 네이티브 StoreKit(`MKStoreManager.*` 등)은 정당한 이름이라 유지, `MKDelegateCPP`가 ObjC↔C++ 브리지.
- Windows/Mac에서 IAP는 **스텁**(구매 없음)으로 링크.

---

## 6. 빌드 & 플랫폼

### Include 체인
```
stdafx.h → cocos2d.h + common_define.h( → DrawUtils.h )
```
씬 헤더는 `common_define.h`, `IAPDelegate.h`(LITE_VER 시 IAP 델리게이트 상속), 필요한 매니저 헤더를 포함.

### 플랫폼별 컴파일 대상 (CMakeLists.txt)
- **크로스플랫폼**(`GAME_SOURCE`): 씬 4-TU × 2, 매니저, `IAPManager.cpp`, `common_define.cpp`, `DrawUtils`/`PixelFont` 등.
- **Windows**(`proj.win32/main.cpp`): IAP·인디케이터는 스텁.
- **Android**(`proj.android/.../main.cpp` + `AndroidBilling.cpp`): Play Billing JNI.
- **iOS**(`proj.ios_mac/ios/main.m` + `.mm` 브리지들): StoreKit·GameKit 링크. `HanoiAppDelegate.mm`, `RootViewController.mm`, `MKStoreManager.mm`, `IAPManager_ios.mm`, `MKStoreObserver.mm`, `MKDelegateCPP.mm`, `ReviewBridge_ios.mm`.
- **Mac**(`proj.ios_mac/mac/main.cpp`).

### Windows 빌드 (VS Code 태스크 or 터미널)
```powershell
# 소스/CMakeLists 변경 시 재구성
cmake -B build_win32 -S . -G 'Visual Studio 17 2022' -A Win32
# 빌드
cmake --build build_win32 --config Debug
```
> 새 `.cpp` 추가 시 CMake 재구성 필요. iOS/Mac는 Mac에서 CMake로 `.xcodeproj` 생성 후 Xcode 빌드.

### Xcode 주의 (Mac)
`.xcodeproj`가 git 미포함이므로 CMake 재생성으로 파일 목록을 맞춘다. 수동 관리 시 §2의 iOS 전용 `.mm`/`.h`가 타겟에 포함돼야 함.

---

## 7. 리팩토링 이력 (2026-07-20)

구조를 대대적으로 정리. 모든 변경은 **동작·인터페이스 무변경**(순수 코드 이동/이름 통일).

| 단계 | 내용 |
|------|------|
| **P0** 정리 | iOS 원본 `.m` 6개 → `legacy_ios/`(빌드 제외), `vc140.pdb` 추적 제외 + `*.pdb` gitignore |
| **P1** IAP 통합 | `CMKStoreManager`→`IAPManager`, `iosLink_MKStoreManager`→`IAPPlatform`, `iosUI`→`IAPIndicator`, `MKStoreManagerDelegate`→`IAPDelegate`. 크로스플랫폼 파일 6→3 |
| **P2** PlayScene 분해 | 3,112줄 God class → 코어/Replay/Hud/Fx 4 TU + `PlaySceneInternal.h` |
| **P3** MainScene 분해 | 2,480줄 → 코어/Rank/Dialog/Bgm 4 TU + `MainSceneInternal.h` |
| **P4** 헤더 유틸 이동 | `common_define.h` static 유틸 함수 → `common_define.cpp`(TU별 중복 컴파일 해소) |

> 각 단계는 자체검증 스플리터(라인 보존·중괄호/전처리기 균형·메서드 수 대조) + 유저 빌드 검증을 거쳐 커밋(브랜치 `refactor/iap-playscene-split` → master ff 병합).
> **미실시**: 진짜 클래스 추출(멤버를 별도 클래스로 옮겨 결합도 완화)은 위험이 커 이번 범위 제외 — TU 분할로 탐색성만 우선 확보. 필요 시 후속.

---

## 8. 코드 규칙

- 드로우 로직은 `DrawUtils.cpp`, 픽셀 폰트는 `PixelFont.cpp`에. `common_define.h`엔 상수/enum/유틸 선언만(드로우 로직 금지).
- 새 멤버 함수는 해당 씬의 담당 TU에 정의. 여러 TU 공유 심볼만 `*Internal.h`로.
- 국가 표시는 PNG 대신 `countryToFlag()` 유니코드 이모지.
- 문자열 자르기는 바이트 아닌 **코드포인트** 단위(`utf8TruncateCP`).
- iOS 전용 코드는 `#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS` 또는 `.mm` 분리. Windows/Mac IAP는 스텁.
- 빌드/실행은 개발자가 직접 수행.
