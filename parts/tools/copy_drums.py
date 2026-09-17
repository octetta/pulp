import os
import glob
import shutil

drum_files = sorted(glob.glob("../sk/drums-*.ks"))

slot = 82
for filepath in drum_files:
    basename = os.path.basename(filepath)
    name = basename.replace("drums-", "").replace(".ks", "")
    new_name = f"{slot}_{name}.ks"
    
    with open(filepath, "r") as f:
        content = f.read()
    
    # We append O: 1 to the end, and then return the last assigned variable.
    # What was the last variable?
    lines = content.strip().split("\n")
    last_line = lines[-1]
    
    # If the last line is an assignment like W: ..., we extract W.
    if ":" in last_line and not last_line.startswith("/"):
        var_name = last_line.split(":")[0].strip()
        lines.append("")
        lines.append("O: 1")
        lines.append(var_name)
    else:
        # Just in case
        lines.append("O: 1")
    
    new_content = "\n".join(lines) + "\n"
    
    with open(f"../waves/drums/{new_name}", "w") as f:
        f.write(new_content)
        
    print(f"Copied {basename} to {new_name}")
    slot += 1

