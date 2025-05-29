import os
import json
import shutil
import subprocess
from tqdm import tqdm
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed


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


output_folder = "output"
json_file = "saves.json"

with open(json_file, "r", encoding="utf-8") as f:
    data = json.load(f)

target_paths = [file for file in extract_values(data) if Path(file).suffix == ".skel"]


def spine_preview(skel):
    os.makedirs("./output", exist_ok=True)
    path = os.path.join("./output", Path(skel).stem)

    prefix = skel.rsplit(".", 1)[0]
    atlas = prefix + ".atlas"
    png = prefix + ".png"

    files = []
    for file, suffix in [(skel, ".skel"), (atlas, ".atlas"), (png, ".png")]:
        shutil.copyfile(file, path + suffix)
        files.append(path + suffix)

    result = subprocess.run(
        ["./preview.exe", *files[:2], path + "_output.png"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )

    for file in files:
        os.remove(file)

    return Path(skel).stem, result.returncode


def main():
    max_workers = 16
    total = len(target_paths)

    with tqdm(total=total, desc="处理进度", unit="文件", colour="green") as pbar:
        with ThreadPoolExecutor(max_workers=max_workers) as executor:
            futures = {executor.submit(spine_preview, skel): skel for skel in target_paths}
            for future in as_completed(futures):
                try:
                    result = future.result()
                    if result:
                        skel_name, returncode = result
                        pbar.update(1)
                        if returncode != 0:
                            pbar.write(f"警告: {skel_name} 处理失败，返回码 {returncode}")
                except Exception as e:
                    pbar.write(f"错误: {futures[future]} 处理出错: {str(e)}")


if __name__ == "__main__":
    main()
