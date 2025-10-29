/// <reference types="vite/client" />

interface ImportMetaEnv {
  readonly VITE_AUTO_LOGIN: string
  readonly VITE_TEST_USER_ID?: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}

import 'vue-router'

declare module 'vue-router' {
  interface RouteMeta {
    title?: string
  }
}
