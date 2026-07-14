# 스토어 등록정보 & ASO 키워드 통합 — Tower of Hanoi

> **대상 필드**
> - **Google Play**: 제목 30자 / 짧은설명 80자 / 전체설명 4000자 · **키워드 필드 없음**(본문 밀도로 랭킹)
> - **iOS App Store**: 이름 30자 / 부제 30자 / **키워드 필드 100자**(콤마 구분, 공백 없이, 제목·부제와 중복 금지)
>
> **기준 기능**: 원반 3~10개, RPM 타임어택, 국가별 글로벌 랭킹(국기), 고스트/리플레이 대결, 칩튠 BGM+레인보우 이퀄라이저, 카운트다운 신호등, Top10 수상소감. 무료.

---

## 📐 필드 → 스토어 매핑

| 필드 | 소속 | 검색 인덱싱 |
|------|:---:|:---:|
| 앱 이름/제목 | 공통 | ✅ |
| 부제(Subtitle) | **iOS 전용** | ✅ |
| iOS 키워드필드(100자) | iOS 전용 | ✅ |
| 짧은 설명(80) | **Google Play 전용** | ✅ |
| 전체 설명 | 공통(iOS는 검색 미인덱싱) | Play만 |

→ iOS는 **이름+부제+키워드필드를 조합**해 매칭하므로, 이름·부제에 든 단어는 키워드필드에서 **제외**(중복=100자 낭비). 각 언어의 `제외` 항목 참고.

## 🌐 영어 혼합 정책 (키워드필드)

| 시장 | 지배 검색어 | 영어 혼합 |
|------|:---:|:---:|
| 🇰🇷 한국 | 한/영 혼용 | ✅ 적극 |
| 🇯🇵 일본 | 일본어 우세(+영어 일부) | 🟡 소량(hanoi,puzzle) |
| 🇨🇳 본토·🇹🇼 대만 | 중국어 | ❌ 순수 유지 |
| 🇺🇸🇪🇸🇧🇷 | 자국어=검색어 | 자국어 그대로 |
| 🇮🇳🇸🇬 인도·싱가포르 | 영어 | (확장 시 영어 위주) |

원칙: **"유저의 실제 검색 언어에 맞춰라."** 한국·일본은 이중언어라 혼합 이득, 중화권은 자국어 지배라 순수 유지가 정답.

## 🏷 키워드 티어 (언어 공통 개념)

| 티어 | 성격 | 예시 |
|------|------|------|
| **브랜드/코어** | 앱 정체성 | tower of hanoi, hanoi tower |
| **장르** | 검색량 큼·경쟁 큼 | puzzle, brain, logic puzzle |
| **차별화** | 고유·전환율 높음 | speedrun, time attack, ranking |
| **롱테일** | 검색량 작지만 경쟁 낮음 | tower of hanoi game, brain speed |

전략: Google Play 본문엔 4티어 모두 녹이고, iOS 키워드필드엔 **장르+차별화+롱테일** 위주(브랜드는 이름에 이미 있으므로 제외).

---

# 언어별 통합 세트

## 🇰🇷 한국어 (기준/원본)

- **Title (30)**: 하노이의 탑 - 스피드런 랭킹   (16자)
- **Subtitle (30)**: 최고 속도 타임어택 · 세계 랭킹 대결
- **Short description (80)**: 하노이의 탑을 최고 속도로! RPM 타임어택 · 국가별 글로벌 랭킹 대결
- **Full description**:
```
🏆 하노이의 탑, 이제 스피드런이다.

전 세계 플레이어와 겨루는 타임어택 퍼즐. 원반을 최소 이동으로, 그리고 최고 속도로 옮겨 당신의 기록을 세우세요.

■ 핵심 기능
· 원반 3개부터 10개까지 — 입문부터 고인물까지
· RPM 미터 — 당신의 손놀림 속도를 실시간 측정
· 국가별 글로벌 랭킹 — 국기를 걸고 세계 순위 경쟁
· 고스트 대결 — 랭커의 플레이를 그대로 리플레이하며 추격
· 칩튠 BGM + 레인보우 이퀄라이저 — 속도에 반응하는 사운드
· 카운트다운 신호등 스타트 — 진짜 경기 같은 긴장감
· Top10 수상소감 — 정상에 오른 자만이 남기는 한마디

■ 이런 분께
· 두뇌 퍼즐과 스피드런을 동시에 즐기고 싶은 분
· 글로벌 랭킹 경쟁을 좋아하는 분
· 짧고 굵게 몰입하고 싶은 분

무료로 플레이하세요. 정상은 당신을 기다립니다.
```

**ASO 키워드**
- 코어: `하노이의 탑`, `하노이탑` · 장르: `퍼즐`, `두뇌`, `논리퍼즐`, `두뇌게임`, `집중력` · 차별화: `스피드런`, `타임어택`, `랭킹`, `순위`, `기록` · 롱테일: `하노이의 탑 게임`, `원반 퍼즐`, `두뇌 훈련`
- 이름+부제 이미 포함(→ **제외**): `하노이`, `탑`, `스피드런`, `랭킹`, `최고`, `속도`, `타임어택`, `세계`, `대결`
- **iOS 키워드필드 (93자, 영어 혼합판)** — 한국은 한/영 혼용 검색:
```
tower,hanoi,puzzle,math,brain,rank,block,kids,iq,logic,disc,두뇌,퍼즐,논리,집중력,원반,경쟁,순위,기록,학습,전략,고전
```
> `olympic`은 상표 리스크로 제외, `game`·`두뇌게임`은 저가치/중복이라 `logic`·`disc`로 교체. `학습`으로 교육 검색층도 커버.

---

## 🇺🇸 English (en-US)

- **Title (30)**: Tower of Hanoi - Speedrun   (25자)
- **Subtitle (30)**: Time-attack puzzle rankings
- **Short description (80)**: Solve the Tower of Hanoi at top speed. Global time-attack rankings by country!
- **Full description**:
```
🏆 The Tower of Hanoi — now it's a speedrun.

Race players worldwide in this time-attack puzzle. Move the discs in the fewest moves AND the fastest time to set your record.

■ Features
· 3 to 10 discs — from beginner to master
· RPM meter — measures your solving speed in real time
· Global leaderboards by country — fly your flag on the world ranking
· Ghost battle — replay a top ranker's run and chase their ghost
· Chiptune soundtrack + rainbow equalizer that reacts to your speed
· Traffic-light countdown start — real competition tension
· Top 10 award comments — only champions leave their mark

■ Perfect for
· Fans of brain puzzles AND speedrunning
· Players who love climbing global leaderboards
· Anyone who wants short, intense sessions

Free to play. The summit is waiting.
```

**ASO 키워드**
- 코어: `tower of hanoi`, `hanoi tower` · 장르: `puzzle game`, `brain teaser`, `logic puzzle`, `brain game`, `iq` · 차별화: `speedrun`, `time attack`, `ranking`, `leaderboard`, `global` · 롱테일: `tower of hanoi game`, `disc puzzle`, `move puzzle`, `brain speed`
- 이름+부제 이미 포함(→ **제외**): `tower`, `hanoi`, `speedrun`, `time`, `attack`, `puzzle`, `ranking`
- **iOS 키워드필드 (콤마구분·공백없음)**:
```
brain,logic,iq,leaderboard,disc,teaser,strategy,global,mind,classic,move,record,reflex
```

---

## 🇪🇸 Español (es-ES / es-419)

- **Title (30)**: Torre de Hanói - Speedrun   (25자)
- **Subtitle (30)**: Puzle contrarreloj y ranking
- **Short description (80)**: ¡Resuelve la Torre de Hanói a máxima velocidad! Ranking global por países.
- **Full description**:
```
🏆 La Torre de Hanói — ahora es un speedrun.

Compite contra jugadores de todo el mundo en este puzle contrarreloj. Mueve los discos con el menor número de movimientos Y en el menor tiempo para marcar tu récord.

■ Características
· De 3 a 10 discos — de principiante a experto
· Medidor de RPM — mide tu velocidad en tiempo real
· Clasificaciones globales por país — luce tu bandera en el ranking mundial
· Duelo fantasma — repite la partida de un top y persigue su fantasma
· Banda sonora chiptune + ecualizador arcoíris que reacciona a tu velocidad
· Salida con semáforo de cuenta atrás — tensión de competición real
· Comentarios del Top 10 — solo los campeones dejan su huella

■ Ideal para
· Amantes de los puzles de ingenio Y del speedrunning
· Quienes disfrutan escalando clasificaciones globales
· Cualquiera que busque partidas cortas e intensas

Gratis. La cima te espera.
```

**ASO 키워드**
- 코어: `torre de hanoi`, `torre hanoi` · 장르: `rompecabezas`, `puzle`, `juego de logica`, `juego mental`, `ingenio` · 차별화: `speedrun`, `contrarreloj`, `ranking`, `clasificacion`, `mundial` · 롱테일: `juego torre de hanoi`, `puzle de discos`, `reto mental`
- 이름+부제 이미 포함(→ **제외**): `torre`, `hanoi`, `speedrun`, `puzle`, `contrarreloj`, `ranking`
- **iOS 키워드필드**:
```
rompecabezas,logica,mental,ingenio,clasificacion,discos,reto,mundial,cerebro,juego,record,rapido
```

---

## 🇧🇷 Português (pt-BR)

- **Title (30)**: Torre de Hanói - Speedrun   (25자)
- **Subtitle (30)**: Puzzle veloz e ranking global
- **Short description (80)**: Resolva a Torre de Hanói na velocidade máxima! Ranking global por país.
- **Full description**:
```
🏆 A Torre de Hanói — agora é um speedrun.

Dispute com jogadores do mundo todo neste quebra-cabeça contra o tempo. Mova os discos com o menor número de jogadas E no menor tempo para cravar seu recorde.

■ Recursos
· De 3 a 10 discos — do iniciante ao mestre
· Medidor de RPM — mede sua velocidade em tempo real
· Rankings globais por país — mostre sua bandeira no ranking mundial
· Duelo fantasma — reveja a partida de um top e persiga o fantasma dele
· Trilha chiptune + equalizador arco-íris que reage à sua velocidade
· Largada com semáforo de contagem regressiva — tensão de competição real
· Comentários do Top 10 — só os campeões deixam sua marca

■ Perfeito para
· Fãs de quebra-cabeças de raciocínio E de speedrun
· Quem adora subir em rankings globais
· Quem quer partidas curtas e intensas

Grátis para jogar. O topo espera por você.
```

**ASO 키워드**
- 코어: `torre de hanoi`, `torre hanoi` · 장르: `quebra-cabeca`, `puzzle`, `jogo de logica`, `jogo mental`, `raciocinio` · 차별화: `speedrun`, `contra o tempo`, `ranking`, `classificacao`, `mundial` · 롱테일: `jogo torre de hanoi`, `quebra cabeca de discos`, `desafio mental`
- 이름+부제 이미 포함(→ **제외**): `torre`, `hanoi`, `speedrun`, `puzzle`, `veloz`, `ranking`, `global`
- **iOS 키워드필드**:
```
quebracabeca,logica,mental,raciocinio,classificacao,discos,desafio,tempo,cerebro,jogo,recorde,foco
```

---

## 🇯🇵 日本語 (ja-JP)

- **Title (30)**: ハノイの塔 - スピードラン順位   (16자)
- **Subtitle (30)**: 最速タイムアタック・世界ランキング
- **Short description (80)**: ハノイの塔を最速で解け！RPMタイムアタック・国別グローバルランキング
- **Full description**:
```
🏆 ハノイの塔が、スピードランになった。

世界中のプレイヤーと競うタイムアタックパズル。円盤を最小手数で、そして最速で動かして自己記録を打ち立てよう。

■ 主な特徴
・円盤3枚から10枚まで — 初心者から達人まで
・RPMメーター — あなたの手の速さをリアルタイム計測
・国別グローバルランキング — 国旗を掲げて世界順位を競え
・ゴースト対戦 — ランカーのプレイをそのままリプレイして追走
・チップチューンBGM＋速度に反応するレインボーイコライザー
・信号カウントダウンスタート — 本物の競技のような緊張感
・トップ10の受賞コメント — 頂点に立った者だけが残せる一言

■ こんな人におすすめ
・頭脳パズルとスピードランを同時に楽しみたい人
・グローバルランキング競争が好きな人
・短時間で熱中したい人

無料でプレイ。頂点があなたを待っている。
```

**ASO 키워드**
- 코어: `ハノイの塔`, `ハノイ` · 장르: `パズル`, `脳トレ`, `論理パズル`, `頭脳`, `知育` · 차별화: `スピードラン`, `タイムアタック`, `ランキング`, `世界ランキング`, `競争` · 롱테일: `ハノイの塔 ゲーム`, `円盤パズル`, `脳トレ ゲーム`
- 이름+부제 이미 포함(→ **제외**): `ハノイ`, `塔`, `スピードラン`, `順位`, `最速`, `タイムアタック`, `世界`, `ランキング`
- **iOS 키워드필드 (영어 소량 혼합)** — `hanoi`·`puzzle`만 보수적으로:
```
hanoi,puzzle,パズル,脳トレ,論理,頭脳,知育,計時,円盤,競争,ゲーム,反射神経,記録,集中力
```
> 여유(≈55/100자)가 있어 `tower`·`brain`·`iq` 추가 여지도 있음. 일본은 자국어 검색 우세라 **소량 유지** 권장.

---

## 🇨🇳 简体中文 (zh-Hans / zh-CN)

> ⚠️ **Google Play는 중국 본토 미서비스** → 이 언어는 주로 **iOS App Store 중국 본토(대형 시장)** 및 싱가포르·말레이시아 화교권 대상. Google Play엔 넣어도 무방하나 우선순위 낮음.

- **Title (30)**: 汉诺塔 - 速通排行榜   (11자)
- **Subtitle (30)**: 极速计时赛 · 全球排行榜
- **Short description (80)**: 极速挑战汉诺塔！RPM计时赛 · 按国家排名的全球排行榜
- **Full description**:
```
🏆 汉诺塔，现在是一场竞速。

与全球玩家同场竞技的计时解谜。用最少的步数、最快的速度移动圆盘，刷新你的纪录。

■ 核心功能
· 3到10个圆盘 — 从新手到高手
· RPM速度表 — 实时测量你的手速
· 按国家的全球排行榜 — 亮出国旗，争夺世界排名
· 幽灵对战 — 回放高手的操作，追赶他的幽灵
· 芯片音乐BGM＋随速度律动的彩虹均衡器
· 红绿灯倒计时开局 — 真实比赛般的紧张感
· 前十名获奖感言 — 只有登顶者才能留下的一句话

■ 适合这样的你
· 同时喜欢烧脑解谜和速通挑战
· 热衷于冲击全球排行榜
· 想要短小而专注的对局

免费畅玩。巅峰在等你。
```

**ASO 키워드** (영어 혼합 ❌ — 본토 중국어 검색 지배)
- 코어: `汉诺塔`, `河内塔`(별칭) · 장르: `解谜`, `益智`, `烧脑`, `逻辑游戏`, `智力` · 차별화: `速通`, `计时赛`, `排行榜`, `世界排名`, `竞速` · 롱테일: `汉诺塔游戏`, `圆盘解谜`, `烧脑游戏`
- 이름+부제 이미 포함(→ **제외**): `汉诺塔`, `速通`, `极速`, `计时赛`, `全球`, `排行榜`
- **iOS 키워드필드**:
```
河内塔,解谜,益智,烧脑,逻辑,世界排名,圆盘,竞速,智力,大脑,记录,专注
```

---

## 🇹🇼 繁體中文 (zh-Hant / zh-TW)

> 대만·홍콩 대상 — **Google Play·iOS 모두 유효**. (홍콩 겨냥 시 `hanoi`,`puzzle` 소량 혼합 옵션 있음)

- **Title (30)**: 漢諾塔 - 速通排行榜   (11자)
- **Subtitle (30)**: 極速計時賽 · 全球排行榜
- **Short description (80)**: 極速挑戰漢諾塔！RPM計時賽 · 依國家排名的全球排行榜
- **Full description**:
```
🏆 漢諾塔，現在是一場競速。

與全球玩家同場較勁的計時解謎。用最少的步數、最快的速度移動圓盤，刷新你的紀錄。

■ 核心功能
· 3到10個圓盤 — 從新手到高手
· RPM速度表 — 即時測量你的手速
· 依國家的全球排行榜 — 亮出國旗，爭奪世界排名
· 幽靈對戰 — 重播高手的操作，追趕他的幽靈
· 晶片音樂BGM＋隨速度律動的彩虹等化器
· 紅綠燈倒數開局 — 真實比賽般的緊張感
· 前十名得獎感言 — 只有登頂者才能留下的一句話

■ 適合這樣的你
· 同時喜歡燒腦解謎和速通挑戰
· 熱衷於衝擊全球排行榜
· 想要短小而專注的對局

免費暢玩。巔峰在等你。
```

**ASO 키워드** (영어 혼합 ❌ — 대만 중국어 검색 지배)
- 코어: `漢諾塔`, `河內塔`(별칭) · 장르: `解謎`, `益智`, `燒腦`, `邏輯遊戲`, `智力` · 차별화: `速通`, `計時賽`, `排行榜`, `世界排名`, `競速` · 롱테일: `漢諾塔遊戲`, `圓盤解謎`, `燒腦遊戲`
- 이름+부제 이미 포함(→ **제외**): `漢諾塔`, `速通`, `極速`, `計時賽`, `全球`, `排行榜`
- **iOS 키워드필드**:
```
河內塔,解謎,益智,燒腦,邏輯,世界排名,圓盤,競速,智力,大腦,記錄,專注
```

> ⚠️ 간체/번체는 글자가 달라 **분리 등록** 필요.

---

# 📌 적용 가이드

## Google Play 본문 삽입 원칙 (키워드필드 없음)
- 제목에 브랜드 핵심어(`Tower of Hanoi` 등) 포함 → 최강 신호 (반영됨).
- 짧은 설명 80자에 **코어 1 + 차별화 1~2** 반드시 포함.
- 전체 설명: 핵심어를 **자연스럽게 3~5회** 반복. 과도한 나열(키워드 스터핑)은 정책 위반 → 문장에 녹일 것.
- 전체설명 하단 `Keywords:`류 나열 줄은 참고용 — 본문에 이미 녹아있으면 삭제해도 무방(Google Play는 본문 문맥을 더 신뢰).

## 등록 시 체크리스트
- **제목 글자수**: 전 언어 "브랜드 - Speedrun[/스피드런/速通...]" 포맷, 모두 30자 이내 (EN/ES/PT 25자, KO/JA 16자, 중국어 11자).
- **"올림픽/Olympic" 제거**: 전 언어 제목에서 삭제 → 브랜드 일관 + IOC "Olympic" 상표 리스크 해소. (앱 번들ID `TowerOfHanoiOlympic`는 그대로 유지)
- Play Console/App Store Connect에서 언어별 **커스텀 스토어 등록정보** 추가 후 위 문구 입력.
- 스크린샷 캡션·피처 그래픽도 같은 언어로 현지화하면 전환율 상승 (별도 작업 권장). 경쟁(랭킹)·교육(두뇌 훈련) 두 각도를 컷 분리해 보여주면 유입층 확대.
- **iOS 키워드필드**: 콤마 구분·공백 없이, 이름/부제와 중복 금지, 100자 이내.

## 다음 우선순위 언어 (요청 시 확장)
힌디(hi) · 인도네시아어(id) · 프랑스어(fr) · 독일어(de) · 러시아어(ru)
→ 인도·싱가포르는 영어 위주 필드가 오히려 정답. 검색 수요 대비 로컬라이즈 노력이 낮은 순으로 확장.
