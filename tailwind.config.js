/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./data/html/**/*.html",
    "./data/**/*.html",
    "./src/**/*.{js,ts,jsx,tsx}",
    "./**/*.html"
  ],
  darkMode: 'media', // Automatic system preference detection
  theme: {
    screens: {
      'sm': '640px',
      'md': '768px',
      'lg': '1024px',
      'xl': '1200px',
      '2xl': '1536px',
    },
    extend: {
      colors: {
        // Custom brand colors extracted from hardcoded values
        brand: {
          'slider': '#10b981',
          'slider-hover': '#059669',
          'slider-bg': '#e5e7eb',
        }
      },
      spacing: {
        // Custom spacing for sliders and specific components
        'slider-thumb': '20px',
      },
      borderRadius: {
        '3xl': '1.5rem', // 24px - already available in Tailwind 3.x+
      },
      animation: {
        'fade-in': 'fade-in 0.6s ease-out forwards',
        'fade-out': 'fade-out 0.6s ease-out forwards',
      },
      keyframes: {
        'fade-in': {
          'from': { opacity: '0', transform: 'translateY(8px)' },
          'to': { opacity: '1', transform: 'translateY(0)' },
        },
        'fade-out': {
          'from': { opacity: '1', transform: 'translateY(0)' },
          'to': { opacity: '0', transform: 'translateY(-8px)' },
        },
      },
      transitionDuration: {
        '600': '600ms',
      }
    }
  }
}