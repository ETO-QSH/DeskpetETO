# import requests
#
# # 设置请求头部，模拟浏览器访问
# headers = {
#     'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3'
# }
#
# url = 'https://prts.wiki/w/%E5%85%8B%E6%B4%9B%E4%B8%9D'
#
# response = requests.get(url, headers=headers, verify=False)
#
# # 检查请求是否成功
# if response.status_code == 200:
#     # 获取网页内容
#     html_content = response.text
#     print(html_content)
# else:
#     print(f"请求失败，状态码：{response.status_code}")


"""
def get_agent_list(session, headers):
    try:
        url = 'https://prts.wiki/w/%E8%89%BE%E9%9B%85%E6%B3%95%E6%8B%89'
        response = session.get(url, headers=headers, verify=False)
        result = [{'name': unquote(url.split('/w/')[1]), 'href': '/w/' + url.split('/w/')[1]}]
        if response.status_code == 200:
            tree = html.fromstring(response.content)
            target_table = tree.xpath('//*[@class="navbox navigation-not-searchable"]')
            if target_table:
                tr_list = [tr_element for tr_element in target_table[0].xpath('./tbody/tr/td/table/tbody/tr')]
                for tr_element in tr_list[2::2]:
                    li_list = [li_element.xpath('.//a')[0] for li_element in tr_element.xpath('./td/div/ul/li')]
                    result += [{'name': li.get('title'), 'href': li.get('href'), "head": {}} for li in li_list if li.get('title')]
                return result if result else "定位元素失败，未发现信息"
            else:
                return "结构解析失败，找不到表格"
        else:
            return f"请求失败，状态码：{response.status_code}"
    except Exception as e:
        return e
"""


"""
    def get_one_agent_head_skin(self, href):
        self.browser.get(f"https://prts.wiki{href}")
        ssw = WebDriverWait(self.browser, 15, 1).until(PEL((By.CLASS_NAME, "skinswitcher-wrapper")))
        div_list = ssw.find_elements(By.XPATH, "./div")
        result = list()
        for div in div_list:
            name = div.find_element(By.CLASS_NAME, 'skinname').get_attribute('textContent')
            link = div.find_element(By.CLASS_NAME, 'skinhead').find_element(By.XPATH, './img').get_attribute('src')
            result.append({"name": name, "link": unquote(link).split("?")[0]})
        self.browser.quit()
        return result

    def get_one_agent_spine(self, href, session, headers):
        self.browser.get(f"https://prts.wiki{href}")
        WebDriverWait(self.browser, 15, 1).until(PEL((By.CLASS_NAME, "n-button__content")))
        self.browser.find_element(By.CLASS_NAME, "n-button__content").click()
        root = WebDriverWait(self.browser, 15, 1).until(PEL((By.ID, "spine-root")))
        per_name = root.get_attribute('data-id')
        URL = f"https://torappu.prts.wiki/assets/char_spine/{per_name}/meta.json"
        prefix = f"https://torappu.prts.wiki/assets/char_spine/{per_name}/"
        self.browser.quit()
        return prefix, session.get(URL, headers=headers, verify=False).json()["skin"]
"""


""" 大无语事件喵 ~
from collections import defaultdict
skin_model_dict = defaultdict()
self.browser.execute_cdp_cmd('Network.enable', {})
self.browser.execute_cdp_cmd('Network.setRequestInterception', {'patterns': [{'urlPattern': '*'}]})
self.file_urls = []
target_extensions = ['.skel', '.atlas']

def request_handler(request_id, **kwargs):
    url = kwargs.get('request', {}).get('url', '')
    if any(url.endswith(ext) for ext in target_extensions):
        self.file_urls.append(url)

self.browser.add_cdp_listener('Network.requestIntercepted', request_handler)

def lct():
    root = WebDriverWait(self.browser, 15, 1).until(PEL((By.ID, "spine-root")))
    pre_name = root.get_attribute('data-id')

    top = "./div/div/div/div[1]"
    WebDriverWait(root, 15, 1).until(PEL((By.XPATH, top)))
    div_top = root.find_element(By.XPATH, top)

    skin_div = div_top.find_elements(By.XPATH, "./div")[0]
    skin_div.find_element(By.XPATH, "./div").click()
    skin_name = skin_div.find_element(By.XPATH, "./div").find_element(By.CLASS_NAME, "n-base-selection-input").get_attribute('title')

    name = "defaultskin" if skin_name == "默认" else "不知道捏"
    s = f"https://torappu.prts.wiki/assets/char_spine/{pre_name}/{name}/"
    skin_model_dict[skin_name] = {"front": f"{s}/front/{name}", "back": f"{s}/back/{name}", "build": f"{s}/build/build_{name}"}

lct()
skin = self.browser.find_elements(By.CLASS_NAME, "v-binder-follower-container")[0]

skin_list = skin.find_element(By.CLASS_NAME, "v-vl-visible-items").find_elements(By.XPATH, "./div")
for skin in skin_list:
    if not skin_model_dict[skin.find_element(By.XPATH, "./div").get_attribute('textContent')]:
        skin.click()
        lct()

print(skin_model_dict)
"""

data = {
    "默认": {
        "背面": {
            "file": "defaultskin/back/char_180_amgoat"
        },
        "正面": {
            "file": "defaultskin/front/char_180_amgoat"
        },
        "基建": {
            "file": "defaultskin/build/build_char_180_amgoat"
        }
    },
    "夏卉 FA018": {
        "正面": {
            "file": "char_180_amgoat_summer_5/front/char_180_amgoat_summer_5"
        },
        "背面": {
            "file": "char_180_amgoat_summer_5/back/char_180_amgoat_summer_5"
        },
        "基建": {
            "file": "char_180_amgoat_summer_5/build/build_char_180_amgoat_summer_5"
        }
    }

}

import json
from main import download_files

with open("data.json", encoding="utf-8") as f:
    download_files(json.load(f))
