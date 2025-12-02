import './assets/main.css'
import '@fortawesome/fontawesome-free/css/all.min.css'
import { createApp } from 'vue'
import { createPinia } from 'pinia'

// src/stores/theme.ts 或者直接在 main.ts
import { useDark, useToggle } from '@vueuse/core'

export const isDark = useDark({
  selector: 'html', // 在 <html> 上加 dark 类
  attribute: 'class',
  valueDark: 'dark',
  valueLight: '',
  storageKey: 'theme-mode', // 记住用户选择
})

export const toggleDark = useToggle(isDark)


import App from './App.vue'
import router from './router'

const app = createApp(App)

app.use(createPinia())
app.use(router)

app.mount('#app')
