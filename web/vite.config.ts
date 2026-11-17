import { defineConfig } from 'vitest/config';

// 배포 경로: https://ozzywow.github.io/Hanoi/play/  (gh-pages 브랜치의 /play/)
// 개발 서버(`npm run dev`)는 루트(http://localhost:5173/)로 열고, 빌드/미리보기만 /Hanoi/play/ base를 쓴다.
export default defineConfig(({ command }) => ({
  base: command === 'serve' ? '/' : '/Hanoi/play/',
  build: {
    target: 'es2020',
    outDir: 'dist',
    emptyOutDir: true,
    sourcemap: false,
    // 영어(/play/) + 한국어(/play/ko/) 두 진입 페이지 — 같은 번들을 공유한다
    rollupOptions: { input: { main: 'index.html', ko: 'ko/index.html' } },
  },
  test: {
    include: ['tests/**/*.test.ts'],
    environment: 'node',
  },
}));
