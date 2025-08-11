#!/bin/bash

# Script to update settings.html with all required changes
# This applies all changes in a single operation to avoid file corruption

SETTINGS_FILE="data/html/settings.html"

echo "Updating settings.html with color themes and improvements..."

# Backup the original file
cp "$SETTINGS_FILE" "${SETTINGS_FILE}.backup"

# Apply all changes using sed
sed -i '' \
    -e 's/<!-- WiFi Configuration -->/<!-- WiFi -->/g' \
    -e 's/<span>WiFi Configuration<\/span>/<span>WiFi<\/span>/g' \
    -e 's/bg-white dark:bg-gray-800 shadow-2xl rounded-3xl p-8 border border-gray-100 dark:border-gray-700/bg-blue-50 dark:bg-blue-900\/20 border border-blue-200 dark:border-blue-700 shadow-2xl rounded-3xl p-8/1' \
    -e 's/text-2xl font-bold text-gray-800 dark:text-gray-100 mb-6 flex items-center space-x-2/text-2xl font-bold text-blue-800 dark:text-blue-100 mb-6 flex items-center space-x-2/1' \
    \
    -e 's/<!-- Device Configuration -->/<!-- Device -->/g' \
    -e 's/<span>Device Configuration<\/span>/<span>Device<\/span>/g' \
    -e 's/bg-white dark:bg-gray-800 shadow-2xl rounded-3xl p-8 border border-gray-100 dark:border-gray-700/bg-purple-50 dark:bg-purple-900\/20 border border-purple-200 dark:border-purple-700 shadow-2xl rounded-3xl p-8/2' \
    -e 's/text-2xl font-bold text-gray-800 dark:text-gray-100 mb-6 flex items-center space-x-2/text-2xl font-bold text-purple-800 dark:text-purple-100 mb-6 flex items-center space-x-2/2' \
    \
    -e 's/<!-- MQTT Configuration -->/<!-- MQTT -->/g' \
    -e 's/<span>MQTT Configuration<\/span>/<span>MQTT<\/span>/g' \
    -e 's/bg-white dark:bg-gray-800 shadow-2xl rounded-3xl p-8 border border-gray-100 dark:border-gray-700/bg-yellow-50 dark:bg-yellow-900\/20 border border-yellow-200 dark:border-yellow-700 shadow-2xl rounded-3xl p-8/3' \
    -e 's/text-2xl font-bold text-gray-800 dark:text-gray-100 mb-6 flex items-center space-x-2/text-2xl font-bold text-yellow-800 dark:text-yellow-100 mb-6 flex items-center space-x-2/3' \
    \
    -e 's/<!-- Unbidden Ink Configuration -->/<!-- Unbidden Ink -->/g' \
    -e 's/<span>Unbidden Ink Configuration<\/span>/<span>Unbidden Ink<\/span>/g' \
    -e 's/bg-white dark:bg-gray-800 shadow-2xl rounded-3xl p-8 border border-gray-100 dark:border-gray-700/bg-green-50 dark:bg-green-900\/20 border border-green-200 dark:border-green-700 shadow-2xl rounded-3xl p-8/4' \
    -e 's/text-2xl font-bold text-gray-800 dark:text-gray-100 mb-6 flex items-center space-x-2/text-2xl font-bold text-green-800 dark:text-green-100 mb-6 flex items-center space-x-2/4' \
    \
    -e 's/<!-- Button Configuration -->/<!-- Buttons -->/g' \
    -e 's/<span>Button Configuration<\/span>/<span>Buttons<\/span>/g' \
    -e 's/bg-white dark:bg-gray-800 shadow-2xl rounded-3xl p-8 border border-gray-100 dark:border-gray-700/bg-orange-50 dark:bg-orange-900\/20 border border-orange-200 dark:border-orange-700 shadow-2xl rounded-3xl p-8/5' \
    -e 's/text-2xl font-bold text-gray-800 dark:text-gray-100 mb-6 flex items-center space-x-2/text-2xl font-bold text-orange-800 dark:text-orange-100 mb-6 flex items-center space-x-2/5' \
    \
    -e 's/Generate a brief, interesting fact about Doctor Who - the characters, episodes, behind-the-scenes trivia, or the show'\''s history\. Keep it under 250 characters\./Generate a fun fact under 200 characters about BBC Doctor Who - the characters, episodes, behind-the-scenes trivia, or the show'\''s history that is esoteric and only 5% of fans might know\./g' \
    \
    "$SETTINGS_FILE"

# Add the test button for Unbidden Ink (this is more complex, so we'll use a different approach)
# Find the Custom prompt textarea and add the test button after it

# First, let's add the test button HTML
TEMP_FILE=$(mktemp)

# Process the file to add the test button
awk '
/<!-- Custom prompt textarea -->/ { in_textarea_section = 1 }
/<\/textarea>/ && in_textarea_section {
    print $0
    print "                            </div>"
    print ""
    print "                            <!-- Test Unbidden Ink -->"
    print "                            <div class=\"pt-4 text-right\">"
    print "                                <button type=\"button\" onclick=\"testUnbiddenInk()\""
    print "                                    class=\"text-sm text-purple-600 dark:text-purple-400 hover:text-purple-800 dark:hover:text-purple-300 hover:underline font-medium transition-colors focus:outline-none focus:ring-2 focus:ring-purple-400 rounded\">"
    print "                                    Bid Unbidden Ink To Print Now"
    print "                                </button>"
    print "                            </div>"
    in_textarea_section = 0
    next
}
/^                            <\/div>$/ && in_textarea_section {
    # Skip the original closing div for the textarea section
    next
}
{ print }
' "$SETTINGS_FILE" > "$TEMP_FILE"

mv "$TEMP_FILE" "$SETTINGS_FILE"

# Remove the magic wand emoji from Enable Unbidden Ink toggle
sed -i '' 's/<span class="text-2xl">🪄<\/span>//g' "$SETTINGS_FILE"

echo "Settings.html has been updated successfully!"
echo "Changes applied:"
echo "  ✅ WiFi section - Blue theme"
echo "  ✅ Device section - Purple theme" 
echo "  ✅ MQTT section - Yellow theme"
echo "  ✅ Unbidden Ink section - Green theme"
echo "  ✅ Buttons section - Orange theme"
echo "  ✅ Removed 'Configuration' from all headings"
echo "  ✅ Updated Doctor Who prompt"
echo "  ✅ Added 'Bid Unbidden Ink To Print Now' button"
echo "  ✅ Removed duplicate magic wand emoji"
echo ""
echo "Backup saved as: ${SETTINGS_FILE}.backup"
