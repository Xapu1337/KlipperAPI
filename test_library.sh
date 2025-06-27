#!/bin/bash

echo "=== KlipperAPI Library Structure Test ==="
echo ""

# Check main library files
echo "📄 Main Library Files:"
if [ -f "KlipperAPI.h" ]; then
    echo "✅ KlipperAPI.h - $(wc -l < KlipperAPI.h) lines"
else
    echo "❌ KlipperAPI.h - Missing"
fi

if [ -f "KlipperAPI.cpp" ]; then
    echo "✅ KlipperAPI.cpp - $(wc -l < KlipperAPI.cpp) lines"
else
    echo "❌ KlipperAPI.cpp - Missing"
fi

if [ -f "library.properties" ]; then
    echo "✅ library.properties - Present"
else
    echo "❌ library.properties - Missing"
fi

if [ -f "keywords.txt" ]; then
    echo "✅ keywords.txt - Present"
else
    echo "❌ keywords.txt - Missing"
fi

echo ""

# Check examples
echo "📚 Example Files:"
example_count=$(find examples -name "*.ino" 2>/dev/null | wc -l)
echo "Total examples: $example_count"

for example in $(find examples -name "*.ino" 2>/dev/null); do
    echo "✅ $example - $(wc -l < "$example") lines"
done

echo ""

# Check library.properties content
echo "📋 Library Properties:"
grep "name=" library.properties 2>/dev/null || echo "❌ Name not found"
grep "version=" library.properties 2>/dev/null || echo "❌ Version not found"
grep "depends=" library.properties 2>/dev/null || echo "❌ Dependencies not found"

echo ""

# Check header file structure
echo "🔍 Header File Analysis:"
echo "Classes defined: $(grep -c "^class " KlipperAPI.h 2>/dev/null || echo "0")"
echo "Structures defined: $(grep -c "^typedef struct" KlipperAPI.h 2>/dev/null || echo "0")"
echo "Public methods: $(grep -c "bool " KlipperAPI.h 2>/dev/null || echo "0")"

echo ""

# Check for memory optimization features
echo "💾 Memory Optimization Features:"
if grep -q "uint8_t.*:" KlipperAPI.h 2>/dev/null; then
    echo "✅ Bit fields detected"
else
    echo "❌ No bit fields found"
fi

if grep -q "uint8_t\|uint16_t\|int16_t" KlipperAPI.h 2>/dev/null; then
    echo "✅ Optimized data types used"
else
    echo "❌ No optimized data types found"
fi

echo ""

# Check implementation file
echo "⚙️ Implementation Analysis:"
echo "API methods implemented: $(grep -c "^bool KlipperApi::" KlipperAPI.cpp 2>/dev/null || echo "0")"
echo "Helper methods: $(grep -c "^void KlipperApi::" KlipperAPI.cpp 2>/dev/null || echo "0")"

echo ""

# File sizes
echo "📊 File Sizes:"
echo "KlipperAPI.h: $(wc -c < KlipperAPI.h 2>/dev/null || echo "0") bytes"
echo "KlipperAPI.cpp: $(wc -c < KlipperAPI.cpp 2>/dev/null || echo "0") bytes"

echo ""
echo "=== Test Complete ==="