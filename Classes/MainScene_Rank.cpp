#include "stdafx.h"
#include <algorithm>
#include <sstream>
#include "MainScene.h"
#include "SoundFactory.h"
#include "PlayScene.h"
#include "UserDataManager.h"
#include "LeaderboardManager.h"
#include "ui/CocosGUI.h"
#include "DrawUtils.h"
#include "PixelFont.h"
#ifdef LITE_VER
#include "IAPManager.h"
#endif //LITE_VER

#include "MainSceneInternal.h"



void MainScene::callbackRankPrev(Ref* pSender)
{
	if (m_rankLevel <= 3) return;
	SoundFactory::Instance()->play("efs_click");
	--m_rankLevel;
	int lv = m_rankLevel;
	scheduleOnce([this, lv](float) { drawOnlineRank(lv); }, 0.0f, "rankPrev");
}

void MainScene::callbackRankNext(Ref* pSender)
{
	if (m_rankLevel >= MAX_PLAY_LEVEL) return;
	SoundFactory::Instance()->play("efs_click");
	++m_rankLevel;
	int lv = m_rankLevel;
	scheduleOnce([this, lv](float) { drawOnlineRank(lv); }, 0.0f, "rankNext");
}

void MainScene::drawOnlineRank(int level, bool retryOnEmpty)
{
	this->unschedule("rankRetry");

	const float PX = 188, PY = 22;   // 좌우 여백 균형 (좌 ~9px ↔ 우 ~8px)
	const float PW = 284, PH = 200;
	const float HDR_H = 34;
	const float DATA_H = PH - HDR_H;

	// 패널 내부 가변 요소 태그
	const int TAG_D_NAV   = 151;   // 내비 메뉴 (◀▶)
	const int TAG_D_TITLE = 152;   // LEVEL N 타이틀
	const int TAG_D_ROWS  = 153;   // 데이터 행 컨테이너 (LED 전환 대상)

	++m_rankGeneration;
	int  gen   = m_rankGeneration;
	auto alive = m_aliveFlag;

	// ── 헤더 내 가변 요소 빌드 헬퍼 (공통: 첫 진입 / 재진입 모두 호출) ──
	auto buildHeader = [=](Node* panel) {
		float hdrMidY = DATA_H + HDR_H / 2.f;

		panel->removeChildByTag(TAG_D_TITLE);
		auto title = Label::createWithSystemFont(
			StringUtils::format("LEVEL  %d", level), "Arial", 14);
		title->setColor(Color3B(255, 215, 0));
		title->setAnchorPoint(Vec2(0.5f, 0.5f));
		title->setPosition(Vec2(PW / 2, hdrMidY + 5));
		panel->addChild(title, 4, TAG_D_TITLE);

		panel->removeChildByTag(TAG_D_NAV);
		bool canPrev = (level > 3), canNext = (level < MAX_PLAY_LEVEL);
		const float BTN_W = 22, BTN_H = 22;
		auto pn = makeVecNavKeycap(BTN_W, BTN_H, true,  canPrev);
		auto pb = MenuItemLabel::create(pn, CC_CALLBACK_1(MainScene::callbackRankPrev, this));
		pb->setEnabled(canPrev); pb->setPosition(Vec2(55, hdrMidY));
		auto nn = makeVecNavKeycap(BTN_W, BTN_H, false, canNext);
		auto nb = MenuItemLabel::create(nn, CC_CALLBACK_1(MainScene::callbackRankNext, this));
		nb->setEnabled(canNext); nb->setPosition(Vec2(PW - 55, hdrMidY));
		auto nav = Menu::create(pb, nb, nullptr);
		nav->setPosition(Vec2::ZERO);
		panel->addChild(nav, 4, TAG_D_NAV);
	};

	// ── 데이터 행 빌드 헬퍼 (콜백 안에서 호출) ──
	// ledScan=true 이면 행별 FadeIn 스캔 효과 적용
	auto buildRows = [=](Node* panel, const std::vector<LeaderboardEntry>& entries, bool ledScan,
	                     const std::map<std::string, std::string>& battleByLoser) {
		panel->removeChildByTag(TAG_D_ROWS);
		auto rowsNode = Node::create();

		if (entries.empty()) {
			const char* msg = retryOnEmpty ? "Refreshing..." : "No records yet";
			auto lbl = Label::createWithSystemFont(msg, "Arial", 12);
			lbl->setColor(Color3B(180, 180, 180));
			lbl->setAnchorPoint(Vec2(0.5f, 0.5f));
			lbl->setPosition(Vec2(PW / 2, DATA_H / 2 - 10));
			if (ledScan) { lbl->setOpacity(0); lbl->runAction(FadeIn::create(0.15f)); }
			rowsNode->addChild(lbl, 2);
			panel->addChild(rowsNode, 2, TAG_D_ROWS);
			if (retryOnEmpty) {
				this->scheduleOnce([this, alive, level](float) {
					if (!alive || !*alive) return;
					drawOnlineRank(level, false);
				}, 2.0f, "rankRetry");
			}
			return;
		}

		const std::string myId = LeaderboardManager::Instance()->getPlayFabId();
		const float FIRST_ROW_Y = DATA_H - 30, ROW_STEP = 14;

		// 격파/피격 낙인용 id→국가코드 맵 (로드된 행 기준 — 상대 깃발 해소, battle_reward)
		std::map<std::string, std::string> idCountry;
		for (const auto& en : entries) idCountry[en.playFabId] = en.countryCode;

		for (int i = 0; i < (int)entries.size(); ++i) {
			const auto& e  = entries[i];
			RecordTime   rt = getRecordTime(e.scoreMs);
			float y         = FIRST_ROW_Y - i * ROW_STEP;
			bool  isMe      = !myId.empty() && (e.playFabId == myId);
			Color3B rowCol  = isMe ? Color3B(255, 215, 0) : Color3B::WHITE;
			int rowFont     = isMe ? 13 : 12;
			float scanDelay = ledScan ? (0.07f + i * 0.035f) : 0.0f;

			auto addLbl = [&](Label* lbl) {
				if (ledScan) {
					lbl->setOpacity(0);
					lbl->runAction(Sequence::create(
						DelayTime::create(scanDelay),
						FadeIn::create(0.06f), nullptr));
				}
				rowsNode->addChild(lbl, 2);
			};

			auto rnk = Label::createWithSystemFont(
				StringUtils::format("%d", e.rank), "Arial", rowFont);
			rnk->setAnchorPoint(Vec2(0, 0.5f)); rnk->setPosition(Vec2(12, y));
			rnk->setColor(rowCol); addLbl(rnk);

			if (!e.countryCode.empty()) {
				auto flg = Label::createWithSystemFont(
					countryToFlag(e.countryCode), "Arial", rowFont + 1);
				flg->setAnchorPoint(Vec2(0, 0.5f)); flg->setPosition(Vec2(32, y));
				addLbl(flg);
			}

			auto nm = Label::createWithSystemFont(
				utf8TruncateCP(e.displayName, 10), "Arial", rowFont);  // 바이트 아닌 코드포인트로 자름 (한글 깨짐 방지)
			nm->setAnchorPoint(Vec2(0, 0.5f)); nm->setPosition(Vec2(55, y));
			nm->setColor(rowCol); addLbl(nm);

#ifdef ENABLE_REPLAY_LIKE
			// 👍N — 수상소감 우측에 배치(N≥1만). 라벨을 먼저 만들어 폭만 재두고(공간 예약),
			// 실제 배치는 소감 렌더 뒤에서(아래). 소감이 길면 소감을 줄여 좋아요 자리를 확보(replay_like).
			Label* lkLabel = nullptr;
			float  lkReserve = 0.f;
			const float kLikeGap = 5.f;
			float  likeAtX = 55.f + nm->getContentSize().width + 6.f;  // 소감 없을 때: 이름 우측
			if (e.likes > 0 && LeaderboardManager::Instance()->isLikeEnabled()) {
				lkLabel = Label::createWithSystemFont(
					"\xF0\x9F\x91\x8D" + formatLikeCount(e.likes), "Arial", rowFont - 2);
				lkLabel->setColor(Color3B(255, 190, 80));   // 앰버
				lkLabel->setAnchorPoint(Vec2(0, 0.5f));
				lkReserve = kLikeGap + lkLabel->getContentSize().width;
			}
#endif

			auto tm = Label::createWithSystemFont(
				StringUtils::format("%02d:%02d.%02d", rt.min, rt.sec, rt.ms), "Arial", rowFont);
			tm->setAnchorPoint(Vec2(1.0f, 0.5f)); tm->setPosition(Vec2(PW - 10, y));
			tm->setColor(rowCol); addLbl(tm);

			// ── 격파/피격 낙인 마커(battle_reward): 시간 왼쪽에 [방향 글리프 + 상대 국가 깃발] ──
			//    피격(💥, 나를 이긴 상대) 우선, 없으면 격파(🗡, 내가 꺾은 상대). 상대 깃발은 id맵 해소.
			Label* bMark = nullptr;
			bool   bDefeated = false;   // 피격(내가 짐) 여부 — 내 행이면 마커를 눈에 띄게 깜빡여 복수(탭) 유도
			{
				std::string bGlyph, bOppId;  Color3B bTint(255, 255, 255);
				auto itL = battleByLoser.find(e.playFabId);
				if (itL != battleByLoser.end()) {          // 피격: 나를 이긴 상대(by)
					bGlyph = "\xF0\x9F\x92\xA5";           // 💥
					bOppId = itL->second;
					bTint  = Color3B(255, 100, 100);
					bDefeated = true;
				} else {                                   // 격파: 내가 꺾은 상대(loser)
					for (const auto& kv : battleByLoser)
						if (kv.second == e.playFabId) { bOppId = kv.first; break; }
					if (!bOppId.empty()) { bGlyph = "\xF0\x9F\x97\xA1"; bTint = Color3B(255, 215, 0); }  // 🗡
				}
				if (!bGlyph.empty()) {
					std::string bFlag;
					auto itc = idCountry.find(bOppId);
					if (itc != idCountry.end() && !itc->second.empty()) bFlag = countryToFlag(itc->second);
					bMark = Label::createWithSystemFont(bGlyph + bFlag, "Arial", rowFont - 1);
					bMark->setColor(bTint);
					bMark->setAnchorPoint(Vec2(1.0f, 0.5f));
					bMark->setPosition(Vec2((PW - 10.f) - tm->getContentSize().width - 4.f, y));
					addLbl(bMark);
				}
			}

#ifdef ENABLE_AWARD_COMMENT
			// ── 수상소감: 이름~시간 사이. 다 들어가면 전문, 초과 시 코드포인트 말줄임 ──
			{
				float nameRight = 55.f + nm->getContentSize().width;
				float timeLeft  = (PW - 10.f) - tm->getContentSize().width;
				if (bMark) timeLeft -= (bMark->getContentSize().width + 4.f);  // 낙인 마커 공간 확보
				float availL    = nameRight + 6.f;
				float availW    = timeLeft - 6.f - availL;
#ifdef ENABLE_REPLAY_LIKE
				availW -= lkReserve;   // 좋아요 라벨 공간 예약 — 소감이 길면 소감을 줄인다
#endif
				if (availW > 12.f) {
					if (!e.comment.empty()) {
						auto cmt = Label::createWithSystemFont(e.comment, "Arial", rowFont - 1);
						cmt->setAnchorPoint(Vec2(0, 0.5f));
						cmt->setColor(isMe ? Color3B(255, 235, 150) : Color3B(150, 200, 180));
						if (cmt->getContentSize().width > availW) {
							int cp = LeaderboardManager::utf8Length(e.comment);
							while (cp > 1 && cmt->getContentSize().width > availW) {
								--cp;
								cmt->setString(utf8TruncateCP(e.comment, cp) + "...");
							}
						}
						cmt->setPosition(Vec2(availL, y));
						addLbl(cmt);
#ifdef ENABLE_REPLAY_LIKE
						likeAtX = availL + cmt->getContentSize().width + kLikeGap;  // 소감 우측
#endif
					} else if (isMe && LeaderboardManager::Instance()->isAwardEnabled()) {
						auto pen = Label::createWithSystemFont("✎", "Arial", rowFont);  // ✎ 미작성 힌트
						pen->setAnchorPoint(Vec2(0, 0.5f));
						pen->setColor(Color3B(255, 215, 0));
						pen->setPosition(Vec2(availL, y));
						addLbl(pen);
#ifdef ENABLE_REPLAY_LIKE
						likeAtX = availL + pen->getContentSize().width + kLikeGap;
#endif
					}
				}
			}
#endif // ENABLE_AWARD_COMMENT

#ifdef ENABLE_REPLAY_LIKE
			// 👍N 최종 배치 — 수상소감(또는 이름) 우측
			if (lkLabel) {
				lkLabel->setPosition(Vec2(likeAtX, y));
				addLbl(lkLabel);
			}
#endif

			if (isMe) {
				float blinkDelay = ledScan ? (scanDelay + 0.06f) : 0.0f;
				auto makeBlink = [blinkDelay](Label* lbl) {
					auto doBlink = CallFunc::create([lbl]() {
						lbl->runAction(RepeatForever::create(Sequence::create(
							DelayTime::create(0.4f),
							FadeTo::create(0.1f, 60),
							DelayTime::create(0.4f),
							FadeTo::create(0.1f, 255),
							nullptr)));
					});
					lbl->runAction(Sequence::create(
						DelayTime::create(blinkDelay),
						doBlink,
						nullptr));
				};
				makeBlink(rnk);
				makeBlink(nm);
				makeBlink(tm);
				// 피격 낙인(💥)은 복수 유도용 → 행 펄스보다 더 눈에 띄게(투명도+살짝 확대) 별도 깜빡임.
				if (bMark && bDefeated) {
					bMark->setOpacity(255);
					auto startPulse = CallFunc::create([bMark]() {
						bMark->runAction(RepeatForever::create(Sequence::create(
							Spawn::create(FadeTo::create(0.3f, 110), ScaleTo::create(0.3f, 1.18f), nullptr),
							Spawn::create(FadeTo::create(0.3f, 255), ScaleTo::create(0.3f, 1.0f),  nullptr),
							nullptr)));
					});
					bMark->runAction(Sequence::create(
						DelayTime::create(blinkDelay), startPulse, nullptr));
				}
			}
		}

#ifdef ENABLE_AWARD_COMMENT
		// 행 탭 → 소감/리플레이 카드 (소감 or 리플레이 보유 행) / 내 미작성 행 탭 → 입력창
		{
			auto entriesCopy = entries;

			// 2차: 이 레벨 리플레이 보유자 프리워밍 → 탭 게이트 + 보유 행 좌측에 ▶ 마커(관전 가능 표시)
			auto replayOwners = std::make_shared<std::set<std::string>>();
			if (level <= LeaderboardManager::REPLAY_MAX_LEVEL) {
				auto alive = m_aliveFlag;
				LeaderboardManager::Instance()->fetchReplays(level,
					[replayOwners, alive, entriesCopy, rowsNode, FIRST_ROW_Y, ROW_STEP](const std::map<std::string, std::string>& m) {
						if (!alive || !*alive) return;
						for (auto& kv : m) replayOwners->insert(kv.first);
						if (!rowsNode || !rowsNode->getParent()) return;   // 보드 교체됨 → 마커 스킵
						for (int i = 0; i < (int)entriesCopy.size(); ++i) {
							if (!replayOwners->count(entriesCopy[i].playFabId)) continue;
							float y = FIRST_ROW_Y - i * ROW_STEP;
							auto mark = Label::createWithSystemFont("\xE2\x96\xB6", "Arial", 8);  // ▶
							mark->setColor(Color3B(120, 200, 255));
							mark->setAnchorPoint(Vec2(0.5f, 0.5f));
							mark->setPosition(Vec2(6, y));
							mark->setOpacity(0);
							mark->runAction(FadeIn::create(0.2f));
							rowsNode->addChild(mark, 3);
						}
					});
			}

			auto touchLs = EventListenerTouchOneByOne::create();
			touchLs->setSwallowTouches(true);
			touchLs->onTouchBegan =
				[this, entriesCopy, replayOwners, battleByLoser, rowsNode, FIRST_ROW_Y, ROW_STEP, PW, level](Touch* t, Event*) -> bool {
					Vec2 p = rowsNode->convertToNodeSpace(t->getLocation());
					if (p.x < 10 || p.x > PW - 10) return false;
					for (int i = 0; i < (int)entriesCopy.size(); ++i) {
						float ry = FIRST_ROW_Y - i * ROW_STEP;
						if (p.y >= ry - ROW_STEP / 2 && p.y <= ry + ROW_STEP / 2) {
							const auto& e = entriesCopy[i];
							std::string myId = LeaderboardManager::Instance()->getPlayFabId();
							bool isMe      = !myId.empty() && e.playFabId == myId;
							bool hasReplay = replayOwners->count(e.playFabId) > 0;
							// 복수(battle_reward): 내 행이 피격 상태면 최우선 복수 창.
							if (isMe) {
								auto itB = battleByLoser.find(e.playFabId);
								if (itB != battleByLoser.end()) {
									const std::string& aId = itB->second;
									for (const auto& en : entriesCopy) {
										if (en.playFabId == aId) {
											showRevengeDialog(level, aId, en.displayName, en.rank, en.scoreMs);
											return true;
										}
									}
									// A가 현재 보드에 없음(GC 지연 등) → 아래 기존 흐름으로 폴백
								}
							}
							if (!e.comment.empty() || hasReplay) { showAwardCardDialog(e, level); return true; }
							if (isMe && LeaderboardManager::Instance()->isAwardEnabled()) {
								showAwardInputDialog(level, e.rank, e.comment); return true;
							}
							return false;
						}
					}
					return false;
				};
			Director::getInstance()->getEventDispatcher()
				->addEventListenerWithSceneGraphPriority(touchLs, rowsNode);
		}
#endif // ENABLE_AWARD_COMMENT

		panel->addChild(rowsNode, 2, TAG_D_ROWS);
	};

	// ══════════════════════════════════════════════════════════════════
	// [A] 재진입 경로: 패널 프레임 유지, 내용만 LED 전환
	// ══════════════════════════════════════════════════════════════════
	if (m_rankBG) {
		auto* panel = m_rankBG;

		// 기존 행 LED 페이드 아웃 후 제거
		if (auto* old = panel->getChildByTag(TAG_D_ROWS)) {
			old->runAction(Sequence::create(
				FadeOut::create(0.10f),
				RemoveSelf::create(), nullptr));
		}

		// 헤더 즉시 업데이트 (레벨/내비)
		buildHeader(panel);

		// 새 데이터 비동기 fetch → 격파 낙인 조인 → LED 스캔 인
		LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
			[this, alive, gen, level, panel, buildRows](const std::vector<LeaderboardEntry>& entries) {
				if (!alive || !*alive || gen != m_rankGeneration) return;
				auto entriesCopy = entries;
				LeaderboardManager::Instance()->fetchBattles(level,
					[this, alive, gen, level, panel, buildRows, entriesCopy](const std::map<std::string, std::string>& bl) {
						if (!alive || !*alive || gen != m_rankGeneration) return;
						
#ifdef ENABLE_REPLAY_LIKE
						{
							auto blCopy = bl;
							LeaderboardManager::Instance()->fetchLikes(level,
								[this, alive, gen, panel, buildRows, entriesCopy, blCopy]
								(const std::map<std::string, int>& likes) mutable {
									if (!alive || !*alive || gen != m_rankGeneration) return;
									for (auto& e : entriesCopy) {
										auto it = likes.find(e.playFabId);
										if (it != likes.end()) e.likes = it->second;
									}
									buildRows(panel, entriesCopy, true, blCopy);   // ledScan = true
								});
						}
#else
						buildRows(panel, entriesCopy, true, bl);   // ledScan = true
#endif
					});
			});

		return;
	}

	// ══════════════════════════════════════════════════════════════════
	// [B] 첫 진입: 로딩 표시 → 데이터 수신 후 패널 슬라이드 인
	// ══════════════════════════════════════════════════════════════════
	const int TAG_LOADING = 50;
	auto loading = Label::createWithSystemFont("Loading...", "Arial", 14);
	loading->setColor(Color3B(180, 180, 180));
	loading->setPosition(Vec2(PX + PW / 2, PY + PH / 2));
	m_rankTableLayer->addChild(loading, 0, TAG_LOADING);

	LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
		[this, alive, gen, level, PX, PY, PW, PH, HDR_H, DATA_H,
		 buildHeader, buildRows, TAG_LOADING]
		(const std::vector<LeaderboardEntry>& entries) {
			if (!alive || !*alive || gen != m_rankGeneration) return;

			m_rankTableLayer->removeChildByTag(TAG_LOADING);
			m_rankTableLayer->removeChildByTag(tagBG);

			// 패널 생성 + 슬라이드 인
			auto panel = LayerColor::create(Color4B(0, 0, 0, 0), PW, PH);
			panel->setPosition(Vec2(RESOURCE_WIDTH + PW, PY));
			panel->runAction(MoveTo::create(0.3f, Vec2(PX, PY)));
			m_rankBG = panel;
			m_rankTableLayer->addChild(panel, tagBGRankingBoard, tagBGRankingBoard);

			// 영구 구조 요소
			auto dataBg = DrawNode::create();
			setupLedBackground(dataBg, PW, DATA_H, 0.f);
			dataBg->setPosition(Vec2(PW / 2, DATA_H / 2));
			panel->addChild(dataBg, 0);

			auto dataDots = DrawNode::create();
			setupLedDotOverlay(dataDots, PW, DATA_H, 3.0f, 0.5f);
			dataDots->setPosition(Vec2(PW / 2, DATA_H / 2));
			panel->addChild(dataDots, 1);

			auto headerBg = DrawNode::create();
			headerBg->drawSolidRect(Vec2(0, DATA_H), Vec2(PW, PH),
				Color4F(0.05f, 0.08f, 0.20f, 1.0f));
			panel->addChild(headerBg, 0);

			auto border = DrawNode::create();
			border->drawRect(Vec2(0, 0), Vec2(PW, PH),
				Color4F(0.31f, 0.86f, 0.70f, 0.85f));
			border->drawLine(Vec2(0, DATA_H), Vec2(PW, DATA_H),
				Color4F(0.31f, 0.86f, 0.70f, 0.85f));
			panel->addChild(border, 6);

			float hdrMidY = DATA_H + HDR_H / 2.f;
			auto subTitle = Label::createWithSystemFont("ONLINE  RANK", "Arial", 8);
			subTitle->setColor(Color3B(160, 160, 200));
			subTitle->setAnchorPoint(Vec2(0.5f, 0.5f));
			subTitle->setPosition(Vec2(PW / 2, hdrMidY - 8));
			panel->addChild(subTitle, 4);

			// 영구 컬럼 헤더 + 구분선 (레벨이 바뀌어도 변하지 않음)
			float colHdrY = DATA_H - 12;
			auto addColHdr = [&](const char* t, float x, bool ra) {
				auto lbl = Label::createWithSystemFont(t, "Arial", 9);
				lbl->setColor(Color3B(130, 130, 150));
				lbl->setAnchorPoint(ra ? Vec2(1.0f, 0.5f) : Vec2(0, 0.5f));
				lbl->setPosition(Vec2(x, colHdrY));
				panel->addChild(lbl, 2);
			};
			addColHdr("#",    12,      false);
			addColHdr("NAME", 50,      false);
			addColHdr("TIME", PW - 10, true);

			auto colDiv = DrawNode::create();
			colDiv->drawLine(Vec2(10, DATA_H - 20), Vec2(PW - 10, DATA_H - 20),
				Color4F(0.3f, 0.5f, 0.5f, 0.5f));
			panel->addChild(colDiv, 2);

			// 헤더 가변 요소 (레벨 타이틀 + 내비)
			buildHeader(panel);

			// 데이터 행 (첫 진입은 스캔 없이 바로 표시) — 격파 낙인 조인 후 렌더
			auto entriesCopy = entries;
			LeaderboardManager::Instance()->fetchBattles(level,
				[this, alive, gen, level, panel, buildRows, entriesCopy](const std::map<std::string, std::string>& bl) {
					if (!alive || !*alive || gen != m_rankGeneration) return;
					
#ifdef ENABLE_REPLAY_LIKE
						{
							auto blCopy = bl;
							LeaderboardManager::Instance()->fetchLikes(level,
								[this, alive, gen, panel, buildRows, entriesCopy, blCopy]
								(const std::map<std::string, int>& likes) mutable {
									if (!alive || !*alive || gen != m_rankGeneration) return;
									for (auto& e : entriesCopy) {
										auto it = likes.find(e.playFabId);
										if (it != likes.end()) e.likes = it->second;
									}
									buildRows(panel, entriesCopy, false, blCopy);
								});
						}
#else
						buildRows(panel, entriesCopy, false, bl);
#endif
				});
		});
}
