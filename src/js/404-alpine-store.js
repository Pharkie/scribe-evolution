/**
 * @file 404-alpine-store.js
 * @brief Alpine.js reactive store for 404 error page functionality
 */

// Alpine.js Store for 404 Error Page
window.ErrorStore = function() {
  return {
    // Error details from URL or template placeholders
    method: 'GET',
    path: '/unknown',
    
    // Animation states
    showContent: false,
    
    // Initialize store
    init() {
      // Extract error details from page if available
      this.extractErrorDetails();
      
      // Show content with animation
      setTimeout(() => {
        this.showContent = true;
      }, 100);
      
      this.startFloatingAnimation();
    },
    
    // Extract error details from template placeholders
    extractErrorDetails() {
      // Try to get method and path from template placeholders
      const methodElement = document.querySelector('[data-error-method]');
      const pathElement = document.querySelector('[data-error-path]');
      
      if (methodElement) {
        const method = methodElement.textContent || methodElement.getAttribute('data-error-method');
        if (method && method !== '{{METHOD}}') {
          this.method = method;
        }
      }
      
      if (pathElement) {
        const path = pathElement.textContent || pathElement.getAttribute('data-error-path');
        if (path && path !== '{{URI}}') {
          this.path = path;
        }
      }
      
      // Fallback: try to get from URL parameters
      const urlParams = new URLSearchParams(window.location.search);
      if (urlParams.get('method')) {
        this.method = urlParams.get('method');
      }
      if (urlParams.get('path')) {
        this.path = urlParams.get('path');
      }
    },
    
    // Get appropriate error message based on path
    get errorMessage() {
      if (this.path.includes('api')) {
        return 'That API endpoint seems to have gotten lost in the paper feed.';
      } else if (this.path.includes('css') || this.path.includes('js')) {
        return 'That resource got caught in the ribbon cartridge.';
      } else if (this.path.includes('image') || this.path.includes('img')) {
        return 'That image fell behind the thermal head.';
      } else {
        return 'Looks like that page got tangled up in the rollers.';
      }
    },
    
    // Get helpful suggestions based on path
    get suggestions() {
      const suggestions = ['Try the main printer interface', 'Check system diagnostics'];
      
      if (this.path.includes('settings')) {
        suggestions.unshift('Go to settings configuration');
      } else if (this.path.includes('api')) {
        suggestions.unshift('Review API documentation');
      } else if (this.path.includes('diagnostics')) {
        suggestions.unshift('Access system diagnostics');
      }
      
      return suggestions;
    },
    
    // Get error icon animation class
    get errorIconClass() {
      return this.showContent ? 'animate-bounce' : '';
    },
    
    // Navigation actions
    goHome() {
      window.location.href = '/';
    },
    
    goToSettings() {
      window.location.href = '/settings.html';
    },
    
    goToDiagnostics() {
      window.location.href = '/diagnostics.html';
    },
    
    goBack() {
      if (window.history.length > 1) {
        window.history.back();
      } else {
        this.goHome();
      }
    },
    
    // Try to reload the requested page
    tryAgain() {
      window.location.reload();
    },
    
    // Start floating animation for decorative elements
    startFloatingAnimation() {
      // Add some visual interest with floating elements
      const createFloatingElement = () => {
        const element = document.createElement('div');
        element.className = 'floating-paper';
        element.style.cssText = `
          position: fixed;
          top: -50px;
          left: ${Math.random() * 100}vw;
          width: 20px;
          height: 30px;
          background: linear-gradient(45deg, #f3f4f6, #e5e7eb);
          border-radius: 2px;
          opacity: 0.3;
          z-index: 1;
          pointer-events: none;
          animation: float-down ${5 + Math.random() * 5}s linear infinite;
        `;
        
        document.body.appendChild(element);
        
        setTimeout(() => {
          if (element.parentNode) {
            element.parentNode.removeChild(element);
          }
        }, 10000);
      };
      
      // Create floating elements periodically
      setInterval(createFloatingElement, 2000);
      
      // Add CSS animation if not exists
      if (!document.getElementById('floating-animation-styles')) {
        const style = document.createElement('style');
        style.id = 'floating-animation-styles';
        style.textContent = `
          @keyframes float-down {
            0% {
              transform: translateY(-50px) rotate(0deg);
              opacity: 0;
            }
            10% {
              opacity: 0.3;
            }
            90% {
              opacity: 0.3;
            }
            100% {
              transform: translateY(100vh) rotate(360deg);
              opacity: 0;
            }
          }
          
          .error-card {
            animation: slide-up 0.6s ease-out;
          }
          
          @keyframes slide-up {
            from {
              transform: translateY(30px);
              opacity: 0;
            }
            to {
              transform: translateY(0);
              opacity: 1;
            }
          }
        `;
        document.head.appendChild(style);
      }
    },
    
    // Format timestamp for error reporting
    get timestamp() {
      return new Date().toISOString();
    },
    
    // Generate error report
    get errorReport() {
      return {
        timestamp: this.timestamp,
        method: this.method,
        path: this.path,
        userAgent: navigator.userAgent,
        referrer: document.referrer || 'Direct access',
        url: window.location.href
      };
    },
    
    // Copy error details for support
    async copyErrorDetails() {
      const details = `Scribe Evolution Printer 404 Error Report
Timestamp: ${this.errorReport.timestamp}
Method: ${this.errorReport.method}
Path: ${this.errorReport.path}
User Agent: ${this.errorReport.userAgent}
Referrer: ${this.errorReport.referrer}
Current URL: ${this.errorReport.url}`;

      try {
        if (navigator.clipboard && window.isSecureContext) {
          await navigator.clipboard.writeText(details);
        } else {
          // Fallback
          const textarea = document.createElement('textarea');
          textarea.value = details;
          textarea.style.position = 'fixed';
          textarea.style.left = '-999999px';
          textarea.style.top = '-999999px';
          document.body.appendChild(textarea);
          textarea.focus();
          textarea.select();
          document.execCommand('copy');
          document.body.removeChild(textarea);
        }
        
        // Visual feedback - would need a toast system or similar
        console.log('Error details copied to clipboard');
        return true;
      } catch (error) {
        console.error('Failed to copy error details:', error);
        return false;
      }
    }
  };
};