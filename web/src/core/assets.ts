// 이미지 로더. 파일은 web/public/img/ (앱 Resources/NewUI 의 사본).

export type Images = Record<string, HTMLImageElement>;

const BASE = import.meta.env.BASE_URL; // 개발 '/', 배포 '/Hanoi/play/'

function loadOne(name: string): Promise<HTMLImageElement> {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.onload = () => resolve(img);
    img.onerror = () => reject(new Error(`image load failed: ${name}`));
    img.src = `${BASE}img/${name}`;
  });
}

/** 이름(예: 'main_bg.png') → 이미지. 하나라도 실패하면 reject. */
export async function loadImages(names: string[]): Promise<Images> {
  const imgs = await Promise.all(names.map(loadOne));
  const out: Images = {};
  names.forEach((n, i) => {
    out[n] = imgs[i];
  });
  return out;
}
