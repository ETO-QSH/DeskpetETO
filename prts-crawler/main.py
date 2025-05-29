import os
import json
import time
from pathlib import Path

import urllib3
import requests
from PIL import Image

from tqdm import tqdm
from SimulationTool import WebScraper
from concurrent.futures import ThreadPoolExecutor, as_completed

scraper = WebScraper()
urllib3.disable_warnings()
session = requests.Session()
# agents = scraper.get_all_agent_head_normal()[:10]
headers = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3'}


def download_files(json_data, filter=None):
    result = {agent["name"]: {} for agent in json_data}
    tasks = []

    # 生成分组任务
    for agent in json_data:
        name = agent["name"]
        # 处理头部资源（保持原有逻辑）
        for head_name, url in agent["head"].items():
            tasks.append(("head", {
                "type": "head",
                "agent_name": name,
                "url": url,
                "save_path": f"saves/{name}/head/{head_name}.png",
                "key_path": [head_name, "head"]
            }))

        # 处理spine资源（按模型分组）
        for spine_name, spine_models in agent["spine"].items():
            for model_name, base_url in spine_models.items():
                if filter and model_name not in filter:
                    continue
                tasks.append(("spine", {
                    "type": "spine",
                    "agent_name": name,
                    "spine_name": spine_name,
                    "model_name": model_name,
                    "base_url": base_url,
                    "directory": f"saves/{name}/spine/{spine_name}/{model_name}/"
                }))

    with tqdm(total=len(tasks), desc="正在处理喵") as pbar:
        with ThreadPoolExecutor(max_workers=32) as executor:
            futures = []
            spine_groups = {}  # 用于跟踪spine分组任务

            # 提交任务
            for task_type, task_data in tasks:
                if task_type == "head":
                    future = executor.submit(process_head_task, task_data, result)
                    futures.append(future)
                elif task_type == "spine":
                    future = executor.submit(process_spine_group, task_data)
                    futures.append(future)
                    # 记录分组任务用于后续处理
                    key = (task_data["agent_name"], task_data["spine_name"], task_data["model_name"])
                    spine_groups[key] = {"future": future, "data": task_data}

            # 处理结果
            for future in as_completed(futures):
                try:
                    res = future.result()
                    if res and res["type"] == "spine":
                        # 更新spine结果到result字典
                        agent_name = res["agent_name"]
                        spine_name = res["spine_name"]
                        model_name = res["model_name"]

                        current = result[agent_name].setdefault(spine_name, {})
                        current[model_name] = {
                            "png": res["png_path"],
                            "skel": res["skel_path"],
                            "atlas": res["atlas_path"]
                        }
                except Exception as e:
                    print(f"任务处理失败: {type(e).__name__} - {str(e)}")
                finally:
                    pbar.update()

    with open("saves.json", "w", encoding="utf-8") as f:
        json.dump(result, f, indent=4, ensure_ascii=False)


def process_head_task(task_data, result_dict):
    while True:
        try:
            os.makedirs(os.path.dirname(task_data["save_path"]), exist_ok=True)
            response = session.get(task_data["url"], headers=headers, verify=False, timeout=15)
            response.raise_for_status()
            with open(task_data["save_path"], "wb") as f:
                f.write(response.content)

            # 更新结果字典
            current = result_dict[task_data["agent_name"]]
            for key in task_data["key_path"][:-1]:
                current = current.setdefault(key, {})
            current[task_data["key_path"][-1]] = task_data["save_path"]
            return
        except Exception as e:
            print(f"头部资源下载失败正在重试: {task_data['url']} ({type(e).__name__})")
            time.sleep(3)


def process_spine_group(task_data):
    while True:
        try:
            base_url = task_data["base_url"]
            directory = task_data["directory"]
            base_filename = base_url.split('/')[-1]

            # 下载组内所有文件
            file_paths = {}
            for suffix in [".png", ".skel", ".atlas"]:
                url = f"{base_url}{suffix}"
                original_path = os.path.join(directory, f"{base_filename}{suffix}")
                os.makedirs(os.path.dirname(original_path), exist_ok=True)

                for _ in range(3):
                    try:
                        response = session.get(url, headers=headers, verify=False, timeout=15)
                        response.raise_for_status()
                        with open(original_path, "wb") as f:
                            f.write(response.content)
                        file_paths[suffix] = original_path
                        break
                    except Exception as e:
                        print(f"资源下载失败正在重试: {url} ({type(e).__name__})")
                        time.sleep(3)
                else:
                    raise Exception(f"无法下载文件: {url}")

            # 读取atlas文件获取新文件名
            atlas_path = file_paths[".atlas"]
            with open(atlas_path, "r+", encoding="utf-8") as f:
                lines = [line.rstrip('\n') for line in f.readlines()]
                if len(lines) < 2 or lines[1].strip()[-4:] != ".png":
                    raise ValueError("Atlas文件格式不正确（文件名行错误）")
                new_filename = convert_filename_to_lower(lines[1].strip().split(".png")[0])

                # 替换第二行内容
                lines[1] = f"{new_filename}.png"
                f.seek(0)
                f.truncate()
                f.write("\n".join(lines))

                # 获取第三行尺寸信息
                if len(lines) < 3 and "size:" in lines[2]:
                    raise ValueError("Atlas文件格式不正确（尺寸行错误）")
                size_str = lines[2].replace("size:", "").strip()
                try:
                    width, height = map(int, size_str.split(','))
                except ValueError:
                    raise ValueError(f"无效的尺寸格式: {size_str}")

            # 重命名所有文件
            new_paths = {}
            for suffix, old_path in file_paths.items():
                new_path = os.path.join(directory, f"{new_filename}{suffix}")
                os.rename(old_path, new_path)
                new_paths[suffix] = new_path

            png_path = new_paths[".png"]
            try:
                with Image.open(png_path) as img:
                    # 使用高质量缩放算法
                    resized_img = img.resize((width, height), Image.Resampling.LANCZOS)
                    resized_img.save(png_path, optimize=True, quality=95)
            except Exception as e:
                raise RuntimeError(f"PNG尺寸调整失败: {png_path} ({str(e)})")

            return {
                "type": "spine",
                "agent_name": task_data["agent_name"],
                "spine_name": task_data["spine_name"],
                "model_name": task_data["model_name"],
                "png_path": new_paths[".png"],
                "skel_path": new_paths[".skel"],
                "atlas_path": new_paths[".atlas"]
            }

        except Exception as e:
            print(f"Spine组处理失败正在重试: {base_url} ({type(e).__name__})")
            # 清理可能已下载的文件
            for path in file_paths.values():
                if os.path.exists(path):
                    os.remove(path)
            time.sleep(3)


def process_agent_spines(item):
    while True:
        try:
            scraper = WebScraper()
            agent = scraper.get_one_agent_spines(agents[item], session, headers)
            agents[item] = agent
            break
        except Exception as e:
            print(f"获取失败正在重试: {agents[item]["name"]} ({type(e).__name__})")
            time.sleep(3)


def convert_filename_to_lower(filename):
    # 将文件名按"_"分割，只将第一个纯数字前的部分转为小写
    parts = filename.split("_")
    for i in range(len(parts)):
        # 判断是否为纯数字
        if parts[i].isdigit():
            break
        parts[i] = parts[i].lower()
    return "_".join(parts).replace("\\", "/").replace("#", "_")


# with tqdm(total=len(agents), desc="正在处理喵") as pbar:
#     with ThreadPoolExecutor(max_workers=8) as executor:
#         futures = [executor.submit(process_agent_spines, item) for item in range(len(agents))]
#         for future in as_completed(futures):
#             future.result()
#             pbar.update()

# with open("data.json", "w", encoding="utf-8") as file:
#     json.dump(agents, file, indent=4, ensure_ascii=False)

with open("data.json", "r", encoding="utf-8") as file:
    agents = json.load(file)

download_files(agents, "基建")
