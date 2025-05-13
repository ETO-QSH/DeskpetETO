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


def download_files(Json):
    result = dict()
    for agent in Json:
        result[agent["name"]] = {}
        os.makedirs(f"saves/{agent["name"]}", exist_ok=True)
        for head, link in agent["head"].items():
            response = session.get(link, headers=headers, verify=False)
            path = f"saves/{agent["name"]}/head/{head}.png"
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, 'wb') as f:
                f.write(response.content)
            result[agent["name"]][head] = {"head": path}
        for spine, links in agent["spine"].items():
            for model, link in links.items():
                result[agent["name"]][spine][model] = {}
                for suffix in [".png", ".skel", ".atlas"]:
                    response = session.get(link + suffix, headers=headers, verify=False)
                    path = f"saves/{agent["name"]}/spine/{spine}/{model}/{link.split("/")[-1]}{suffix}"
                    os.makedirs(os.path.dirname(path), exist_ok=True)
                    with open(path, 'wb') as f:
                        f.write(response.content)
                    result[agent["name"]][spine][model][suffix[1:]] = path
    with open(f"saves.json", "w", encoding="utf-8") as file:
        json.dump(result, file, indent=4, ensure_ascii=False)


def process_agent(item):
    while True:
        try:
            scraper = WebScraper()
            agent = scraper.get_one_agent_infor(agents[item], session, headers)
            agents[item] = agent
            break
        except Exception as e:
            print(agents[item], e)
            time.sleep(3)


with tqdm(total=len(agents), desc="正在处理喵") as pbar:
    with ThreadPoolExecutor(max_workers=4) as executor:
        futures = [executor.submit(process_agent, item) for item in range(len(agents))]
        for future in as_completed(futures):
            future.result()
            pbar.update()

with open("data.json", "w", encoding="utf-8") as file:
    json.dump(agents, file, indent=4, ensure_ascii=False)
download_files(agents)

with open("data.json", encoding="utf-8") as f:
    download_files(json.load(f))
