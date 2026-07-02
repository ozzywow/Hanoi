#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
LDNOOBW(표준 오픈 금칙어 목록)에서 대상 언어를 받아
로컬 보강 목록(banned_extra_ko.txt)과 병합 → banned_words.json 생성.

산출된 banned_words.json 문자열을 PlayFab 대시보드
  Content → Title Data → key: "banned_words"
에 그대로 붙여넣으면 CloudScript(awardLoadBanned)가 로드합니다.

사용:
  python build_banned_words.py
인터넷이 막힌 환경이면 LDNOOBW txt를 미리 받아 ./ldnoobw/<lang> 로 두면
로컬 파일을 우선 사용합니다.
"""

import json, os, sys, urllib.request

# 플레이어가 실제 쓰는 언어만 골라서 (필요시 추가/삭제)
LANGS = ["en", "ko", "ja", "zh", "es", "fr", "de", "ru", "pt", "ar"]

RAW_BASE = ("https://raw.githubusercontent.com/LDNOOBW/"
            "List-of-Dirty-Naughty-Obscene-and-Otherwise-Bad-Words/master/{}")

HERE = os.path.dirname(os.path.abspath(__file__))


def fetch_lang(lang):
    # 1) 로컬 캐시 우선
    local = os.path.join(HERE, "ldnoobw", lang)
    if os.path.exists(local):
        with open(local, "r", encoding="utf-8") as f:
            return f.read().splitlines()
    # 2) 원격
    try:
        with urllib.request.urlopen(RAW_BASE.format(lang), timeout=15) as r:
            return r.read().decode("utf-8").splitlines()
    except Exception as e:
        print("  [skip] %s: %s" % (lang, e), file=sys.stderr)
        return []


def load_extra():
    extra = []
    path = os.path.join(HERE, "banned_extra_ko.txt")
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                w = line.strip()
                if w and not w.startswith("#"):
                    extra.append(w)
    return extra


def main():
    words = set()
    for lang in LANGS:
        got = fetch_lang(lang)
        n = 0
        for w in got:
            w = w.strip().lower()
            # CloudScript awardNormalize와 동일하게: 공백 제거
            w = w.replace(" ", "")
            if w:
                words.add(w); n += 1
        print("  %-3s +%d" % (lang, n))

    for w in load_extra():
        words.add(w.lower().replace(" ", ""))

    out = sorted(words)
    dst = os.path.join(HERE, "banned_words.json")
    with open(dst, "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False)
    print("total %d words -> %s" % (len(out), dst))


if __name__ == "__main__":
    main()
