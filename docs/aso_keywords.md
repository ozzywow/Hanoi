# ASO 키워드 전략 — Tower of Hanoi Olympic

> 두 스토어의 키워드 처리 방식이 다릅니다.
> - **Google Play**: 키워드 필드 없음 → 제목·짧은설명·전체설명 **본문 밀도**로 랭킹. 핵심어를 자연스럽게 반복 노출.
> - **iOS App Store**: 별도 **키워드 필드 100자** 존재 → 콤마 구분, 공백 없이, 제목/부제와 중복 금지(중복은 낭비). 아래 100자 최적화 문자열 제공.

---

## 1. 키워드 티어 (언어 공통 개념)

| 티어 | 성격 | 예시 |
|------|------|------|
| **브랜드/코어** | 앱 정체성, 필수 | tower of hanoi, hanoi tower |
| **장르** | 검색량 큼, 경쟁도 큼 | puzzle, brain, logic puzzle |
| **차별화** | 이 앱 고유, 전환율 높음 | speedrun, time attack, ranking, leaderboard |
| **롱테일** | 검색량 작지만 경쟁 낮음 | tower of hanoi game, brain speed game |

전략: Google Play 본문엔 4티어 모두 녹이고, iOS 키워드 필드엔 **장르+차별화+롱테일** 위주(브랜드는 제목에 이미 있으므로 필드에서 제외).

---

## 2. 언어별 키워드 세트

### 🇺🇸 English
- 코어: `tower of hanoi`, `hanoi tower`
- 장르: `puzzle game`, `brain teaser`, `logic puzzle`, `brain game`, `iq`
- 차별화: `speedrun`, `time attack`, `ranking`, `leaderboard`, `global`
- 롱테일: `tower of hanoi game`, `disc puzzle`, `move puzzle`, `brain speed`
- **iOS 키워드필드 (100자, 콤마구분·공백없음)**:
```
hanoi,puzzle,brain,logic,iq,speedrun,timeattack,ranking,leaderboard,disc,teaser,strategy,global
```

### 🇪🇸 Español
- 코어: `torre de hanoi`, `torre hanoi`
- 장르: `rompecabezas`, `puzle`, `juego de logica`, `juego mental`, `ingenio`
- 차별화: `speedrun`, `contrarreloj`, `ranking`, `clasificacion`, `mundial`
- 롱테일: `juego torre de hanoi`, `puzle de discos`, `reto mental`
- **iOS 키워드필드 (100자)**:
```
hanoi,rompecabezas,puzle,logica,mental,ingenio,contrarreloj,ranking,clasificacion,discos,reto,mundial
```

### 🇧🇷 Português (pt-BR)
- 코어: `torre de hanoi`, `torre hanoi`
- 장르: `quebra-cabeca`, `puzzle`, `jogo de logica`, `jogo mental`, `raciocinio`
- 차별화: `speedrun`, `contra o tempo`, `ranking`, `classificacao`, `mundial`
- 롱테일: `jogo torre de hanoi`, `quebra cabeca de discos`, `desafio mental`
- **iOS 키워드필드 (100자)**:
```
hanoi,quebracabeca,puzzle,logica,mental,raciocinio,speedrun,ranking,classificacao,discos,desafio,tempo
```

### 🇯🇵 日本語
- 코어: `ハノイの塔`, `ハノイ`
- 장르: `パズル`, `脳トレ`, `論理パズル`, `頭脳`, `知育`
- 차별화: `スピードラン`, `タイムアタック`, `ランキング`, `世界ランキング`, `競争`
- 롱테일: `ハノイの塔 ゲーム`, `円盤パズル`, `脳トレ ゲーム`
- **iOS 키워드필드 (100자, 일본어는 콤마 없이 공백/콤마 혼용도 가능하나 콤마 권장)**:
```
ハノイ,パズル,脳トレ,論理,頭脳,知育,スピードラン,タイムアタック,ランキング,円盤,競争,世界
```

### 🇨🇳 简体中文 (zh-Hans)
- 코어: `汉诺塔`, `河内塔`(별칭)
- 장르: `解谜`, `益智`, `烧脑`, `逻辑游戏`, `智力`
- 차별화: `速通`, `计时赛`, `排行榜`, `世界排名`, `竞速`
- 롱테일: `汉诺塔游戏`, `圆盘解谜`, `烧脑游戏`
- **iOS 키워드필드 (100자, 중국어는 콤마 구분)**:
```
汉诺塔,河内塔,解谜,益智,烧脑,逻辑,速通,计时赛,排行榜,世界排名,圆盘,竞速,智力
```

### 🇹🇼 繁體中文 (zh-Hant)
- 코어: `漢諾塔`, `河內塔`(별칭)
- 장르: `解謎`, `益智`, `燒腦`, `邏輯遊戲`, `智力`
- 차별화: `速通`, `計時賽`, `排行榜`, `世界排名`, `競速`
- 롱테일: `漢諾塔遊戲`, `圓盤解謎`, `燒腦遊戲`
- **iOS 키워드필드 (100자, 중국어는 콤마 구분)**:
```
漢諾塔,河內塔,解謎,益智,燒腦,邏輯,速通,計時賽,排行榜,世界排名,圓盤,競速,智力
```

> ⚠️ **중국 본토 iOS**는 zh-Hans 사용 + Google Play 미서비스. zh-Hant는 대만/홍콩 대상(Google Play·iOS 모두). 간체/번체는 키워드도 글자가 달라 **분리 등록** 필요.

---

## 3. Google Play 본문 삽입 원칙 (중요)
- 제목에 `Tower of Hanoi` 포함 → 최강 신호 (이미 반영됨).
- 짧은 설명 80자에 **코어 1 + 차별화 1~2** 반드시 포함 (예: "Tower of Hanoi … time-attack … ranking").
- 전체 설명: 핵심어를 **자연스럽게 3~5회** 반복. 과도한 나열(키워드 스터핑)은 정책 위반 위험 → 문장 안에서 녹일 것.
- 각 언어 store_listing.md 전체설명 하단 `Keywords:` 줄은 참고용 — **자연 문장에 이미 녹아있으면 삭제해도 무방**. (Google Play는 나열 줄보다 본문 문맥을 더 신뢰)

## 4. 다음 우선순위 언어 (요청 시 확장)
힌디(hi) · 인도네시아어(id) · 프랑스어(fr) · 독일어(de) · 러시아어(ru)
→ 검색 수요 대비 로컬라이즈 노력이 낮은 순. 필요하면 동일 포맷으로 추가.
