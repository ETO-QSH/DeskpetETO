import httpx

from PyQt5.QtCore import QObject, pyqtSignal


class GithubWorker(QObject):
    finished = pyqtSignal(bool, list, str)

    def __init__(self, owner, repo):
        super().__init__()
        self.owner = owner
        self.repo = repo

    def run(self):
        try:
            data = get_repo_releases(self.owner, self.repo)
            if data is None:
                self.finished.emit(False, [], "IP被封了喵")
            else:
                self.finished.emit(True, data, "")
        except Exception as e:
            self.finished.emit(False, [], str(e))


def get_repo_releases(owner, repo):
    url = f"https://api.github.com/repos/{owner}/{repo}/releases"
    response = httpx.get(url)
    if response.status_code != 200:
        return None
    releases, release_info = response.json(), []
    for release in releases:
        release_data = {"tag": release["tag_name"], "title": release["name"], "body": release["body"], "assets": []}
        for asset in release.get("assets", []):
            asset_data = {"name": asset["name"], 'size': asset["size"], "download_url": asset.get("browser_download_url", "")}
            release_data["assets"].append(asset_data)
        release_info.append(release_data)
    return release_info


if __name__ == "__main__":
    owner = "ETO-QSH"
    repo = "DeskpetETO-download"
    releases = get_repo_releases(owner, repo)
    print(releases)
