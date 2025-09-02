# CSS Styling Requirements

## Tailwind CSS Usage

- Use utility-first approach with Tailwind utility classes
- Use `@layer components` for reusable component classes
- Always implement mobile-first responsive patterns
- Include dark mode variants using `dark:` prefix

## Responsive Breakpoints

- Mobile First: Start with base styles, add breakpoint prefixes
- `sm:` (640px+), `md:` (768px+), `lg:` (1024px+), `xl:` (1280px+)
- Use appropriate grid column counts for different screen sizes

## Component Patterns

```css
.btn-primary {
  @apply px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded-lg transition-colors;
}
.card-primary {
  @apply bg-white dark:bg-gray-800 rounded-xl shadow-lg p-6;
}
.form-input {
  @apply w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md focus:ring-2 focus:ring-blue-500;
}
```

## Layout & Color System

- Use consistent Tailwind spacing scale (px-4, py-2, mt-8, etc.)
- Clear typography scale and proper contrast ratios
- Ensure sufficient color contrast and focus states
- Minimum 44px touch targets for mobile
- Use `transition-colors duration-200` for interactive elements
- Always include hover states for interactive elements

## LED Effect Colors

- Blue for simple_chase, Red for rainbow, Yellow for twinkle
- Green for chase, Purple for pulse, Gray for turn off
- Green for success, red for errors, yellow for warnings

## Build Process

- CSS source in `src/css/`, compiled to `data/css/`
- Production builds are minified automatically
- Unused styles are removed during build
- Use `npm run watch-css` during development

## File Organization

- Modular CSS: separate files for different pages/components
- `index.css`, `settings.css`, `diagnostics.css`, `shared.css`
- Group related components in @layer components

## Performance

- Prefer Tailwind utilities over custom styles
- Avoid deep nesting and complex selectors
- Monitor compiled CSS size, aim for <50KB per page
