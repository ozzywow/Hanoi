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

> **영어 브랜드/장르어 소량 삽입** (2026-07-16): iOS 키워드필드는 100자 토큰 주머니라 여유분에 영어를 넣어도 로컬 밀도 희석 없음. 비영어권(id·ru·th·tr·fr·de·it·vi)에 `iq`·`brain`·`puzzle`·`tower` 중 여유만큼 추가(가장 저렴·고검색량 `iq`부터). 제목에 이미 있는 `hanoi`는 중복이라 제외. 포화 필드는 저가치 filler(`strategie`/`strategia`·`thử thách`)를 교체. **중화권·힌디(이미 영어 위주)는 제외.**

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

## 🇮🇳 हिन्दी (hi)

> 인도는 앱 검색이 **영어 우세** → 데바나가리 제목/부제로 힌디 검색층을 잡고, iOS 키워드필드는 **영어 위주 + 힌디 소량**이 정답. (제목이 데바나가리라 영어 `hanoi`·`tower`는 필드에서 중복 아님 → 넣어도 무방)

- **Title (30)**: हनोई टॉवर - स्पीडरन   (≈15자)
- **Subtitle (30)**: टाइम-अटैक पज़ल रैंकिंग
- **Short description (80)**: हनोई टॉवर को तेज़ गति से हल करें! देश-वार वैश्विक टाइम-अटैक रैंकिंग।
- **Full description**:
```
🏆 हनोई टॉवर — अब यह एक स्पीडरन है।

इस टाइम-अटैक पज़ल में दुनिया भर के खिलाड़ियों से मुकाबला करें। डिस्क को सबसे कम चालों में और सबसे तेज़ समय में घुमाकर अपना रिकॉर्ड बनाएं।

■ ख़ासियतें
· 3 से 10 डिस्क — शुरुआती से महारथी तक
· RPM मीटर — आपकी रफ़्तार को रियल-टाइम में मापता है
· देश-वार वैश्विक रैंकिंग — विश्व रैंकिंग में अपना झंडा लहराएं
· घोस्ट मुक़ाबला — टॉप खिलाड़ी की चाल दोहराएं और उसके घोस्ट का पीछा करें
· चिपट्यून संगीत + आपकी गति पर झूमता रेनबो इक्वलाइज़र
· ट्रैफ़िक-लाइट काउंटडाउन स्टार्ट — असली मुक़ाबले जैसा रोमांच
· टॉप 10 विजेता संदेश — सिर्फ़ चैंपियन ही अपनी छाप छोड़ते हैं

■ किनके लिए बढ़िया
· दिमाग़ी पज़ल और स्पीडरनिंग दोनों के शौकीन
· वैश्विक रैंकिंग चढ़ना पसंद करने वालों के लिए
· जो छोटे पर गहन सेशन चाहते हैं

मुफ़्त खेलें। शिखर आपका इंतज़ार कर रहा है।
```

**ASO 키워드**
- 코어: `tower of hanoi`, `hanoi tower` · 장르: `puzzle`, `brain game`, `logic puzzle`, `पहेली`, `दिमागी खेल` · 차별화: `speedrun`, `time attack`, `ranking`, `leaderboard`, `record` · 롱테일: `tower of hanoi game`, `disc puzzle`, `brain training`
- 데바나가리 이름+부제 이미 포함(→ **제외**): `हनोई`, `टॉवर`, `स्पीडरन`, `टाइम`, `अटैक`, `रैंकिंग`
- **iOS 키워드필드 (영어 위주 + 힌디 소량)** — 인도 유저 영어 검색 우세:
```
brain,logic,puzzle,iq,leaderboard,disc,strategy,global,record,दिमाग,पहेली,तेज़
```
> 데바나가리 제목이 힌디 검색을, 영어 필드가 실제 다수 검색을 커버. `olympic`은 상표 리스크로 제외.

---

## 🇮🇩 Bahasa Indonesia (id)

- **Title (30)**: Menara Hanoi - Speedrun   (23자)
- **Subtitle (30)**: Adu cepat & peringkat global
- **Short description (80)**: Selesaikan Menara Hanoi secepat mungkin! Peringkat global antarnegara.
- **Full description**:
```
🏆 Menara Hanoi — kini jadi ajang speedrun.

Bertanding dengan pemain dari seluruh dunia dalam puzzle lawan waktu ini. Pindahkan cakram dengan langkah paling sedikit DAN waktu tercepat untuk mencetak rekormu.

■ Fitur
· 3 hingga 10 cakram — dari pemula sampai master
· Meteran RPM — mengukur kecepatanmu secara real-time
· Papan peringkat global per negara — kibarkan benderamu di peringkat dunia
· Duel hantu — putar ulang permainan top ranker dan kejar hantunya
· Musik chiptune + equalizer pelangi yang bereaksi pada kecepatanmu
· Start hitung mundur lampu lalu lintas — tegangnya kompetisi sungguhan
· Komentar Top 10 — hanya juara yang meninggalkan jejak

■ Cocok untuk
· Penggemar puzzle otak DAN speedrunning
· Yang suka menanjak di papan peringkat global
· Siapa pun yang mau sesi singkat nan intens

Main gratis. Puncak menantimu.
```

**ASO 키워드**
- 코어: `menara hanoi`, `hanoi` · 장르: `puzzle`, `teka teki`, `game logika`, `asah otak`, `game otak` · 차별화: `speedrun`, `lawan waktu`, `peringkat`, `ranking`, `global` · 롱테일: `game menara hanoi`, `puzzle cakram`, `tantangan otak`
- 이름+부제 이미 포함(→ **제외**): `menara`, `hanoi`, `speedrun`, `peringkat`, `global`
- **iOS 키워드필드**:
```
tekateki,logika,otak,asah,cakram,tantangan,waktu,ranking,game,rekor,cepat,strategi,iq,brain,puzzle
```

---

## 🇫🇷 Français (fr)

- **Title (30)**: Tour de Hanoï - Speedrun   (24자)
- **Subtitle (30)**: Puzzle chrono et classement
- **Short description (80)**: Résolvez la Tour de Hanoï à toute vitesse ! Classement mondial par pays.
- **Full description**:
```
🏆 La Tour de Hanoï — c'est désormais un speedrun.

Affrontez des joueurs du monde entier dans ce puzzle contre la montre. Déplacez les disques en un minimum de coups ET le plus vite possible pour établir votre record.

■ Fonctionnalités
· De 3 à 10 disques — du débutant à l'expert
· Compteur RPM — mesure votre vitesse en temps réel
· Classements mondiaux par pays — hissez votre drapeau au classement mondial
· Duel fantôme — rejouez la partie d'un top joueur et poursuivez son fantôme
· Bande-son chiptune + égaliseur arc-en-ciel qui réagit à votre vitesse
· Départ au feu tricolore à rebours — la tension d'une vraie compétition
· Commentaires du Top 10 — seuls les champions laissent leur marque

■ Parfait pour
· Les amateurs de casse-tête ET de speedrunning
· Ceux qui adorent grimper dans les classements mondiaux
· Quiconque cherche des parties courtes et intenses

Gratuit. Le sommet vous attend.
```

**ASO 키워드**
- 코어: `tour de hanoi`, `hanoi` · 장르: `puzzle`, `casse-tete`, `jeu de logique`, `reflexion`, `jeu de cerveau` · 차별화: `speedrun`, `contre la montre`, `classement`, `chrono`, `mondial` · 롱테일: `jeu tour de hanoi`, `casse-tete disques`, `defi mental`
- 이름+부제 이미 포함(→ **제외**): `tour`, `hanoi`, `speedrun`, `puzzle`, `chrono`, `classement`
- **iOS 키워드필드**:
```
casse-tete,logique,reflexion,cerveau,disques,defi,mental,mondial,jeu,record,rapide,iq,brain,puzzle
```

---

## 🇩🇪 Deutsch (de)

- **Title (30)**: Türme von Hanoi - Speedrun   (26자)
- **Subtitle (30)**: Zeitrennen-Puzzle & Rangliste
- **Short description (80)**: Löse die Türme von Hanoi in Höchsttempo! Globale Rangliste nach Ländern.
- **Full description**:
```
🏆 Die Türme von Hanoi — jetzt als Speedrun.

Tritt gegen Spieler aus aller Welt in diesem Puzzle gegen die Zeit an. Bewege die Scheiben mit den wenigsten Zügen UND in der schnellsten Zeit, um deinen Rekord aufzustellen.

■ Funktionen
· 3 bis 10 Scheiben — vom Anfänger bis zum Meister
· RPM-Messer — misst deine Geschwindigkeit in Echtzeit
· Globale Ranglisten nach Ländern — zeig deine Flagge in der Weltrangliste
· Geister-Duell — spiele den Lauf eines Top-Spielers nach und jage seinen Geist
· Chiptune-Soundtrack + Regenbogen-Equalizer, der auf dein Tempo reagiert
· Ampel-Countdown-Start — echte Wettkampfspannung
· Top-10-Siegerkommentare — nur Champions hinterlassen ihre Spur

■ Perfekt für
· Fans von Denksport UND Speedrunning
· Alle, die gern in globalen Ranglisten aufsteigen
· Jeden, der kurze, intensive Runden sucht

Kostenlos spielen. Der Gipfel wartet auf dich.
```

**ASO 키워드**
- 코어: `türme von hanoi`, `hanoi turm` · 장르: `puzzle`, `denkspiel`, `logikspiel`, `knobelspiel`, `gehirnjogging` · 차별화: `speedrun`, `zeitrennen`, `rangliste`, `bestzeit`, `weltweit` · 롱테일: `türme von hanoi spiel`, `scheiben puzzle`, `denksport`
- 이름+부제 이미 포함(→ **제외**): `türme`, `hanoi`, `speedrun`, `puzzle`, `zeitrennen`, `rangliste`
- **iOS 키워드필드**:
```
denkspiel,logik,knobeln,gehirn,scheiben,bestzeit,denksport,weltweit,spiel,rekord,schnell,iq,brain
```

---

## 🇷🇺 Русский (ru)

- **Title (30)**: Ханойская башня - спидран   (25자)
- **Subtitle (30)**: Головоломка на время, рейтинг
- **Short description (80)**: Собери Ханойскую башню на максимальной скорости! Мировой рейтинг по странам.
- **Full description**:
```
🏆 Ханойская башня — теперь это спидран.

Соревнуйтесь с игроками со всего мира в этой головоломке на время. Перемещайте диски за наименьшее число ходов И на максимальной скорости, чтобы установить свой рекорд.

■ Возможности
· От 3 до 10 дисков — от новичка до мастера
· Счётчик RPM — измеряет вашу скорость в реальном времени
· Глобальные рейтинги по странам — поднимите свой флаг в мировом зачёте
· Дуэль с призраком — повторите заезд топ-игрока и догоните его призрака
· Чиптюн-саундтрек и радужный эквалайзер, реагирующий на вашу скорость
· Старт по светофору с обратным отсчётом — напряжение настоящих соревнований
· Комментарии Топ-10 — только чемпионы оставляют свой след

■ Идеально для
· Любителей головоломок И спидрана
· Тех, кто обожает подниматься в мировых рейтингах
· Всех, кто хочет коротких и напряжённых партий

Играйте бесплатно. Вершина ждёт вас.
```

**ASO 키워드**
- 코어: `ханойская башня`, `ханой` · 장르: `головоломка`, `логика`, `пазл`, `логическая игра`, `для ума` · 차별화: `спидран`, `на время`, `рейтинг`, `рекорд`, `мировой` · 롱테일: `игра ханойская башня`, `головоломка с дисками`, `тренировка мозга`
- 이름+부제 이미 포함(→ **제외**): `ханойская`, `башня`, `ханой`, `спидран`, `рейтинг`, `время`
- **iOS 키워드필드**:
```
головоломка,логика,пазл,ум,диски,рекорд,мозг,мировой,игра,скорость,стратегия,вызов,iq,brain,puzzle
```

---

## 🇹🇷 Türkçe (tr)

- **Title (30)**: Hanoi Kulesi - Speedrun   (23자)
- **Subtitle (30)**: Zamana karşı yarış · sıralama
- **Short description (80)**: Hanoi Kulesi'ni en hızlı sen çöz! Ülkelere göre küresel sıralama.
- **Full description**:
```
🏆 Hanoi Kulesi — artık bir speedrun.

Bu zamana karşı bulmacada dünyanın dört bir yanından oyuncularla yarış. Diskleri en az hamleyle VE en hızlı şekilde taşıyarak rekorunu kır.

■ Özellikler
· 3'ten 10 diske kadar — yeni başlayandan ustaya
· RPM göstergesi — hızını gerçek zamanlı ölçer
· Ülkelere göre küresel sıralama — bayrağını dünya sıralamasına dik
· Hayalet düellosu — bir top oyuncunun oyununu izle ve hayaletini kovala
· Chiptune müzik + hızına tepki veren gökkuşağı ekolayzır
· Trafik ışıklı geri sayım başlangıcı — gerçek bir yarışın gerilimi
· İlk 10 yorumu — izini yalnızca şampiyonlar bırakır

■ Şunlar için ideal
· Hem zeka bulmacası HEM speedrun sevenler
· Küresel sıralamada yükselmeyi sevenler
· Kısa ve yoğun turlar isteyenler

Ücretsiz oyna. Zirve seni bekliyor.
```

**ASO 키워드**
- 코어: `hanoi kulesi`, `hanoi` · 장르: `bulmaca`, `zeka oyunu`, `mantık oyunu`, `puzzle`, `beyin` · 차별화: `speedrun`, `zamana karşı`, `sıralama`, `rekor`, `küresel` · 롱테일: `hanoi kulesi oyunu`, `disk bulmacası`, `beyin egzersizi`
- 이름+부제 이미 포함(→ **제외**): `hanoi`, `kulesi`, `speedrun`, `yarış`, `sıralama`
- **iOS 키워드필드**:
```
bulmaca,zeka,mantık,beyin,disk,rekor,puzzle,küresel,oyun,hız,strateji,eğitici,iq,brain,tower
```

---

## 🇻🇳 Tiếng Việt (vi)

- **Title (30)**: Tháp Hà Nội - Speedrun   (22자)
- **Subtitle (30)**: Giải đố tính giờ · xếp hạng
- **Short description (80)**: Giải Tháp Hà Nội với tốc độ nhanh nhất! Bảng xếp hạng toàn cầu theo quốc gia.
- **Full description**:
```
🏆 Tháp Hà Nội — giờ đây là một speedrun.

Tranh tài với người chơi khắp thế giới trong câu đố tính giờ này. Di chuyển các đĩa với số bước ít nhất VÀ nhanh nhất để lập kỷ lục của bạn.

■ Tính năng
· Từ 3 đến 10 đĩa — từ người mới đến cao thủ
· Đồng hồ RPM — đo tốc độ của bạn theo thời gian thực
· Bảng xếp hạng toàn cầu theo quốc gia — cắm cờ của bạn lên bảng xếp hạng thế giới
· Đấu với bóng ma — xem lại màn chơi của top player và đuổi theo bóng ma
· Nhạc chiptune + equalizer cầu vồng phản ứng theo tốc độ của bạn
· Xuất phát đèn giao thông đếm ngược — căng thẳng như một cuộc đua thật
· Bình luận Top 10 — chỉ nhà vô địch mới để lại dấu ấn

■ Hoàn hảo cho
· Người mê câu đố trí tuệ VÀ speedrun
· Người thích leo lên bảng xếp hạng toàn cầu
· Ai muốn những ván chơi ngắn và căng thẳng

Chơi miễn phí. Đỉnh cao đang chờ bạn.
```

**ASO 키워드**
- 코어: `tháp hà nội`, `hà nội` · 장르: `giải đố`, `câu đố`, `trò chơi trí tuệ`, `puzzle`, `trò chơi logic` · 차별화: `speedrun`, `tính giờ`, `xếp hạng`, `kỷ lục`, `toàn cầu` · 롱테일: `trò chơi tháp hà nội`, `giải đố đĩa`, `luyện não`
- 이름+부제 이미 포함(→ **제외**): `tháp`, `hà nội`, `speedrun`, `xếp hạng`, `giờ`
- **iOS 키워드필드**:
```
giải đố,câu đố,trí tuệ,logic,đĩa,kỷ lục,puzzle,toàn cầu,trò chơi,luyện não,tốc độ,iq,brain,tower
```

---

## 🇹🇭 ภาษาไทย (th)

> 태국어는 **별도 스크립트**(타이 문자) — 텍스트 필드는 자동 처리되나 RTL은 아니라 레이아웃 뒤집기는 불필요. 스크린샷 캡션 현지화는 별도 권장.

- **Title (30)**: หอคอยฮานอย - สปีดรัน   (≈20자)
- **Subtitle (30)**: ปริศนาจับเวลา · จัดอันดับโลก
- **Short description (80)**: ไขหอคอยฮานอยด้วยความเร็วสูงสุด! จัดอันดับโลกแยกตามประเทศ
- **Full description**:
```
🏆 หอคอยฮานอย — ตอนนี้คือสปีดรัน

แข่งกับผู้เล่นทั่วโลกในเกมปริศนาจับเวลานี้ ย้ายจานให้น้อยครั้งที่สุด และเร็วที่สุด เพื่อทำสถิติของคุณ

■ ฟีเจอร์
· ตั้งแต่ 3 ถึง 10 จาน — จากมือใหม่ถึงเซียน
· มาตรวัด RPM — วัดความเร็วมือของคุณแบบเรียลไทม์
· จัดอันดับโลกแยกตามประเทศ — ปักธงของคุณบนกระดานอันดับโลก
· ดวลกับผี — เล่นซ้ำเกมของนักแข่งอันดับต้นและไล่ตามผีของเขา
· เพลงชิปทูน + อีควอไลเซอร์สีรุ้งที่ตอบสนองต่อความเร็ว
· เริ่มด้วยไฟจราจรนับถอยหลัง — ตื่นเต้นเหมือนการแข่งจริง
· ความเห็น Top 10 — มีเพียงแชมป์ที่ทิ้งร่องรอยไว้

■ เหมาะสำหรับ
· คนที่รักทั้งปริศนาฝึกสมองและสปีดรัน
· คนที่ชอบไต่อันดับโลก
· ใครก็ตามที่อยากเล่นสั้นๆ แต่เข้มข้น

เล่นฟรี ยอดเขากำลังรอคุณอยู่
```

**ASO 키워드**
- 코어: `หอคอยฮานอย`, `ฮานอย` · 장르: `ปริศนา`, `เกมปริศนา`, `เกมฝึกสมอง`, `ตรรกะ`, `พัซเซิล` · 차별화: `สปีดรัน`, `จับเวลา`, `จัดอันดับ`, `สถิติ`, `ระดับโลก` · 롱테일: `เกมหอคอยฮานอย`, `ปริศนาจาน`, `ฝึกสมอง`
- 이름+부제 이미 포함(→ **제외**): `หอคอย`, `ฮานอย`, `สปีดรัน`, `จัดอันดับ`, `เวลา`
- **iOS 키워드필드**:
```
ปริศนา,ตรรกะ,ฝึกสมอง,สมอง,จาน,สถิติ,พัซเซิล,ระดับโลก,เกม,ความเร็ว,กลยุทธ์,ท้าทาย,iq,brain,puzzle
```

---

## 🇮🇹 Italiano (it)

- **Title (30)**: Torre di Hanoi - Speedrun   (25자)
- **Subtitle (30)**: Puzzle a tempo e classifica
- **Short description (80)**: Risolvi la Torre di Hanoi a tutta velocità! Classifica mondiale per Paese.
- **Full description**:
```
🏆 La Torre di Hanoi — ora è uno speedrun.

Sfida giocatori di tutto il mondo in questo puzzle contro il tempo. Sposta i dischi nel minor numero di mosse E nel minor tempo possibile per stabilire il tuo record.

■ Funzionalità
· Da 3 a 10 dischi — dal principiante all'esperto
· Contatore RPM — misura la tua velocità in tempo reale
· Classifiche mondiali per Paese — issa la tua bandiera nella classifica mondiale
· Duello col fantasma — rigioca la partita di un top player e insegui il suo fantasma
· Colonna sonora chiptune + equalizzatore arcobaleno che reagisce alla tua velocità
· Partenza col semaforo e conto alla rovescia — la tensione di una vera gara
· Commenti della Top 10 — solo i campioni lasciano il segno

■ Perfetto per
· Chi ama i rompicapo E lo speedrunning
· Chi adora scalare le classifiche mondiali
· Chi cerca partite brevi e intense

Gioca gratis. La vetta ti aspetta.
```

**ASO 키워드**
- 코어: `torre di hanoi`, `hanoi` · 장르: `puzzle`, `rompicapo`, `gioco di logica`, `rompicapi`, `gioco mentale` · 차별화: `speedrun`, `contro il tempo`, `classifica`, `record`, `mondiale` · 롱테일: `gioco torre di hanoi`, `rompicapo dischi`, `allenamento mentale`
- 이름+부제 이미 포함(→ **제외**): `torre`, `hanoi`, `speedrun`, `puzzle`, `classifica`
- **iOS 키워드필드**:
```
rompicapo,logica,ragionamento,mente,dischi,record,puzzle,mondiale,gioco,veloce,sfida,iq,brain,tower
```

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
✅ **완료**: 힌디(hi) · 인도네시아어(id) · 프랑스어(fr) · 독일어(de) · 러시아어(ru) · 터키어(tr) · 베트남어(vi) · 태국어(th) · 이탈리아어(it)
→ 인도는 영어 위주 필드가 정답이라 데바나가리 제목 + 영어 키워드필드로 처리함.
→ 태국어는 별도 스크립트(타이 문자)지만 RTL 아님 → 텍스트 필드 자동 처리, 스크린샷 캡션만 별도 권장.
→ 아랍어(ar)는 현지화 대상에서 제외 (RTL·별도 스크립트라 스크린샷까지 병행 현지화 필요 → 비용 대비 우선순위 낮음).
