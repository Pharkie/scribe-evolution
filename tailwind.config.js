/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./data/html/**/*.html",
    "./src/js/**/*.js"
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
        // Custom colors can be added here if needed
      },
      spacing: {
        // Custom spacing for sliders and specific components
        'slider-thumb': '20px',
      },
      borderRadius: {
        '3xl': '1.5rem', // 24px - already available in Tailwind 3.x+
      },
      transitionDuration: {
        '600': '600ms',
      }
    }
  }
}