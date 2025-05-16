from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.support.wait import WebDriverWait
from selenium.webdriver.remote.webelement import WebElement
from selenium.webdriver.support.expected_conditions import presence_of_element_located as PEL
from urllib.parse import unquote

WebElement.stext = property(lambda self: self.get_attribute('textContent').strip())


class WebScraper:
    def __init__(self):
        chrome_options = Options()
        chrome_options.add_argument('--headless')
        chrome_options.add_argument('--disable-gpu')
        self.browser = webdriver.Chrome(options=chrome_options)
        self.get_xpath_script = """
            function getAbsoluteXPath(element) {
                let path = '';
                while (element !== document.documentElement) { // 从当前元素向上遍历到 html 节点
                    let tagName = element.tagName.toLowerCase();
                    let index = 1; // 兄弟节点中的位置索引（从1开始）
                    let siblings = element.parentNode.childNodes;
                    for (let sib of siblings) {
                        if (sib === element) break;
                        if (sib.nodeType === 1 && sib.tagName === element.tagName) index++;
                    }
                    path = '/' + tagName + (index > 1 ? '[' + index + ']' : '') + path;
                    element = element.parentNode;
                }
                return '/html' + path; // 补全根节点
            }
            return getAbsoluteXPath(arguments[0]);
        """

    def get_all_agent_head_normal(self):
        self.browser.get("https://prts.wiki/w/%E5%B9%B2%E5%91%98%E4%B8%80%E8%A7%88")
        app_element = WebDriverWait(self.browser, 15, 1).until(PEL((By.ID, "root")))

        tx = "./div/div[5]/div[2]/div[3]"            # 切换至头像标签
        lb = "./div/div[6]/div[2]/select/option[3]"  # 每页两百条
        su = "./div/div[6]/div[2]/div[1]"            # 一共有多少干员
        ne = "./div/div[6]/div[2]/div[2]/div[$]"     # 切换页码
        ac = "./div/div[7]/div/div[$]"               # 干员头像控件

        WebDriverWait(app_element, 15, 1).until(PEL((By.XPATH, tx)))
        app_element.find_element(By.XPATH, tx).click()
        app_element.find_element(By.XPATH, lb).click()

        num = int(app_element.find_element(By.XPATH, su).text[1:-1])
        page = (num + 199) // 200
        links = list()

        for i in range(1, page + 1):
            app_element.find_element(By.XPATH, ne.replace("$", str(i))).click()
            for j in range(1, 201):
                if len(links) < num:
                    one = app_element.find_element(By.XPATH, ac.replace("$", str(j)))
                    name = unquote(one.find_element(By.XPATH, './a').get_attribute('href')).split('/w/')[1]
                    link = one.find_element(By.XPATH, './a/img').get_attribute('data-src')
                    links.append({"name": name, "head": {"默认": link}, "spine": {}})
                else:
                    break

        self.browser.quit()
        return links

    def get_one_agent_spines(self, agent, session, headers):
        self.browser.get(f"https://prts.wiki/w/{agent["name"]}")

        ssw = WebDriverWait(self.browser, 15, 1).until(PEL((By.CLASS_NAME, "skinswitcher-wrapper")))
        div_list = ssw.find_elements(By.XPATH, "./div")

        for div in div_list:
            name = div.find_element(By.CLASS_NAME, 'skinname').stext
            link = div.find_element(By.CLASS_NAME, 'skinhead').find_element(By.XPATH, './img').get_attribute('src')
            agent["head"][name] = unquote(link.split("?")[0])

        WebDriverWait(self.browser, 15, 1).until(PEL((By.CLASS_NAME, "n-button__content")))
        self.browser.find_element(By.CLASS_NAME, "n-button__content").click()

        root = WebDriverWait(self.browser, 15, 1).until(PEL((By.ID, "spine-root")))
        per_name = root.get_attribute('data-id')

        URL = f"https://torappu.prts.wiki/assets/char_spine/{per_name}/meta.json"
        prefix = f"https://torappu.prts.wiki/assets/char_spine/{per_name}/"
        data = session.get(URL, headers=headers, verify=False).json()["skin"]

        for skin in data:
            if skin not in agent["spine"]:
                agent["spine"][skin] = {}
            for model, link in data[skin].items():
                agent["spine"][skin][model] = prefix + link["file"]

        self.browser.quit()
        return agent

    def get_one_agent_voices(self, name):
        self.browser.get(f"https://prts.wiki/w/{name}")

        vdr = WebDriverWait(self.browser, 15, 1).until(PEL((By.ID, "voice-data-root")))
        voice_dict = eval("{'" + vdr.get_attribute('data-voice-base').replace(",", "','").replace(":", "':'") + "'}")
        div_list = vdr.find_elements(By.XPATH, "./div")
        prefix = "https://torappu.prts.wiki/assets/audio/"
        voice_result = {}

        for div in div_list:
            title = div.get_attribute('data-title')
            filename = div.get_attribute('data-voice-filename')
            voice_result[title] = {"text": {}, "link": {}}
            voice_list = div.find_elements(By.XPATH, "./div")
            for voice in voice_list:
                voice_kind = voice.get_attribute('data-kind-name')
                voice_text = voice.stext
                voice_result[title]["text"][voice_kind] = voice_text
            for kind, link in voice_dict.items():
                voice_result[title]["link"][kind] = prefix + link + "/" + filename

        self.browser.quit()
        return voice_result

    def get_one_agent_records(self, name):
        self.browser.get(f"https://prts.wiki/w/{name}")
        result = {}

        def title_2_record(title, n, filter=None):
            element = WebDriverWait(self.browser, 15, 1).until(PEL((By.ID, title)))
            parent_element = element.find_element(By.XPATH, "./..")
            parent_tag = parent_element.tag_name

            if filter:
                siblings = parent_element.find_elements(By.XPATH, "following-sibling::*")

                count, target_parent = 0, None
                for sib in siblings:
                    if sib.tag_name == parent_tag:
                        count += 1
                        if count == n:
                            target_parent = sib
                            break

                if not target_parent:
                    return []

                elements, current = [], parent_element.find_element(By.XPATH, "following-sibling::*[1]")
                while current and current != target_parent:
                    elements.append(current)
                    next_elements = current.find_elements(By.XPATH, "following-sibling::*[1]")
                    current = next_elements[0] if next_elements else None

                if len(elements) < len(filter):
                    return []

                i, matched, window_size = 0, [], len(filter)
                while i <= len(elements) - window_size:
                    match = all(elements[i + j].tag_name.lower() == filter[j].lower() for j in range(window_size))
                    if match:
                        matched.extend(elements[i:i + window_size])
                        i += window_size
                    else:
                        i += 1

                return matched

            else:
                return parent_element.find_element(By.XPATH, f"following-sibling::*[{n}]")

        illustrate_list = WebDriverWait(self.browser, 15, 1).until(PEL((By.ID, "charimg"))).find_elements(By.XPATH, "./img")
        result["立绘"] = {illustrate.get_attribute('id'): illustrate.get_attribute('src').split("?")[0] for illustrate in illustrate_list}

        h3divs = title_2_record("模组", 1, filter=["h3", "div"])
        titles = [h3.find_elements(By.XPATH, "./span")[-1].stext for h3 in h3divs[::2]]
        contents = [h3divs[1].find_element(By.XPATH, "./div[3]/div[2]").stext]
        contents += [div.find_element(By.XPATH, "./div/div[2]/div[2]/div[2]").stext for div in h3divs[3::2]]
        result["模组"] = dict(zip(titles, contents))

        ht, xw = title_2_record("相关道具", 1).find_element(By.XPATH, "./tbody").find_elements(By.XPATH, "./tr")
        potential = {}
        for tag, name in [(ht, "招聘合同"), (xw, "信物")]:
            contents = []
            for p in tag.find_elements(By.XPATH, './td/p'):
                if content := p.stext:
                    contents.append(content)
                else:
                    contents += [i.stext for i in p.find_elements(By.XPATH, './i')]
            potential[name] = "\n".join(contents)
        result["相关道具"] = potential

        tr_list = title_2_record("干员档案", 2).find_element(By.XPATH, "./tbody").find_elements(By.XPATH, "./tr")
        titles = [tr_title.find_element(By.XPATH, "./th/div/p").stext for tr_title in tr_list[1::3]]
        contents = [tr_content.find_element(By.XPATH, "./td/div/p").stext for tr_content in tr_list[3::3]]
        result["干员档案"] = dict(zip(titles, contents))

        try:
            tr_list = title_2_record("悖论模拟", 2).find_element(By.XPATH, "./tbody").find_elements(By.XPATH, "./tr")
            title = tr_list[1].find_element(By.XPATH, "./th/div/p").stext
            content = tr_list[3].find_element(By.XPATH, "./td/div/p").stext
            result["悖论模拟"] = {title: content}
        except:
            result["悖论模拟"] = dict()

        try:
            tr_list = title_2_record("干员密录", 2).find_element(By.XPATH, "./tbody").find_elements(By.XPATH, "./tr")[1:]
            paradox = []
            for tr in tr_list:
                title_div, poem_div = tr.find_element(By.XPATH, "./td/div").find_elements(By.XPATH, "./div")
                poem_b, link_a = poem_div.find_elements(By.XPATH, "./div")
                title = title_div.find_element(By.XPATH, "./div[2]/b").stext
                poem = poem_b.find_element(By.XPATH, "./div/p/i").stext
                link = link_a.find_element(By.XPATH, "./a").get_attribute('href')
                paradox.append({"title": title, "poem": poem, "link": link})
            result["干员密录"] = paradox
        except:
            result["干员密录"] = list()

        self.browser.quit()
        return result

    def get_one_story_page(self, link):
        self.browser.get(link)
        playback_all_result = WebDriverWait(self.browser, 15, 1).until(PEL((By.ID, "playback_all_result"))).find_elements(By.XPATH, "./*")
        firstHeading = self.browser.find_element(By.ID, "firstHeading").stext

        dialogue_result = []
        for item in playback_all_result:
            tag = item.tag_name.lower()
            if tag == "li":
                em_elements = item.find_elements(By.XPATH, "./em")
                em = em_elements[0].stext if em_elements else None
                span = item.find_element(By.XPATH, "./span").stext
                dialogue_result.append({"type": "normal", "dialogue": {"by": em, "text": span}})
            elif tag == "div":
                sub_divs = item.find_elements(By.XPATH, "./div")
                if sub_divs:
                    temp_dict = []
                    for sub_div in sub_divs:
                        key_span = sub_div.find_element(By.XPATH, "./span").stext
                        li_list = [{"by": "Dr.", "text": key_span}]
                        for li in sub_div.find_elements(By.XPATH, "./li"):
                            li_em = li.find_elements(By.XPATH, ".//em")
                            li_span = li.find_element(By.XPATH, ".//span").stext
                            li_list.append({"by": li_em[0].stext if li_em else None, "text": li_span})
                        temp_dict.append(li_list)
                    dialogue_result.append({"type": "choice", "dialogue": temp_dict})
                else:
                    key_span = item.find_element(By.XPATH, "./li").find_element(By.XPATH, ".//span").stext
                    dialogue_result.append({"type": "single", "dialogue": {"by": "Dr.", "text": key_span}})

        self.browser.quit()
        return {firstHeading: dialogue_result}

    def get_main_story_list(self):
        self.browser.get("https://prts.wiki/w/%E5%89%A7%E6%83%85%E4%B8%80%E8%A7%88")

        WebDriverWait(self.browser, 15, 1).until(PEL((By.CLASS_NAME, "nomobile")))
        tr_list_0 = self.browser.find_element(By.ID, "collapsibleTable0").find_elements(By.XPATH, "./tbody/tr")[2::2]
        tr_list_1 = self.browser.find_element(By.ID, "collapsibleTable1").find_elements(By.XPATH, "./tbody/tr")[2::2]

        result = dict()
        for key, value in [("主线剧情", tr_list_0), ("活动剧情", tr_list_1)]:
            tr_01 = []
            for tr in tr_list_0:
                title = tr.find_element(By.XPATH, "./th").stext
                tbody = tr.find_element(By.XPATH, "./td/table/tbody/tr")
                classify = tbody.find_element(By.XPATH, "./th").stext
                li_list = tbody.find_elements(By.XPATH, "./td/div/li")
                story = []
                for li in li_list:
                    link = li.find_element(By.XPATH, "./a").get_attribute('href')
                    name = li.find_element(By.XPATH, "./a").get_attribute('textContent')
                    story.append({"name": name, "link": link})
                tr_01.append({"title": title, "classify": classify, "story": story})
            result[key] = tr_01

        self.browser.quit()
        return result

    def get_skin_brand_list(self):
        self.browser.get("https://prts.wiki/w/%E6%97%B6%E8%A3%85%E5%9B%9E%E5%BB%8A")

        result, brand_list = {}, WebDriverWait(self.browser, 15, 1).until(PEL((By.CLASS_NAME, "brandbtncontroler")))
        for div in brand_list.find_elements(By.XPATH, "./div"):
            item = div.find_element(By.XPATH, "./div/div/a")
            name = item.find_element(By.XPATH, "./div[2]").stext
            link = item.get_attribute('href')
            result[name] = link

        self.browser.quit()
        return result

    def get_one_brand_infor(self, link):
        self.browser.get(link)

        mpo = WebDriverWait(self.browser, 15, 1).until(PEL((By.CLASS_NAME, "mw-parser-output")))
        result, table_list = {}, mpo.find_elements(By.XPATH, "./table")[::3]
        for table in table_list:
            tr_list = table.find_elements(By.XPATH, "./tbody/tr")
            title = tr_list[0].find_element(By.XPATH, "./th").stext
            description = tr_list[1].find_element(By.XPATH, "./td[2]")
            annotations = tr_list[-1].find_element(By.XPATH, "./td")
            result[title] = dict()

            for tag, name in [(description, "描述"), (annotations, "注释")]:
                contents = []
                for p in tag.find_elements(By.XPATH, './p'):
                    if content := p.stext:
                        contents.append(content)
                    else:
                        contents += [i.stext for i in p.find_elements(By.XPATH, './i')]
                result[title][name] = contents

        self.browser.quit()
        return result


if __name__ == "__main__":
    scraper = WebScraper()
    print(scraper.get_one_brand_infor('https://prts.wiki/w/%E6%97%B6%E8%A3%85%E5%9B%9E%E5%BB%8A/%E5%BF%92%E6%96%AF%E7%89%B9%E6%94%B6%E8%97%8F'))
