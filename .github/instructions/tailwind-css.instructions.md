---
applyTo: "src/css/**/*.css"
---

# CSS Styling Requirements

When working with CSS files for the Scribe project, follow these Tailwind-based
guidelines:

## Tailwind CSS Usage

1. **Utility-First Approach** - Use Tailwind utility classes whenever possible
2. **Custom Components** - Use `@layer components` for reusable component
   classes
3. **Responsive Design** - Always implement mobile-first responsive patterns
4. **Dark Mode** - Include dark mode variants using `dark:` prefix

## Responsive Breakpoints

1. **Mobile First** - Start with base styles, add breakpoint prefixes
2. **Standard Breakpoints**:
   - `sm:` (640px+) - Small tablets
   - `md:` (768px+) - Tablets
   - `lg:` (1024px+) - Small desktops
   - `xl:` (1280px+) - Large desktops
3. **Grid Layouts** - Use appropriate column counts for different screen sizes

## Recent Layout Improvements

1. **Quick Action Buttons** - Use balanced grid layouts:
   - Mobile: `grid-cols-2` (2 columns)
   - Small: `sm:grid-cols-3` (3 columns)
   - Medium+: `md:grid-cols-4` (4 columns) for 4+3 layout instead of 6+1
2. **LED Effect Buttons** - Integrated grid with spanning for Turn Off button
3. **Form Layouts** - Responsive form elements with proper spacing

## Component Patterns

1. **Button Styles**:
   ```css
   .btn-primary {
     @apply px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded-lg transition-colors;
   }
   ```
2. **Card Layouts**:
   ```css
   .card-primary {
     @apply bg-white dark:bg-gray-800 rounded-xl shadow-lg p-6;
   }
   ```
3. **Form Elements**:
   ```css
   .form-input {
     @apply w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md focus:ring-2 focus:ring-blue-500;
   }
   ```

## Color System

1. **Theme Colors** - Use consistent color palette across components
2. **LED Effect Colors** - Colorful buttons for different LED effects:
   - Blue for simple_chase
   - Red for rainbow
   - Yellow for twinkle
   - Green for chase
   - Purple for pulse
   - Gray for turn off
3. **Status Colors** - Green for success, red for errors, yellow for warnings

## Layout Principles

1. **Consistent Spacing** - Use Tailwind spacing scale (px-4, py-2, mt-8, etc.)
2. **Visual Hierarchy** - Clear typography scale and proper contrast ratios
3. **Accessibility** - Ensure sufficient color contrast and focus states
4. **Touch Targets** - Minimum 44px touch targets for mobile

## Animation and Transitions

1. **Smooth Transitions** - Use `transition-colors duration-200` for interactive
   elements
2. **Hover States** - Always include hover states for interactive elements
3. **Loading States** - Consider loading animations for async operations
4. **Micro-interactions** - Subtle feedback for user actions

## Build Process Integration

1. **Source Location** - CSS source in `src/css/`, compiled to `data/css/`
2. **Minification** - Production builds are minified automatically
3. **Purging** - Unused styles are removed during build
4. **Watch Mode** - Use `npm run watch-css` during development

## File Organization

1. **Modular CSS** - Separate files for different pages/components:
   - `index.css` - Homepage styles
   - `settings.css` - Settings page styles
   - `diagnostics.css` - Diagnostics page styles
   - `shared.css` - Common utilities
2. **Component Grouping** - Group related components in @layer components

## Performance Considerations

1. **Minimal Custom CSS** - Prefer Tailwind utilities over custom styles
2. **Efficient Selectors** - Avoid deep nesting and complex selectors
3. **Critical Path** - Inline critical styles, load non-critical styles
   asynchronously
4. **File Size** - Monitor compiled CSS size, aim for <50KB per page

## Recent Fixes Applied

- Fixed 6+1 button layout issue with better responsive grid (4+3 layout at
  1200px)
- Improved LED button integration with proper column spanning
- Enhanced dark mode support across all components
- Optimized build process to exclude unused generated files
