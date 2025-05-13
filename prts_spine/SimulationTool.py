from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.support.wait import WebDriverWait
from selenium.webdriver.support.expected_conditions import presence_of_element_located as PEL
from urllib.parse import unquote


class WebScraper:
    def __init__(self):
        chrome_options = Options()
        chrome_options.add_argument('--headless')
        chrome_options.add_argument('--disable-gpu')
        self.browser = webdriver.Chrome(options=chrome_options)

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

    def get_one_agent_infor(self, agent, session, headers):
        self.browser.get(f"https://prts.wiki/w/{agent["name"]}")

        ssw = WebDriverWait(self.browser, 15, 1).until(PEL((By.CLASS_NAME, "skinswitcher-wrapper")))
        div_list = ssw.find_elements(By.XPATH, "./div")

        for div in div_list:
            name = div.find_element(By.CLASS_NAME, 'skinname').get_attribute('textContent')
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


if __name__ == "__main__":
    scraper = WebScraper()
