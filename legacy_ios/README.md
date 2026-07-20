# legacy_ios — iOS 원본 Objective-C 소스 (참조용, 빌드 제외)

Cocos2d-x(C++) 포팅 이전의 **iOS 오리지널 소스**입니다.
빌드에는 포함되지 않습니다 — 각 파일은 아래의 컴파일 대응본으로 대체되었습니다.

| 레거시 원본 (.m)        | 현재 컴파일본            |
|-------------------------|-------------------------|
| `PlayScene.m`           | `Classes/PlayScene.cpp` |
| `MKStoreManager.m`      | `Classes/MKStoreManager.mm` |
| `MKStoreObserver.m`     | `Classes/MKStoreObserver.mm` |
| `HanoiAppDelegate.m`    | `Classes/HanoiAppDelegate.mm` |
| `RootViewController.m`  | `Classes/RootViewController.mm` |
| `SoundFactory.m`        | `Classes/SoundFactory.cpp` |

> 포팅 검증/대조 목적으로만 보관. 신규 코드는 `Classes/`에 작성할 것.
