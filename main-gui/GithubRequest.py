import requests


def get_repo_releases(owner, repo):
    url = f"https://api.github.com/repos/{owner}/{repo}/releases"
    response = requests.get(url)
    if response.status_code != 200:
        return None
    releases, release_info = response.json(), []
    for release in releases:
        release_data = {"tag": release["tag_name"], "title": release["name"], "body": release["body"], "assets": []}
        for asset in release.get("assets", []):
            asset_data = {"name": asset["name"], "download_url": asset.get("browser_download_url", "")}
            release_data["assets"].append(asset_data)
        release_info.append(release_data)
    return release_info


if __name__ == "__main__":
    owner = "ETO-QSH"
    repo = "DeskpetETO-download"
    releases = get_repo_releases(owner, repo)
    print(releases)
