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


// ────────────────────────────────────────────────────────────────────────────
// drawOnlineRank  ─  첫 진입: 패널 슬라이드 인 / 재진입(◀▶): LED 내용 전환
// ────────────────────────────────────────────────────────────────────────────
// UTF-8 문자열을 최대 maxCP 코드포인트까지 자른다 (경계 유지). 이름/소감 truncate 공용.
std::string utf8TruncateCP(const std::string& s, int maxCP)
{
	int cp = 0; size_t i = 0;
	while (i < s.size() && cp < maxCP) {
		unsigned char c = (unsigned char)s[i];
		size_t adv = 1;
		if      (c >= 0xF0) adv = 4;
		else if (c >= 0xE0) adv = 3;
		else if (c >= 0xC0) adv = 2;
		i += adv; ++cp;
	}
	if (i > s.size()) i = s.size();
	return s.substr(0, i);
}

#ifdef ENABLE_AWARD_COMMENT
// writeAwardComment/clientFilterComment reason → 영어 안내
static std::string awardReasonMessage(const std::string& r)
{
	if (r == "empty")     return "Please enter a message";
	if (r == "too_long")  return StringUtils::format("Max %d characters", LeaderboardManager::AWARD_MAX_CP);
	if (r == "link")      return "Links are not allowed";
	if (r == "profanity") return "Inappropriate language";
	if (r == "network")   return "Network error, try again";
	if (r == "disabled")  return "Comments are currently unavailable";
	return "Failed, please try again";
}

// UGC 신고/문의 수신 이메일 (App Store 심사 1.2: 신고 수단 + 연락처)
static const char* AWARD_SUPPORT_EMAIL = "ozzywow2@gmail.com";

// mailto용 퍼센트 인코딩 (unreserved 문자만 통과)
static std::string urlEncode(const std::string& s)
{
	static const char* HEX = "0123456789ABCDEF";
	std::string out;
	out.reserve(s.size() * 3);
	for (unsigned char c : s) {
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
			out += (char)c;
		} else {
			out += '%';
			out += HEX[c >> 4];
			out += HEX[c & 0xF];
		}
	}
	return out;
}

// 문제 소감 신고 — 기본 메일 클라이언트로 사전 작성된 신고 메일 열기.
// 심사관이 요구하는 "신고 수단 + 개발자 연락처"를 버튼 하나로 동시 충족.
static void openAwardReportEmail(int level, int rank,
                                 const std::string& playFabId, const std::string& comment)
{
	std::string subject = StringUtils::format("[Hanoi] Report award comment (L%02d #%d)", level, rank);
	std::string body = StringUtils::format(
		"I'd like to report the following winning speech as inappropriate.\n\n"
		"Level: %d\nRank: %d\nUser ID: %s\nComment: \"%s\"\n\n"
		"Reason (please describe):\n",
		level, rank, playFabId.c_str(), comment.c_str());
	std::string url = std::string("mailto:") + AWARD_SUPPORT_EMAIL
		+ "?subject=" + urlEncode(subject)
		+ "&body="    + urlEncode(body);
	Application::getInstance()->openURL(url);
}
#endif // ENABLE_AWARD_COMMENT

// 플랫폼별 스토어 페이지 URL. ⚠️ iOS App Store ID / Android 패키지 배포 정보와 일치 확인 필요.
std::string appStoreUrl()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	return "https://play.google.com/store/apps/details?id=com.ozzywow.hanoi";
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	return "https://apps.apple.com/app/id430261581";
#else
	return BUY_AT_STORE_URL;
#endif
}

// 버전 라벨 갱신 — "current/latest"(v 없음) 숫자 + [Update] 버튼. current<latest면 버튼 표시/활성(깜빡임 없음).
// 최신 버전은 fetchTitleConfig 도착 후에만 유효 → 도착 전엔 current만, 도착 후 갱신 호출.
void MainScene::refreshVersionLabel()
{
	if (!m_versionLabel || !m_updateItem) return;

	// 화면 오른쪽 끝 우측정렬(약간 여백). 세로로 쌓음 — 버전 숫자 위, [Update] 아래. (앵커 (1,0.5))
	const float BASE_X = RESOURCE_WIDTH - 8.0f;     // 우측 끝 여백 8px
	const float BASE_Y = 280.0f;                    // 버전 숫자 높이
	const float BTN_Y  = BASE_Y - 15.0f;            // [Update] 버튼(바로 아래)
	std::string cur = Application::getInstance()->getVersion();
	const std::string& latest = LeaderboardManager::Instance()->getLatestVersion();
	bool haveBoth   = !cur.empty() && !latest.empty();
	bool hasUpdate  = haveBoth && LeaderboardManager::compareVersion(cur, latest) < 0;

	// 버전 숫자 (v 없음). 확인됐으면 current/latest, 미확인이면 current만.
	m_versionLabel->setString(haveBoth
		? StringUtils::format("ver %s/%s", cur.c_str(), latest.c_str())
		: cur);
	m_versionLabel->setColor(Color3B(185, 200, 220));   // 차분한 밝은 회청색
	m_versionLabel->setOpacity(220);
	m_versionLabel->setPosition(Vec2(BASE_X, BASE_Y));

	// [Update] 버튼 — current<latest일 때만 표시/활성. 버전 숫자 바로 아래.
	m_updateItem->setPosition(Vec2(BASE_X, BTN_Y));
	if (hasUpdate) {
		m_updateItem->setEnabled(true);
		m_updateItem->setVisible(true);
	} else {
		m_updateItem->setVisible(false);
		m_updateItem->setEnabled(false);
	}
}

// 최신 버전이 아직 없으면(콜드 스타트 로그인/네트워크 지연) 도착할 때까지 재조회 → 확보 순간 라벨 갱신.
// 첫 진입 fetchTitleConfig가 latest를 못 받고 끝나도(로그인 지연/씬 전환) 여기서 따라잡는다.
void MainScene::refreshVersionLabelWhenReady(int attempt)
{
	if (!m_versionLabel) return;
	// 이미 최신 버전 확보 → 갱신하고 종료.
	if (!LeaderboardManager::Instance()->getLatestVersion().empty()) {
		refreshVersionLabel();
		return;
	}
	// 로컬 버전 불명(win32 개발빌드 등)은 비교 불가 → 재시도 의미 없음.
	if (Application::getInstance()->getVersion().empty()) return;
	if (attempt >= 6) return;   // 상한(약 6회 × 1.5s ≈ 9초)

	auto alive = m_aliveFlag;
	// 잠시 후 재확인: 그 사이 도착했으면 갱신, 아니면 재조회 후 재귀 재시도.
	this->scheduleOnce([this, alive, attempt](float) {
		if (!alive || !*alive) return;
		if (!LeaderboardManager::Instance()->getLatestVersion().empty()) {
			refreshVersionLabel();                 // 그새 도착 → 재조회 없이 갱신
			return;
		}
		LeaderboardManager::Instance()->fetchTitleConfig([this, alive, attempt]() {
			if (!alive || !*alive) return;
			refreshVersionLabelWhenReady(attempt + 1);  // 도착이면 위에서 refresh+return, 아니면 재시도
		});
	}, 1.5f, "verRefreshRetry");
}

// 앱 버전 게이트 다이얼로그.
//   force=true  → 차단 모달. UPDATE 버튼만, 배경/바깥 탭으로 닫히지 않음. onClose 무시.
//   force=false → 권장 안내. UPDATE + LATER, 바깥 탭으로도 닫힘. 닫을 때 onClose 호출.
void MainScene::showUpdateDialog(bool force, std::function<void()> onClose)
{
	const int TAG = 194;
	if (this->getChildByTag(TAG)) return;   // 이미 떠 있으면 중복 생성 방지
	// 강제 업데이트는 다른 모달(이름 입력창 tag=199)보다 우선 — 닫아서 겹침/키보드 잔존 방지.
	// (EditBox 노드가 제거되면 iOS/Android 모두 소프트 키보드가 함께 닫힘)
	if (force) this->removeChildByTag(199);
	SoundFactory::Instance()->play("efs_click");

	// "다시 묻지 않기" 체크 상태(권장/소극 전용). 닫거나 UPDATE 시 체크돼 있으면 해당 버전 억제.
	auto optOut = std::make_shared<bool>(false);
	// 권장 창 닫기(LATER/바깥탭) — 1회만 실행하고 onClose 콜백 호출.
	auto closedGuard = std::make_shared<bool>(false);
	auto dismiss = [this, TAG, closedGuard, onClose, optOut, force]() {
		if (*closedGuard) return;
		*closedGuard = true;
		if (!force && *optOut) LeaderboardManager::Instance()->suppressOptionalUpdate();
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
		if (onClose) onClose();
	};

	// 체크박스 자리를 위해 권장/소극 창은 세로로 조금 더 크게.
	const float DW = 264, DH = force ? 150 : 176;

	// 공지 톤 = 골드 테두리·타이틀. 배경 탭 처리(closeLs)라 modal=false. 강제=딤 210, 헤더 title+msg라 divider=false. 키보드 아님이나 기존 y+40 유지.
	auto f = makePopupFrame(force ? "UPDATE REQUIRED" : "UPDATE AVAILABLE",
		kBorderGold, kTextTitle, DW, DH, 15.f, force ? kDimForce : kDimStd, false, false);
	f.backdrop->setTag(TAG);
	this->addChild(f.backdrop, 1000);
	auto dlg = f.box;
	dlg->setPositionY(dlg->getPositionY() + 40);

	// 모달 — 강제면 바깥 탭도 삼키기만(닫지 않음), 권장이면 바깥 탭 시 닫힘.
	auto closeLs = EventListenerTouchOneByOne::create();
	closeLs->setSwallowTouches(true);
	closeLs->onTouchBegan = [dlg, DW, DH, force, dismiss](Touch* t, Event*) -> bool {
		Vec2 p = dlg->convertToNodeSpace(t->getLocation());
		if (p.x >= 0 && p.x <= DW && p.y >= 0 && p.y <= DH) return true;  // 박스 내부
		if (force) return true;   // 강제 — 바깥 탭 무시(닫기 불가)
		dismiss();                // 권장 — 바깥 탭 시 닫기(+onClose)
		return true;
	};
	Director::getInstance()->getEventDispatcher()
		->addEventListenerWithSceneGraphPriority(closeLs, f.backdrop);

	auto msg = Label::createWithSystemFont(
		force ? "A new version is required to keep playing."
		      : "A new version is available.", "Arial", 11);
	msg->setColor(Color3B(200, 215, 230));
	msg->setDimensions(DW - 30, 0);
	msg->setHorizontalAlignment(TextHAlignment::CENTER);
	msg->setPosition(Vec2(DW / 2, DH - 66));
	dlg->addChild(msg);

	// 현재 → 최신 버전 표기
	std::string cur = Application::getInstance()->getVersion();
	std::string tgt = LeaderboardManager::Instance()->getLatestVersion();
	if (!cur.empty() && !tgt.empty()) {
		auto ver = Label::createWithSystemFont(
			StringUtils::format("v%s  ->  v%s", cur.c_str(), tgt.c_str()), "Arial", 10);
		ver->setColor(Color3B(150, 165, 185));
		ver->setPosition(Vec2(DW / 2, DH - 90));
		dlg->addChild(ver);
	}

	// UPDATE 버튼 (스토어로 이동). 체크돼 있으면 이동 전에 억제 저장.
	auto upBtn = makePopupChipButton("\xE2\x86\x91 UPDATE", kBtnFunc, [optOut, force](Ref*) {
		if (!force && *optOut) LeaderboardManager::Instance()->suppressOptionalUpdate();
		Application::getInstance()->openURL(appStoreUrl());
	}, force ? 110.f : 88.f, 34.f);

	Menu* menu = nullptr;
	if (force) {
		upBtn->setPosition(Vec2(DW / 2, 20));
		menu = Menu::create(upBtn, nullptr);
	} else {
		auto laterBtn = makePopupChipButton("LATER", kBtnDismiss, [dismiss](Ref*) {
			dismiss();
		}, 88.f, 34.f);
		upBtn->setPosition(Vec2(DW / 2 + 52, 20));
		laterBtn->setPosition(Vec2(DW / 2 - 52, 20));

		// "다시 묻지 않기" 체크박스 (탭 시 토글 + 라벨 갱신). [  ] / [X]
		auto cbLabel = Label::createWithSystemFont("[  ]  Don't ask again", "Arial", 11);
		cbLabel->setColor(Color3B(150, 165, 185));
		auto cbBtn = MenuItemLabel::create(cbLabel, [optOut, cbLabel](Ref*) {
			*optOut = !*optOut;
			cbLabel->setString(*optOut ? "[X]  Don't ask again" : "[  ]  Don't ask again");
			cbLabel->setColor(*optOut ? Color3B(255, 215, 0) : Color3B(150, 165, 185));
			SoundFactory::Instance()->play("efs_click");
		});
		cbBtn->setPosition(Vec2(DW / 2, 46));

		menu = Menu::create(upBtn, laterBtn, cbBtn, nullptr);
	}
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu, 5);
}

static std::vector<std::string>& getRandomNamePool() {
	static std::vector<std::string> pool;
	if (pool.empty()) {
		std::string content = FileUtils::getInstance()->getStringFromFile("random_names.txt");
		std::istringstream ss(content);
		std::string line;
		while (std::getline(ss, line)) {
			while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
				line.pop_back();
			// EditBox 제약(3~12자)에 맞는 이름만 채택 — 14자 CAMELOPARDALIS/NEBUCHADNEZZAR 등
			// 초과 이름은 자동 채움 시 too_long 유발하므로 풀에서 제외.
			if (line.size() >= 3 && line.size() <= 12)
				pool.push_back(line);
		}
		if (pool.empty()) {
			pool = {"NOVA","ACE","BLAZE","ECHO","PIXEL","SWIFT","STORM","VORTEX","FLASH","NEXUS"};
		}
	}
	return pool;
}

// 자동 추천 이름 풀은 큐레이션된 안전 목록(천문/신화/성경 이름)이므로 신뢰.
// SCULPTOR(별자리) 등이 서버 banned_words 부분일치로 오탐되는 걸 방지하려 욕설검증을 우회할 때 사용.
static bool isTrustedPoolName(const std::string& name) {
	size_t a = name.find_first_not_of(" \t\r\n");
	if (a == std::string::npos) return false;
	size_t b = name.find_last_not_of(" \t\r\n");
	std::string up = name.substr(a, b - a + 1);
	for (char& c : up) if (c >= 'a' && c <= 'z') c = (char)(c - 32);  // ASCII 대문자화
	for (const std::string& p : getRandomNamePool())
		if (p == up) return true;
	return false;
}

// 이름 입력 EditBox 델리게이트 — 엔터를 빈칸으로 누르면 랜덤 이름 자동 입력
class NameInputDelegate : public Ref, public cocos2d::ui::EditBoxDelegate {
public:
	cocos2d::ui::EditBox* box = nullptr;
	void editBoxEditingDidBegin(cocos2d::ui::EditBox*) override {}
	void editBoxEditingDidEndWithAction(cocos2d::ui::EditBox*, EditBoxEndAction) override {}
	void editBoxTextChanged(cocos2d::ui::EditBox*, const std::string&) override {}
	void editBoxReturn(cocos2d::ui::EditBox*) override {
		if (box && strlen(box->getText()) < 3) {
			auto& pool = getRandomNamePool();
			box->setText(pool[rand() % pool.size()].c_str());
		}
	}
};
static Ref* s_nameDelegate = nullptr;

void MainScene::showNameInputDialog(const std::string& prefill)
{
	SoundFactory::Instance()->play("efs_click");
	const int DIALOG_TAG = 199;
	const float DW = 250, DH = 158;   // 에디트박스 + 상태메시지 + OK 버튼 세로 여유 확보

	// 유틸 톤 = 그레이 테두리, 골드 타이틀. 키보드 공간 확보 위해 박스 y +40.
	auto f = makePopupFrame("SET YOUR NAME FOR RANKING", kBorderGray, kTextTitle, DW, DH, 14.f);
	f.backdrop->setTag(DIALOG_TAG);
	this->addChild(f.backdrop, 999);
	auto dlg = f.box;
	dlg->setPositionY(dlg->getPositionY() + 40);

	auto editBoxBg = DrawNode::create();
	editBoxBg->drawSolidRect(Vec2(8, 86), Vec2(DW - 8, 114),
		Color4F(0.05f, 0.07f, 0.15f, 0.9f));
	editBoxBg->drawRect(Vec2(8, 86), Vec2(DW - 8, 114),
		Color4F(0.4f, 0.4f, 0.6f, 0.7f));
	dlg->addChild(editBoxBg);

	auto editBox = cocos2d::ui::EditBox::create(Size(DW - 20, 24),
		cocos2d::ui::Scale9Sprite::create());
	editBox->setFont("Arial", 13);
	editBox->setFontColor(Color3B::WHITE);
	editBox->setPlaceholderFontColor(Color3B(180, 180, 180));
	editBox->setPlaceHolder("3-12 chars");
	editBox->setMaxLength(12);
	editBox->setInputMode(cocos2d::ui::EditBox::InputMode::SINGLE_LINE);
	editBox->setReturnType(cocos2d::ui::EditBox::KeyboardReturnType::DONE);
	editBox->setAnchorPoint(Vec2(0, 0.5f));
	editBox->setPosition(Vec2(10, 100));
	// ResetAll 등에서 마지막 사용 이름을 미리 채워 재입력 편의 제공
	if (!prefill.empty()) editBox->setText(prefill.c_str());
	dlg->addChild(editBox);

	// 엔터→랜덤이름 델리게이트
	if (s_nameDelegate) { s_nameDelegate->release(); s_nameDelegate = nullptr; }
	auto nd = new NameInputDelegate();
	nd->box = editBox;
	s_nameDelegate = nd;  // ref count=1, static이 소유
	editBox->setDelegate(nd);

	// 다이얼로그 애니메이션 후 키보드 자동 표시
	dlg->scheduleOnce([editBox](float) {
		editBox->openKeyboard();
	}, 0.35f, "autoKb");

	// 욕설/링크 필터 거부 메시지 표시용
	auto status = Label::createWithSystemFont("", "Arial", 10);
	status->setColor(Color3B(255, 120, 120));
	status->setPosition(Vec2(DW / 2, 62));
	dlg->addChild(status);

	// 재진입(더블탭) 방지 플래그 — 검증 성공 시 scene 전환하므로 중복 실행 차단
	auto submitting = std::make_shared<bool>(false);
	auto okBtn = makePopupChipButton("\xE2\x9C\x93 OK", kBtnFunc, [this, editBox, status, submitting](Ref*) {
		if (*submitting) return;
		std::string name = editBox->getText();
		*submitting = true;
		status->setColor(Color3B(180, 180, 180));
		status->setString("Checking...");

		auto alive = m_aliveFlag;
		// 검증 통과(또는 신뢰 우회) 후 실행되는 후속 처리 — 랭커 이름 중복검사 + 서버 반영.
		auto afterValidate = [this, alive, name, status, submitting]() {
			if (!alive || !*alive) return;
			// 랭커 이름 중복(대소문자 무시) 소프트 필터. ResetAll이 리더보드 캐시를 비우므로
			// 동기 검사 대신 전 레벨 리더보드를 fresh 조회 후 대조(비동기). 아래 updateDisplayName는
			// 이 콜백 안에서 실행되며, 람다는 아래쪽에서 닫힌다.
			LeaderboardManager::Instance()->isNameTakenByRankerAsync(name,
					[alive, name, status, submitting](bool nameTaken) {
				if (!alive || !*alive) return;
				if (nameTaken) {
					*submitting = false;
					status->setColor(Color3B(255, 120, 120));
					status->setString("Name already taken");
					return;
				}

				// 서버 반영(updateDisplayName) 성공 시에만 로컬 확정 + 진행.
				// 실패 시 로컬 커밋 안 함 + 입력창 유지(로컬↔서버 불일치 방지). 이름 설정은 이미 온라인 전제.
				LeaderboardManager::Instance()->updateDisplayName(name, [alive, name, status, submitting](bool nameOk) {
					if (!alive || !*alive) return;
					if (!nameOk) {
						*submitting = false;
						status->setColor(Color3B(255, 120, 120));
						status->setString("Couldn't set name, try again");
						return;
					}
					std::string nameCopy = name;  // SetUserName은 non-const ref를 받음
					UserDataManager::Instance()->SetUserName(nameCopy);
					UserDataManager::Instance()->SaveUserData();
					auto* ud = UserDataManager::Instance();
					int lv = ud->m_pendingSubmitLevel;
					int tm = ud->m_pendingSubmitTime;
					bool hasPending = ud->HasPendingSubmit();
					ud->ClearPendingSubmit();
					auto goToMain = []() {
						UserDataManager::Instance()->m_justRegistered = true;
						Director::getInstance()->replaceScene(
							TransitionFade::create(0.3f, MainScene::createScene()));
					};
					if (hasPending) {
						LeaderboardManager::Instance()->submitScore(lv, tm, [goToMain](bool) {
							goToMain();
						});
					} else {
						goToMain();
					}
				});
				});
		};
		// 큐레이션 추천 이름은 서버 욕설검증 우회(SCULPTOR 등 별자리명이 banned_words
		// 부분일치로 오탐되는 것 방지). 사용자가 직접 입력한 이름만 클라+서버 검증.
		if (isTrustedPoolName(name)) {
			afterValidate();
		} else {
			LeaderboardManager::Instance()->validateName(name,
				[alive, status, submitting, afterValidate](bool ok, const std::string& reason) {
					if (!alive || !*alive) return;
					if (!ok) {
						*submitting = false;
						status->setColor(Color3B(255, 120, 120));
						status->setString(
							reason == "profanity" ? "Inappropriate name" :
							reason == "link"      ? "Links are not allowed" :
							reason == "too_long"  ? "Max 12 characters" :
							                        "Please choose another name");
						return;
					}
					afterValidate();
				});
		}
	}, 100.f, 36.f, 15.f);
	auto okLabel = (Label*)okBtn->getChildByTag(kChipLabelTag);
	okBtn->setEnabled(false);

	dlg->schedule([editBox, okBtn, okLabel](float) {
		bool valid = strlen(editBox->getText()) >= 3;
		okBtn->setEnabled(valid);
		okLabel->setColor(valid ? kBtnFunc : Color3B(100, 100, 100));   // 유효=스카이블루(테두리색 일치)
	}, 0.05f, CC_REPEAT_FOREVER, 0.f, "nameCheck");

	auto menu = Menu::create(okBtn, nullptr);
	menu->setPosition(Vec2(DW / 2, kPopupBtnY));
	dlg->addChild(menu);
}

#ifdef ENABLE_AWARD_COMMENT
// ─────────────────────────────────────────────────────────────
//  랭킹 Top10 수상소감
// ─────────────────────────────────────────────────────────────

// 신기록 rank 확정 후 — 내 순위가 Top10이면 로컬 리플레이 업로드(고스트/관전용).
// (수상소감 입력창 자동 표시는 제거됨 — 유저가 랭킹에서 본인 항목을 탭할 때만 입력)
void MainScene::checkAndPromptAward(int level)
{
	auto alive = m_aliveFlag;
	LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
		[this, alive, level](const std::vector<LeaderboardEntry>& entries) {
			if (!alive || !*alive) return;
			if (!LeaderboardManager::Instance()->isAwardEnabled()) return;  // 마스터 OFF → 작성창 생략
			std::string myId = LeaderboardManager::Instance()->getPlayFabId();
			if (myId.empty()) return;
			for (const auto& e : entries) {
				if (e.playFabId == myId) {
					if (e.rank >= 1 && e.rank <= 10) {
						// 2차: level≤5 + Top10 → 로컬 최고기록 리플레이 업로드 (소스=UserDefault replay_L%d)
						if (level <= LeaderboardManager::REPLAY_MAX_LEVEL) {
							std::string blob = UserDefault::getInstance()->getStringForKey(
								StringUtils::format("replay_L%d", level).c_str(), "");
							if (!blob.empty()) {
								// 업로드 성공 시 리플레이 캐시가 비워지므로, 지금 이 레벨 보드를 보고 있으면
								// 다시 그려 ▶ 마커가 즉시 뜨게 함(안 그러면 다른 레벨 갔다 와야 표시됨).
								LeaderboardManager::Instance()->uploadReplay(level, blob,
									[this, alive, level](bool ok) {
										if (!ok || !alive || !*alive) return;
										if (m_rankLevel == level) drawOnlineRank(level);
									});
							}
						}
						// 수상소감 입력창 자동 표시 제거 — 유저가 원할 때 직접(랭킹에서 본인 항목 탭) 입력.
					}
					return;
				}
			}
		});
}

// 수상소감 작성/수정 입력창
void MainScene::showAwardInputDialog(int level, int rank, const std::string& existing)
{
	SoundFactory::Instance()->play("efs_click");
	const int TAG = 193;
	this->removeChildByTag(TAG);
	const float DW = 264, DH = 152;

	// 유틸 톤 = 그레이 테두리, 골드 타이틀. 배경 탭 닫힘(closeLs)이라 modal=false, 헤더가 title+sub라 divider=false. 키보드 공간 y+40.
	auto f = makePopupFrame(
		StringUtils::format("CONGRATULATIONS!  LEVEL %d  \xC2\xB7  RANK %d", level, rank),
		kBorderGray, kTextTitle, DW, DH, 13.f, kDimStd, false, false);
	f.backdrop->setTag(TAG);
	this->addChild(f.backdrop, 999);
	auto dlg = f.box;
	dlg->setPositionY(dlg->getPositionY() + 40);

	// 모달 — 박스 안 빈 곳은 삼키기만, 박스 밖(배경) 탭하면 닫힘.
	// (EditBox/POST 버튼은 더 깊은 자식이라 먼저 터치를 받아 정상 동작)
	auto closeLs = EventListenerTouchOneByOne::create();
	closeLs->setSwallowTouches(true);
	closeLs->onTouchBegan = [this, TAG, dlg, DW, DH](Touch* t, Event*) -> bool {
		Vec2 p = dlg->convertToNodeSpace(t->getLocation());
		if (p.x >= 0 && p.x <= DW && p.y >= 0 && p.y <= DH)
			return true;  // 박스 내부 — 삼키기만 (닫지 않음)
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
		return true;       // 박스 밖(배경) — 닫기
	};
	Director::getInstance()->getEventDispatcher()
		->addEventListenerWithSceneGraphPriority(closeLs, f.backdrop);

	auto sub = Label::createWithSystemFont("Leave your winning speech to the world", "Arial", 10);
	sub->setColor(Color3B(180, 200, 220));
	sub->setPosition(Vec2(DW / 2, DH - 38));
	dlg->addChild(sub);

	auto editBoxBg = DrawNode::create();
	editBoxBg->drawSolidRect(Vec2(8, DH - 86), Vec2(DW - 8, DH - 58), Color4F(0.05f, 0.07f, 0.15f, 0.9f));
	editBoxBg->drawRect(Vec2(8, DH - 86), Vec2(DW - 8, DH - 58), Color4F(0.4f, 0.4f, 0.6f, 0.7f));
	dlg->addChild(editBoxBg);

	auto editBox = cocos2d::ui::EditBox::create(Size(DW - 20, 24),
		cocos2d::ui::Scale9Sprite::create());
	editBox->setFont("Arial", 13);
	editBox->setFontColor(Color3B::WHITE);
	editBox->setPlaceholderFontColor(Color3B(170, 170, 170));
	editBox->setPlaceHolder(StringUtils::format("Your speech (max %d)",
		LeaderboardManager::AWARD_MAX_CP).c_str());
	// 코드포인트 제한은 아래 스케줄(utf8TruncateCP)이 실제로 강제. maxLength(UTF-16 코드유닛 기준)는
	// 이모지(서로게이트 2유닛)로도 항상 코드포인트 강제가 먼저 걸리도록 여유롭게(=서로게이트 분할 방지).
	editBox->setMaxLength(LeaderboardManager::AWARD_MAX_CP * 3);
	editBox->setInputMode(cocos2d::ui::EditBox::InputMode::SINGLE_LINE);
	editBox->setReturnType(cocos2d::ui::EditBox::KeyboardReturnType::DONE);
	editBox->setAnchorPoint(Vec2(0, 0.5f));
	editBox->setPosition(Vec2(10, DH - 72));
	// 기존 소감이 있으면 그대로, 없으면(신규 작성) 자국어 인삿말을 기본값으로 채움
	if (!existing.empty()) editBox->setText(existing.c_str());
	else                   editBox->setText(getLocalGreeting().c_str());
	dlg->addChild(editBox);

	dlg->scheduleOnce([editBox](float) { editBox->openKeyboard(); }, 0.35f, "awardKb");

	auto counter = Label::createWithSystemFont(
		StringUtils::format("0 / %d", LeaderboardManager::AWARD_MAX_CP), "Arial", 10);
	counter->setColor(Color3B(150, 150, 150));
	counter->setAnchorPoint(Vec2(1.0f, 0.5f));
	counter->setPosition(Vec2(DW - 10, DH - 98));
	dlg->addChild(counter);

	auto status = Label::createWithSystemFont("", "Arial", 10);
	status->setColor(Color3B(255, 120, 120));
	status->setAnchorPoint(Vec2(0, 0.5f));
	status->setPosition(Vec2(10, DH - 98));
	dlg->addChild(status);

	// 코드포인트 제한 강제 + 카운터 갱신 (iOS/Android 공용)
	dlg->schedule([editBox, counter](float) {
		const int MAXCP = LeaderboardManager::AWARD_MAX_CP;
		std::string t = editBox->getText();
		int len = LeaderboardManager::utf8Length(t);
		if (len > MAXCP) {
			editBox->setText(utf8TruncateCP(t, MAXCP).c_str());
			len = MAXCP;
		}
		counter->setString(StringUtils::format("%d / %d", len, MAXCP));
		counter->setColor(len >= MAXCP ? Color3B(255, 180, 80) : Color3B(150, 150, 150));
	}, 0.1f, CC_REPEAT_FOREVER, 0.f, "awardCount");

	// POST
	auto okBtn = makePopupChipButton("\xE2\x9C\x93 POST", kBtnFunc,
		[this, TAG, editBox, status, level, existing](Ref*) {
			std::string text = editBox->getText();
			// 변경 사항 없으면 서버 전송 없이 그냥 닫기
			auto trimws = [](std::string s) -> std::string {
				size_t a = s.find_first_not_of(" \t\r\n");
				if (a == std::string::npos) return std::string();
				size_t b = s.find_last_not_of(" \t\r\n");
				return s.substr(a, b - a + 1);
			};
			if (trimws(text) == trimws(existing)) {
				this->removeChildByTag(TAG);
				return;
			}
			std::string reason;
			if (!LeaderboardManager::clientFilterComment(text, reason)) {
				status->setColor(Color3B(255, 120, 120));
				status->setString(awardReasonMessage(reason));
				return;
			}
			status->setColor(Color3B(180, 180, 180));
			status->setString("Sending...");
			LeaderboardManager::Instance()->writeAwardComment(level, text,
				[this, TAG, status, level](bool ok, const std::string& r) {
					if (ok) {
						this->removeChildByTag(TAG);
						LeaderboardManager::Instance()->invalidateComments(level);
						drawOnlineRank(level);
						// 전파 지연 보상 재갱신
						this->scheduleOnce([this, level](float) {
							LeaderboardManager::Instance()->invalidateComments(level);
							drawOnlineRank(level);
						}, 1.5f, "awardRefresh");
					} else {
						status->setColor(Color3B(255, 120, 120));
						status->setString(awardReasonMessage(r));
					}
				});
		}, 100.f, 34.f);
	okBtn->setPosition(Vec2(DW / 2, 18));

	auto menu = Menu::create(okBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu);
}

// 수상소감 읽기 카드 (탭 시). 내 항목이면 '수정' 버튼 포함. 배경 탭하면 닫힘.
void MainScene::showAwardCardDialog(const LeaderboardEntry& e, int level)
{
	SoundFactory::Instance()->play("efs_click");
	const int TAG = 194;
	this->removeChildByTag(TAG);
	const float DW = 250, DH = 152;   // ▶WATCH 행 공간 확보 위해 세로 확장

	// 성취 톤 = 골드 테두리. 배경 탭 닫힘(closeLs)이라 modal=false, 헤더 2행이라 divider=false.
	auto f = makePopupFrame(StringUtils::format("\xF0\x9F\x8F\x86  RANK %d", e.rank),
	                        kBorderGold, kTextTitle, DW, DH, 16.f, kDimStd, false, false);
	f.backdrop->setTag(TAG);
	this->addChild(f.backdrop, 999);
	auto dlg = f.box;

	// 배경 아무 곳이나 탭하면 닫힘 (소감/버튼 메뉴가 더 깊은 노드라 먼저 처리됨)
	auto closeLs = EventListenerTouchOneByOne::create();
	closeLs->setSwallowTouches(true);
	closeLs->onTouchBegan = [this, TAG](Touch*, Event*) -> bool {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
		return true;
	};
	Director::getInstance()->getEventDispatcher()
		->addEventListenerWithSceneGraphPriority(closeLs, f.backdrop);

	std::string flag = e.countryCode.empty() ? "" : countryToFlag(e.countryCode);
	auto nameLbl = Label::createWithSystemFont(flag + "  " + e.displayName, "Arial", 13);
	nameLbl->setColor(kTextData);
	nameLbl->setAnchorPoint(Vec2(0, 0.5f));
	nameLbl->setPosition(Vec2(16, DH - 50));
	dlg->addChild(nameLbl);

	RecordTime rt = getRecordTime(e.scoreMs);
	auto timeLbl = Label::createWithSystemFont(
		StringUtils::format("%02d:%02d.%02d", rt.min, rt.sec, rt.ms), "Arial", 13);
	timeLbl->setColor(kTextAccent);
	timeLbl->setAnchorPoint(Vec2(1.0f, 0.5f));
	timeLbl->setPosition(Vec2(DW - 16, DH - 50));
	dlg->addChild(timeLbl);

	auto divider = DrawNode::create();
	divider->drawLine(Vec2(16, DH - 62), Vec2(DW - 16, DH - 62), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	dlg->addChild(divider);

	// ▶ WATCH — 이 랭커가 해당 레벨 리플레이 보유 시 비동기로 버튼 추가 (2차 관전)
	if (level <= LeaderboardManager::REPLAY_MAX_LEVEL) {
		auto alive = m_aliveFlag;
		std::string pid = e.playFabId, nm = e.displayName;
		int lv = level, rnk = e.rank, sms = e.scoreMs;
		LeaderboardManager::Instance()->fetchReplays(level,
			[this, alive, TAG, pid, nm, lv, rnk, sms, dlg, DW](const std::map<std::string, std::string>& m) {
				if (!alive || !*alive) return;
				if (!this->getChildByTag(TAG)) return;   // 카드 닫힘 → dlg 무효
				auto it = m.find(pid);
				if (it == m.end() || it->second.empty()) return;
				std::string blob = it->second;
				auto watchBtn = makePopupChipButton("\xF0\x9F\x91\x81 WATCH", kBtnFunc,
					[this, TAG, lv, blob, nm, rnk, sms](Ref*) {
						SoundFactory::Instance()->play("efs_click");
						this->removeChildByTag(TAG);
						Director::getInstance()->replaceScene(
							TransitionFade::create(0.3f, PlayScene::createSpectateScene(lv, blob, nm, rnk, sms)));
					}, 100.f, 30.f, 13.f);
				watchBtn->setPosition(Vec2(DW * 0.27f, kPopupBtnY));

				// ⚔ RACE — 고스트 대결 (3차)
				auto raceBtn = makePopupChipButton("\xE2\x9A\x94 RACE", kBtnRace,
					[this, TAG, lv, blob, nm, rnk, sms, pid](Ref*) {
						SoundFactory::Instance()->play("efs_click");
						this->removeChildByTag(TAG);
						Director::getInstance()->replaceScene(
							TransitionFade::create(0.3f, PlayScene::createRaceScene(lv, blob, nm, rnk, sms, pid)));
					}, 100.f, 30.f, 13.f);
				raceBtn->setPosition(Vec2(DW * 0.73f, kPopupBtnY));

				auto menu = Menu::create(watchBtn, raceBtn, nullptr);
				menu->setPosition(Vec2::ZERO);
				dlg->addChild(menu, 6);
			});
	}

	// 소감 어피던스(버튼 대체): 내 항목=끝에 ✎(수정, 소감 없으면 "Add a comment") /
	//                          남 항목=소감 있을 때만 끝에 🚩(신고). 소감 없는 남 항목엔 아무것도 안 뜸.
	std::string myId = LeaderboardManager::Instance()->getPlayFabId();
	bool isMe = !myId.empty() && e.playFabId == myId;
	int rank  = e.rank;
	std::string existing = e.comment;
	if (isMe || !existing.empty()) {
		std::string shown;
		if (isMe)
			shown = existing.empty() ? "\xE2\x9C\x8E  Add a comment"          // ✎ Add a comment
			                         : "\"" + existing + "\"  \xE2\x9C\x8E";   // "..."  ✎
		else
			shown = "\"" + existing + "\"  \xF0\x9F\x9A\xA9";                   // "..."  🚩 (소감 있을 때만)

		auto cmtLbl = Label::createWithSystemFont(shown, "Arial", 13);
		cmtLbl->setColor(isMe && existing.empty() ? kTextMuted : Color3B(230, 235, 180));
		cmtLbl->setDimensions(DW - 30, 0);
		cmtLbl->setHorizontalAlignment(TextHAlignment::CENTER);

		std::string pid = e.playFabId;
		auto cmtItem = MenuItemLabel::create(cmtLbl,
			[this, TAG, level, rank, existing, isMe, pid](Ref*) {
				if (isMe) {
					this->removeChildByTag(TAG);
					std::string prefill = existing;   // 격파 브래그 프리필(battle_reward), 1회 소비
					if (prefill.empty()) {
						std::string bragKey = StringUtils::format("battle_brag_L%02d", level);
						std::string brag = UserDefault::getInstance()->getStringForKey(bragKey.c_str(), "");
						if (!brag.empty()) {
							prefill = "Climbed over " + brag;
							UserDefault::getInstance()->deleteValueForKey(bragKey.c_str());
							UserDefault::getInstance()->flush();
						}
					}
					this->showAwardInputDialog(level, rank, prefill);
				} else {
					SoundFactory::Instance()->play("efs_click");
					openAwardReportEmail(level, rank, pid, existing);
					this->removeChildByTag(TAG);
				}
			});
		auto menu = Menu::create(cmtItem, nullptr);
		menu->setPosition(Vec2(DW / 2, DH - 82));
		dlg->addChild(menu, 6);
	}
}
#endif // ENABLE_AWARD_COMMENT

void MainScene::showSettingsMenu()
{
	const int TAG = 196;
	if (this->getChildByTag(TAG)) return;
	SoundFactory::Instance()->play("efs_click");

	const float DW = 240, DH = 130.f;

	// 파괴적 위험 톤 = 레드 테두리 + 레드 타이틀
	auto f = makePopupFrame("RESET ALL?", kBorderRed, kBorderRed, DW, DH, 15.f);
	f.backdrop->setTag(TAG);
	this->addChild(f.backdrop, 999);
	auto dlg = f.box;

	auto msg = Label::createWithSystemFont(
		"This will reset your name\nand clear your ranking.",
		"Arial", 12);
	msg->setColor(Color3B(210, 210, 210));
	msg->setAlignment(TextHAlignment::CENTER);
	msg->setPosition(Vec2(DW / 2, DH - 70));
	dlg->addChild(msg);

	auto doReset = [this]() {
		auto* ud = UserDataManager::Instance();
		// 이름 입력창에 미리 채울 마지막 사용 이름 — 초기화 전에 보관
		std::string lastName = ud->GetUserName();
		ud->ResetRecords();
		std::string empty;
		ud->SetUserName(empty);
		ud->SaveUserData();
		LeaderboardManager::Instance()->resetStats();

		// 랭킹보드 관련 전 캐시(리더보드/리플레이/수상소감) 무효화 — 리셋 후 옛 데이터 잔존 방지.
		LeaderboardManager::Instance()->clearAllCaches();
		// 로컬 최고기록 리플레이 저장본도 삭제: 기록이 0으로 초기화됐으니 stale.
		for (int lv = 3; lv <= LeaderboardManager::REPLAY_MAX_LEVEL; ++lv)
			UserDefault::getInstance()->deleteValueForKey(StringUtils::format("replay_L%d", lv).c_str());
		UserDefault::getInstance()->flush();

		if (m_rankBG)
		{
			m_rankBG->removeAllChildren();
			for (int level = 3; level <= MAX_PLAY_LEVEL; ++level)
			{
				int record = ud->GetBestRecord(level);
				RecordTime rt = getRecordTime(record);
				std::string str = StringUtils::format("%02d                                         %02d:%02d.%02d",
					level, rt.min, rt.sec, rt.ms);
				Label* lbl = Label::createWithSystemFont(str, "Arial", 14);
				lbl->setAnchorPoint(Vec2(0, 0));
				lbl->setPosition(Vec2(40, (level * 22) - 20));
				m_rankBG->addChild(lbl);
			}
		}
		showNameInputDialog(lastName);
	};

	auto okBtn = makePopupChipButton("\xE2\x9A\xA0 OK", kBtnDanger, [this, TAG, doReset](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
		doReset();
	}, 88.f, 34.f);
	okBtn->setPosition(Vec2(DW * 0.3f, kPopupBtnY));

	auto cancelBtn = makePopupChipButton("\xE2\x9C\x95 CANCEL", kBtnDismiss, [this, TAG](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
	}, 88.f, 34.f);
	cancelBtn->setPosition(Vec2(DW * 0.7f, kPopupBtnY));

	auto menu = Menu::create(okBtn, cancelBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu);
}

// 복수 다이얼로그 (battle_reward): 내 피격 낙인 탭 → 나를 이긴 상대(A) 고스트로 재도전.
void MainScene::showRevengeDialog(int level, const std::string& aId, const std::string& aName,
                                  int aRank, int aScore)
{
	const int TAG = 197;
	if (this->getChildByTag(TAG)) return;
	SoundFactory::Instance()->play("efs_click");

	const float DW = 248, DH = 140.f;

	// 부정(패배) 톤 = 그레이 테두리 + 레드 타이틀. 배경은 네이비 강제(붉은 톤 폐기).
	auto f = makePopupFrame("\xF0\x9F\x92\xA5 DEFEATED", kBorderGray, kTextTitleBad, DW, DH, 15.f);
	f.backdrop->setTag(TAG);
	this->addChild(f.backdrop, 999);
	auto dlg   = f.box;
	auto title = f.title;   // 리플레이 없을 때 "NO REPLAY"로 갱신

	// "{A} stole your ranking!"
	auto msg = Label::createWithSystemFont(
		StringUtils::format("%s stole your ranking!", utf8TruncateCP(aName, 10).c_str()),
		"Arial", 12);
	msg->setColor(Color3B(235, 215, 215));
	msg->setAlignment(TextHAlignment::CENTER);
	msg->setDimensions(DW - 28, 0);
	msg->setPosition(Vec2(DW / 2, DH - 68));
	dlg->addChild(msg);

	auto alive = m_aliveFlag;

	// [⚔ REVENGE] — A 리플레이 조회 후 고스트 레이스로 직행. 없으면 타이틀에 안내.
	auto revBtn = makePopupChipButton("\xE2\x9A\x94 REVENGE", kBtnRace,
		[this, alive, TAG, title, level, aId, aName, aRank, aScore](Ref*) {
			SoundFactory::Instance()->play("efs_click");
			LeaderboardManager::Instance()->fetchReplays(level,
				[this, alive, TAG, title, level, aId, aName, aRank, aScore](const std::map<std::string, std::string>& m) {
					if (!alive || !*alive) return;
					if (!this->getChildByTag(TAG)) return;   // 창 닫힘
					auto it = m.find(aId);
					if (it == m.end() || it->second.empty()) {
						// A 리플레이 없음 → 복수 불가 안내(창 유지)
						title->setString("NO REPLAY");
						title->setColor(Color3B(200, 200, 200));
						SoundFactory::Instance()->play("efs_cancel_select");
						return;
					}
					this->removeChildByTag(TAG);
					Director::getInstance()->replaceScene(TransitionFade::create(0.3f,
						PlayScene::createRaceScene(level, it->second, aName, aRank, aScore, aId, true)));   // 복수전
				});
		}, 104.f, 34.f);
	revBtn->setPosition(Vec2(DW * 0.30f, kPopupBtnY));

	auto laterBtn = makePopupChipButton("LATER", kBtnDismiss, [this, TAG](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
	}, 80.f, 34.f);
	laterBtn->setPosition(Vec2(DW * 0.72f, kPopupBtnY));

	auto menu = Menu::create(revBtn, laterBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu);
}
