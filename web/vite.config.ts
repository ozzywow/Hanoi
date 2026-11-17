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
  },
  test: {
    include: ['tests/**/*.test.ts'],
    environment: 'node',
  },
}));
