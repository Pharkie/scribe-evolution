// page-settings-memos.js
// Memo Settings Page - Standalone Alpine store for managing memo configurations

// =================== MEMOS API FUNCTIONS ===================

/**
 * Load memos from server
 * @returns {Promise<Object>} Memos object from server
 */
async function loadMemos() {
    try {
        const response = await fetch('/api/memos');
        if (!response.ok) {
            throw new Error(`Memos API returned ${response.status}: ${response.statusText}`);
        }
        
        const memos = await response.json();
        return memos;
        
    } catch (error) {
        console.error('Error loading memos:', error);
        throw error;
    }
}

/**
 * Save memos to server
 * @param {Object} memosData - The memos data to save
 * @returns {Promise<string>} Server response message
 */
async function saveMemos(memosData) {
    try {
        const response = await fetch('/api/memos', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(memosData)
        });
        
        if (!response.ok) {
            const errorText = await response.text();
            throw new Error(`Memos API returned ${response.status}: ${errorText}`);
        }
        
    } catch (error) {
        console.error('Error saving memos:', error);
        throw error;
    }
}

// =================== UTILITY FUNCTIONS ===================

/**
 * Show error message with consistent styling
 * @param {string} message - Error message to display
 * @param {number} duration - Duration in ms to show message (default: 5000)
 */
function showErrorMessage(message, duration = 5000) {
    console.error('Error:', message);
    // Create error toast - could be enhanced with a toast library
    const errorDiv = document.createElement('div');
    errorDiv.className = 'fixed top-4 right-4 bg-red-500 text-white p-4 rounded-lg shadow-lg z-50';
    errorDiv.textContent = message;
    document.body.appendChild(errorDiv);
    
    setTimeout(() => {
        if (errorDiv.parentNode) {
            errorDiv.parentNode.removeChild(errorDiv);
        }
    }, duration);
}

// =================== MAIN ALPINE STORE ===================

function createMemosStore() {
    const store = {
        // =================== STATE ===================
        loaded: false,  // Simple loading flag (starts false)
        saving: false,
        error: null,
        initialized: false,
        hasUnsavedChanges: false,
        
        // Configuration data (empty object populated on load)
        memos: {},

        // =================== MEMO CONFIGURATION API ===================
        
        async loadConfiguration() {
            // Duplicate initialization guard (failsafe)
            if (this.initialized) {
                return;
            }
            this.initialized = true;
            
            this.loaded = false;
            this.error = null;
            
            try {
                const memosData = await loadMemos();
                
                // ✅ CRITICAL: Direct assignment to memos object
                this.memos = {
                    memo1: memosData?.memo1 || '',
                    memo2: memosData?.memo2 || '',
                    memo3: memosData?.memo3 || '',
                    memo4: memosData?.memo4 || ''
                };
                
                this.hasUnsavedChanges = false;
                this.loaded = true;  // Mark as loaded AFTER data assignment
                
            } catch (error) {
                console.error('Error loading memos:', error);
                this.error = `Failed to load memo settings: ${error.message}`;
                showErrorMessage(this.error);
            }
        },

        // =================== MEMO SAVING ===================
        
        async saveMemos() {
            if (!this.hasUnsavedChanges) {
                return;
            }
            
            this.saving = true;
            this.error = null;
            
            try {
                const cleanMemos = {
                    memo1: this.memos.memo1,
                    memo2: this.memos.memo2,
                    memo3: this.memos.memo3,
                    memo4: this.memos.memo4
                };
                
                await saveMemos(cleanMemos);
                this.hasUnsavedChanges = false;
                
                // Redirect immediately back to settings with success indicator
                window.location.href = '/settings.html?saved=memos';
                
            } catch (error) {
                console.error('Error saving memos:', error);
                this.error = `Failed to save memo settings: ${error.message}`;
                showErrorMessage(this.error);
                this.saving = false;
            }
        },

        // =================== CHARACTER COUNT FUNCTIONS ===================
        
        // Character count getters for each memo (500 character limit)
        get memo1CharacterCount() {
            return this.memos.memo1?.length || 0;
        },
        
        get memo1CharacterText() {
            const count = this.memo1CharacterCount;
            const limit = 500;
            if (count > limit) {
                const over = count - limit;
                return `${count}/${limit} (${over} over)`;
            }
            return `${count}/${limit}`;
        },
        
        get memo1CharacterClass() {
            const count = this.memo1CharacterCount;
            const limit = 500;
            const percentage = count / limit;
            
            if (count > limit) {
                return 'text-red-600 dark:text-red-400 font-semibold';
            } else if (percentage > 0.8) {
                return 'text-yellow-600 dark:text-yellow-400';
            } else {
                return 'text-gray-500 dark:text-gray-400';
            }
        },

        get memo2CharacterCount() {
            return this.memos.memo2?.length || 0;
        },
        
        get memo2CharacterText() {
            const count = this.memo2CharacterCount;
            const limit = 500;
            if (count > limit) {
                const over = count - limit;
                return `${count}/${limit} (${over} over)`;
            }
            return `${count}/${limit}`;
        },
        
        get memo2CharacterClass() {
            const count = this.memo2CharacterCount;
            const limit = 500;
            const percentage = count / limit;
            
            if (count > limit) {
                return 'text-red-600 dark:text-red-400 font-semibold';
            } else if (percentage > 0.8) {
                return 'text-yellow-600 dark:text-yellow-400';
            } else {
                return 'text-gray-500 dark:text-gray-400';
            }
        },

        get memo3CharacterCount() {
            return this.memos.memo3?.length || 0;
        },
        
        get memo3CharacterText() {
            const count = this.memo3CharacterCount;
            const limit = 500;
            if (count > limit) {
                const over = count - limit;
                return `${count}/${limit} (${over} over)`;
            }
            return `${count}/${limit}`;
        },
        
        get memo3CharacterClass() {
            const count = this.memo3CharacterCount;
            const limit = 500;
            const percentage = count / limit;
            
            if (count > limit) {
                return 'text-red-600 dark:text-red-400 font-semibold';
            } else if (percentage > 0.8) {
                return 'text-yellow-600 dark:text-yellow-400';
            } else {
                return 'text-gray-500 dark:text-gray-400';
            }
        },

        get memo4CharacterCount() {
            return this.memos.memo4?.length || 0;
        },
        
        get memo4CharacterText() {
            const count = this.memo4CharacterCount;
            const limit = 500;
            if (count > limit) {
                const over = count - limit;
                return `${count}/${limit} (${over} over)`;
            }
            return `${count}/${limit}`;
        },
        
        get memo4CharacterClass() {
            const count = this.memo4CharacterCount;
            const limit = 500;
            const percentage = count / limit;
            
            if (count > limit) {
                return 'text-red-600 dark:text-red-400 font-semibold';
            } else if (percentage > 0.8) {
                return 'text-yellow-600 dark:text-yellow-400';
            } else {
                return 'text-gray-500 dark:text-gray-400';
            }
        },

        // =================== VALIDATION ===================
        
        // Check if any memo exceeds character limit
        get hasCharacterLimitExceeded() {
            const limit = 500;
            return this.memo1CharacterCount > limit ||
                   this.memo2CharacterCount > limit ||
                   this.memo3CharacterCount > limit ||
                   this.memo4CharacterCount > limit;
        },

        // Check if save button should be enabled (following wifi.js pattern)
        get canSave() {
            // Don't allow save while not loaded, saving, or with errors
            if (!this.loaded || this.saving || this.error) {
                return false;
            }
            
            // Don't allow save if no changes or character limit exceeded
            return this.hasUnsavedChanges && !this.hasCharacterLimitExceeded;
        }
    };
    
    return store;
}

// =================== ALPINE STORE REGISTRATION ===================

document.addEventListener('alpine:init', () => {
    const memosStore = createMemosStore();
    Alpine.store('settingsMemos', memosStore);
});