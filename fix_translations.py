#!/usr/bin/env python3
import re
import sys

# Import translation dictionary
from translations_dict import translations

def process_ts_file(filename):
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()

    lines = content.split('\n')
    output_lines = []
    i = 0

    while i < len(lines):
        line = lines[i]
        output_lines.append(line)

        # Check if this is a <source> line with unfinished translation
        if '<source>' in line and i + 1 < len(lines):
            # Extract source text
            source_match = re.search(r'<source>(.*?)</source>', line)
            if source_match:
                source_text = source_match.group(1)

                # Look ahead for translation line
                next_line = lines[i + 1]
                if 'type="unfinished"' in next_line and '<translation' in next_line:
                    # Check if source is Chinese (contains Chinese characters)
                    if re.search(r'[\u4e00-\u9fff]', source_text):
                        # Chinese source - use source as translation
                        output_lines.append(f'        <translation>{source_text}</translation>')
                        i += 2  # Skip the original translation line
                        continue
                    # Check if there's already a translation in the line (unfinished but present)
                    trans_match = re.search(r'<translation type="unfinished">(.*?)</translation>', next_line)
                    if trans_match and trans_match.group(1):
                        # Already has translation, just remove unfinished flag
                        translation_text = trans_match.group(1)
                        output_lines.append(f'        <translation>{translation_text}</translation>')
                        i += 2
                        continue
                    else:
                        # English source - translate if we have mapping
                        if source_text in translations:
                            output_lines.append(f'        <translation>{translations[source_text]}</translation>')
                            i += 2
                            continue

        i += 1

    # Write back
    with open(filename, 'w', encoding='utf-8') as f:
        f.write('\n'.join(output_lines))

    print(f"Processed {filename}")

if __name__ == '__main__':
    process_ts_file('translations/videostudio_zh_CN.ts')
