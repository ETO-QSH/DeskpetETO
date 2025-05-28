import os
import json
import time
import urllib3
import requests
from tqdm import tqdm
from SimulationTool import WebScraper
from concurrent.futures import ThreadPoolExecutor, as_completed

scraper = WebScraper()
urllib3.disable_warnings()
session = requests.Session()
agents = scraper.get_all_agent_head_normal()
headers = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3'}


def download_files(json_data, filter=None):
    result, tasks = {agent["name"]: {} for agent in json_data}, []
    for agent in json_data:
        name = agent["name"]
        for head_name, url in agent["head"].items():
            tasks.append((url, f"saves/{name}/head/{head_name}.png", [name, head_name, "head"]))
        for spine_name, spine_models in agent["spine"].items():
            for model_name, base_url in spine_models.items():
                if filter:
                    if model_name not in filter:
                        break
                for suffix in [".png", ".skel", ".atlas"]:
                    path = f"saves/{name}/spine/{spine_name}/{model_name}/{base_url.split('/')[-1]}{suffix}"
                    tasks.append((f"{base_url}{suffix}", path, [name, spine_name, model_name, suffix[1:]]))

    with tqdm(total=len(tasks), desc="正在处理喵") as pbar:
        with ThreadPoolExecutor(max_workers=32) as executor:
            futures = []
            for task in tasks:
                future = executor.submit(download_single_file, *task, result_dict=result)
                futures.append(future)
            for _ in as_completed(futures):
                pbar.update()

    with open("saves.json", "w", encoding="utf-8") as f:
        json.dump(result, f, indent=4, ensure_ascii=False)


def download_single_file(url, save_path, key_path, result_dict):
    while True:
        try:
            os.makedirs(os.path.dirname(save_path), exist_ok=True)
            response = session.get(url, headers=headers, verify=False, timeout=15)
            response.raise_for_status()
            with open(save_path, "wb") as f:
                f.write(response.content)
            current = result_dict
            for key in key_path[:-1]:
                current = current.setdefault(key, {})
            current[key_path[-1]] = save_path.lower()
            break
        except Exception as e:
            print(f"下载失败正在重试: {url} ({type(e).__name__})")
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


with tqdm(total=len(agents), desc="正在处理喵") as pbar:
    with ThreadPoolExecutor(max_workers=8) as executor:
        futures = [executor.submit(process_agent_spines, item) for item in range(len(agents))]
        for future in as_completed(futures):
            future.result()
            pbar.update()

with open("data.json", "w", encoding="utf-8") as file:
    json.dump(agents, file, indent=4, ensure_ascii=False)
download_files(agents, "基建")
