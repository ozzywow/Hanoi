# 유튜브 숏츠 제작 가이드

> 가로 녹화본 → 9:16 세로 숏츠. ffmpeg 만 사용 (영상 편집기 불필요)
> 작성일: 2026-08-07 · 기준 결과물: `shorts_hanoi_lv5.mp4` (LEVEL 5, 16.87초)

---

## 1. 핵심 원칙 — 유튜브 편집기를 쓰지 않는다

가로 영상을 올린 뒤 **「Shorts 동영상으로 수정」을 누르면 자동 크롭된다.** 이 편집기는 프레임을
꽉 채우는 게 동작이라 크롭을 끌 수 없다. 확대·위치 조정만 가능하고 레터박스 옵션이 없다.

**해법은 업로드 전에 파일 자체를 9:16 으로 만드는 것 하나뿐이다.**

유튜브는 **파일의 프레임 크기**로 Shorts 를 판별한다. 두 조건만 맞으면 자동 분류된다:

| 조건 | 값 |
|---|---|
| 가로세로비 | 정사각 또는 세로 (1:1 이하) |
| 길이 | 3분 이하 |

1080×1920 파일을 평소대로 업로드하면 「Shorts 동영상으로 수정」 버튼 **자체가 안 뜬다.**
내용이 레터박스인지는 판단하지 않으므로, 아래처럼 여백을 채워 넣은 파일도 그대로 통한다.

> `#Shorts` 해시태그는 예전엔 분류에 관여했으나 지금은 화면비·길이로 판별한다. 필수 아님.

---

## 2. 여백은 가리지 말고 채운다

1080×1920 에 3:2 게임 화면을 가로 꽉 채워 넣으면 높이가 약 720px. **나머지 1200px, 화면의 62%가
남는다.** 블러 배경으로 덮는 건 이 공간을 버리는 것 — 정보가 0이고 시청자는 "작은 영상"으로 읽는다.

3단 구성으로 채운다:

```
┌─────────────────┐
│  후킹 문구        │  상단 — 레벨 / 조건 / (확정 시) 순위
├─────────────────┤
│  게임 플레이      │  중앙 — 원본 그대로, 좌우 크롭 없음
├─────────────────┤
│  도발 + 브랜드    │  하단 — 랭킹보드를 넣을 수도 있음
└─────────────────┘
```

숏폼은 **소리 없이** 보는 사람이 많아 상단 문구가 사실상 유일한 설명이다.

배경색은 `#0a0e1a` — 랜딩([landing/index.html](../landing/index.html))·게임과 같은 네이비라
채널 전체가 한 브랜드로 보인다.

---

## 3. 녹화 단계에서 챙길 것

- **길이 10~20초.** 5초 미만은 상황 파악 전에 끝나 루프만 돈다. 긴장이 쌓일 시간이 필요하다.
- **레벨 6~7 이 소재로 유리하다.** 원반이 많아 화면이 화려하고, 최소 수순(`2^N - 1`)이 커서
  제목의 숫자가 강해진다. 레벨 5=31, 6=63, 7=127, 8=255.
- **리플레이 화면보다 실제 플레이가 낫다.** 리플레이는 하단에 `PAUSE / SPEED / STOP` 컨트롤이
  붙어 잘라내야 하고, 결과 팝업(`NEW RECORD!`)이라는 결말도 없다.
- 앞부분 **카운트다운 약 4초**는 잘라낸다.

---

## 4. 크롭 값 — 녹화 화면별

게임 프레임 바깥은 전부 죽은 여백이다. 좌측 도크(홈/재시작/레벨/BGM)도 시청자에겐 의미 없는 UI.

| 녹화 원본 | 화면 | crop 필터 | 결과 |
|---|---|---|---|
| 2400×1080 (폰 전체화면, 실제 플레이) | 게임 프레임 385~2010, 도크 405~540 | `crop=1465:1080:545:0` | 도크 제외, 상단바·이퀄라이저 유지 |
| 1618×1080 (리플레이 재생) | 하단 컨트롤바 890~ | `crop=1618:890:0:0` | 컨트롤바 제거, 상단 이름·타이머 유지 |

**유지할 것**: 상단 바(레벨/RPM/타이머 — 정보), 하단 레인보우 이퀄라이저(화면을 살림)
**버릴 것**: 좌측 도크, 리플레이 컨트롤, 좌우 네이비 여백

> 새 기기·해상도로 녹화하면 값이 달라진다. 프레임 뽑아 경계를 먼저 재라:
> `ffmpeg -ss 8 -i SRC -frames:v 1 -vf "crop=800:1080:250:0" left.png`

---

## 5. 제작 명령

### 5.1 폰트 준비 (Windows 필수)

ffmpeg `drawtext` 의 `fontfile` 에 `C:/...` 를 쓰면 **드라이브 문자의 콜론이 필터 문법의 구분자와
충돌**해 파싱 에러가 난다(`No option name near '/Windows/Fonts/...'`). 이스케이프도 잘 안 먹는다.

→ **폰트를 작업 폴더로 복사하고 상대 경로로 참조한다.** 가장 확실하다.

```bash
WORK=/c/temp/shorts && mkdir -p "$WORK"
cp /c/Windows/Fonts/arial.ttf /c/Windows/Fonts/arialbd.ttf "$WORK/"
```

### 5.2 필터 스크립트

`-filter_complex` 대신 `-filter_complex_script` 로 파일에서 읽는다(줄바꿈 가능 → 읽기 쉬움).

`$WORK/filter.txt`:

```
[0:v]crop=1465:1080:545:0,scale=1080:-2,setsar=1[g];
color=0x0a0e1a:1080x1920:d=60:r=60[bg];
[bg][g]overlay=0:560:shortest=1,
drawtext=fontfile=arialbd.ttf:text='LEVEL 5':fontsize=84:fontcolor=0xffcf4a:x=(w-text_w)/2:y=196,
drawtext=fontfile=arial.ttf:text='31 moves, no mistakes':fontsize=46:fontcolor=0xeaf0f8:x=(w-text_w)/2:y=314,
drawtext=fontfile=arial.ttf:text='Think you are faster?':fontsize=46:fontcolor=0x3ecf9a:x=(w-text_w)/2:y=1424,
drawtext=fontfile=arialbd.ttf:text='TOWER OF HANOI - SPEEDRUN':fontsize=46:fontcolor=0xeaf0f8:x=(w-text_w)/2:y=1502,
drawtext=fontfile=arial.ttf:text='Free on iOS and Android':fontsize=36:fontcolor=0x98a6be:x=(w-text_w)/2:y=1576[v]
```

### 5.3 인코딩

```bash
cd "$WORK" && ffmpeg -y -ss 4.1 -i "SRC.mp4" \
  -filter_complex_script filter.txt -map "[v]" -map 0:a -r 60 \
  -c:v libx264 -preset slow -crf 18 -pix_fmt yuv420p \
  -c:a aac -b:a 192k -movflags +faststart \
  "shorts_out.mp4"
```

- `-ss 4.1` **을 `-i` 앞에** 둔다 → 영상·오디오가 함께 잘려 싱크가 유지된다.
- `-r 60` — 유튜브 상한이 60fps. 원본이 90~100fps 여도 어차피 재인코딩되므로 미리 낮춘다.
- `-pix_fmt yuv420p` — 없으면 일부 플레이어에서 재생 불가.

### 5.4 검증

```bash
ffprobe -v error -select_streams v:0 \
  -show_entries stream=width,height,avg_frame_rate \
  -show_entries format=duration -of default=noprint_wrappers=1 shorts_out.mp4
```

`1080 / 1920 / 60/1` 이어야 한다. 프레임 확인:

```bash
ffmpeg -y -ss 0.2 -i shorts_out.mp4 -frames:v 1 -vf "scale=400:-2" p_start.png
```

---

## 6. 배치 좌표

세로 1920 기준. 게임 화면 높이는 크롭 비율에 따라 달라지므로 `overlay` y 값으로 조절한다.

| 원본 크롭 | 스케일 후 | overlay y | 게임 화면 구간 |
|---|---|---|---|
| `1465:1080` (실제 플레이) | 1080×796 | 560 | 560 ~ 1356 |
| `1618:890` (리플레이) | 1080×594 | 520 | 520 ~ 1114 |

**⚠️ 하단 안전 영역** — 유튜브 숏츠는 하단 약 250~300px 에 제목·채널명·버튼 UI 를 덮는다.
**중요한 텍스트는 y=1620 아래로 내리지 말 것.** 우측 100px 도 좋아요/공유 버튼이 차지한다.

랭킹보드를 합성할 경우(선택):

```
[1:v]scale=900:-2[lb];
... [a][lb]overlay=90:1225[b];
```

**상위 3위까지만 넣는다.** 10명 전체는 세로 500px 안에서 글자가 뭉개져 모바일에서 안 읽힌다.
그리고 **랭킹보드의 레벨과 영상의 레벨이 반드시 같아야 한다** — 다르면 "이 기록이 저 순위표"로
잘못 읽힌다.

---

## 7. 제목 · 설명

### 템플릿

```
제목: Tower of Hanoi Level {N} — {2^N-1} moves in {기록}s
1줄:  {N} discs. {2^N-1} moves is the minimum. No mistakes allowed.
```

`Tower of Hanoi` 를 **맨 앞에** 둔다. 실제 검색어이고, 모바일은 제목을 40자쯤에서 자른다.

### 설명 본문 (고정)

```
Tower of Hanoi - Speedrun turns the classic puzzle into a race:
live world rankings by country, ghost races against top players,
and a replay of every run.

Play free
iOS: https://apps.apple.com/app/id430261581
Android: https://play.google.com/store/apps/details?id=com.ozzywow.hanoi
Rankings: https://ozzywow.github.io/Hanoi/
Instagram: https://www.instagram.com/towerofhanoispeedrun

#TowerOfHanoi #Speedrun #PuzzleGame #BrainGame #Shorts
```

Android 링크를 빼거나 아래로 밀지 말 것 — **Play 스토어 해외 유입이 거의 없는 상황에서 이
설명란이 사실상 유일한 Android 직접 유입 경로다.**

---

## 8. 문구를 쓸 때의 판단

### 결과를 먼저 깔 것인가

| 영상 길이 | 상단 문구 | 이유 |
|---|---|---|
| ~5초 | **결과 공개** (`WORLD #1`, `13.53s`) | 이야기가 성립할 시간이 없다 |
| 10초 이상 | **결과 감춤** (`LEVEL 5 / 31 moves, no mistakes`) | 결과 팝업이 결말을 맡는다 |

긴 클립에서 시작부터 기록을 알려주면 마지막 `NEW RECORD!` 팝업이 김빠진다.

### ⚠️ "WORLD RECORD" 는 함부로 쓰지 않는다

게임 팝업의 `NEW RECORD!` 는 **본인 최고기록**이지 세계 기록이 아니다.
해당 레벨 랭킹보드에서 1위를 **확인한 경우에만** `WORLD #1` 을 쓴다.
확인 없이 썼다가 상위권이 드러나면 채널 신뢰를 잃는다.

### 랭킹보드에 노출하면 곤란한 이름

실명·학교·학년으로 읽히는 닉네임이 상위권에 있을 수 있다(예: `포항 환호여중 중3`).
합성 전 확인하고, 애매하면 그 행을 빼거나 상위 3위까지만 쓴다.

---

## 9. 배포 리듬

주 1회. 영상 하나를 만들어 **YouTube Shorts · TikTok · Instagram Reels 에 함께 올린다** —
제작비는 1인분, 도달은 3인분이다. 소재(신기록·고스트 대결·세계 기록)는 서버에 자동으로 쌓인다.
