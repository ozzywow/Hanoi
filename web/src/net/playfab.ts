// PlayFab REST (읽기 전용 + 익명 집계). 계정 없이 랜딩과 같은 공용 뷰어 계정으로만 접속한다.
// ※ public 저장소 — 여기에는 Title ID(공개 정보)만 있고 비밀 키는 절대 두지 않는다.

const TITLE_ID = '119C4E';
const API = `https://${TITLE_ID}.playfabapi.com`;
const VIEWER_CUSTOM_ID = 'web_public_viewer'; // landing/index.html 과 동일한 공용 뷰어 계정
const TICKET_KEY = 'hanoi_web_pf_t';

interface PfResponse<T> {
  code?: number;
  status?: string;
  data?: T;
}

function readTicket(): string | null {
  try {
    return sessionStorage.getItem(TICKET_KEY);
  } catch {
    return null;
  }
}
function writeTicket(t: string | null): void {
  try {
    if (t) sessionStorage.setItem(TICKET_KEY, t);
    else sessionStorage.removeItem(TICKET_KEY);
  } catch {
    /* 무시 */
  }
}

let loginInFlight: Promise<string> | null = null;

/** 공용 뷰어 로그인 — 동시 호출은 하나로 합친다(앱의 login in-flight 가드와 동일) */
function login(): Promise<string> {
  if (loginInFlight) return loginInFlight;
  loginInFlight = fetch(`${API}/Client/LoginWithCustomID`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ TitleId: TITLE_ID, CustomId: VIEWER_CUSTOM_ID, CreateAccount: true }),
  })
    .then((r) => r.json() as Promise<PfResponse<{ SessionTicket?: string }>>)
    .then((j) => {
      const t = j.data?.SessionTicket;
      if (!t) throw new Error('login failed');
      writeTicket(t);
      return t;
    })
    .finally(() => {
      loginInFlight = null;
    });
  return loginInFlight;
}

async function ticket(): Promise<string> {
  return readTicket() ?? login();
}

/** 인증이 필요한 Client API 호출. 401(티켓 만료)이면 한 번 재로그인 후 재시도 */
export async function pfPost<T>(path: string, body: unknown): Promise<T> {
  const call = async (t: string): Promise<Response> =>
    fetch(`${API}${path}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', 'X-Authorization': t },
      body: JSON.stringify(body),
    });

  let res = await call(await ticket());
  if (res.status === 401) {
    writeTicket(null);
    res = await call(await login());
  }
  const j = (await res.json()) as PfResponse<T>;
  if (!res.ok || j.data === undefined) throw new Error(`PlayFab ${path} failed: ${res.status}`);
  return j.data;
}
