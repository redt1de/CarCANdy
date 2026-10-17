import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// Relative base path since the built bundle is served from the ESP32's
// LittleFS root, not from a domain root.
// https://vite.dev/config/
export default defineConfig({
  base: './',
  plugins: [react()],
})
