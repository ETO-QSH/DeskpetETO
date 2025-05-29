import os
import json
import shutil


def extract_values(json_data):
    values = []
    if isinstance(json_data, dict):
        for value in json_data.values():
            values.extend(extract_values(value))
    elif isinstance(json_data, list):
        for item in json_data:
            values.extend(extract_values(item))
    else:
        values.append(json_data)
    return values


models_folder = "models"
json_file = "saves.json"

with open(json_file, "r", encoding="utf-8") as f:
    data = json.load(f)

target_paths = extract_values(data)
print(target_paths)

for root, dirs, files in os.walk(models_folder):
    for file in files:
        print(file)
        for target_path in target_paths:
            if (file.replace("#", '_').lower() in target_path.lower()
                    and "." in file and file.rsplit(".", 1)[1] == "png"):
                current_path = os.path.join(root, file)
                os.makedirs(os.path.dirname(target_path), exist_ok=True)
                shutil.move(current_path, target_path)
                print(f"Moved: {current_path} -> {target_path}")
                break
