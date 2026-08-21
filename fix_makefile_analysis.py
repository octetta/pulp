import re

with open("parts/Makefile", "r") as f:
    text = f.read()

# Remove analysis-src and analysis-check from .PHONY
text = text.replace(" analysis-src analysis-check", "")

# Remove variables
text = re.sub(r'ANALYSIS_BUILD_DIR \?= build_analysis\nANALYSIS_SRC_DIR \?= analysis-src\nANALYSIS_GENERATED = \\\n(?:\t[a-zA-Z0-9_.-]+ \\\n)*\t[a-zA-Z0-9_.-]+\n\n', '', text)

# Remove analysis-src and analysis-check targets
text = re.sub(r'analysis-src:\n(?:\t.*\n)+', '', text)
text = re.sub(r'analysis-check: analysis-src\n(?:\t.*\n)+', '', text)

with open("parts/Makefile", "w") as f:
    f.write(text)

