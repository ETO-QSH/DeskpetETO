import os
import shutil
import subprocess
from pathlib import Path

from PyQt5.QtCore import pyqtSignal, QThread


class SpinePreviewThread(QThread):
    """ 骨骼预览生成线程 """
    finished = pyqtSignal(int, str)  # (状态码, 结果路径)

    def __init__(self, skel_path, atlas_path, png_path):
        super().__init__()
        self.skel_path = skel_path
        self.atlas_path = atlas_path
        self.png_path = png_path

    def run(self):
        try:
            os.makedirs("./output/temp", exist_ok=True)
            path = os.path.join("./output/temp", Path(self.skel_path).stem)

            files = []
            for file, suffix in [(self.skel_path, ".skel"), (self.atlas_path, ".atlas"), (self.png_path, ".png")]:
                shutil.copyfile(file, path + suffix)
                files.append(path + suffix)

            result = subprocess.run(["./preview/preview.exe", *files[:2], path + "_output.png"], capture_output=True, text=True)

            for file in files:
                os.remove(file)

            if result.returncode == 0:
                self.finished.emit(0, path + "_output.png" + "%%%" + result.stdout)
            else:
                self.finished.emit(result.returncode, "模型解析失败")

        except Exception as e:
            self.finished.emit(-1, str(e))
            print(e)


def spine_preview(skel, atlas, png):
    os.makedirs("./output/temp", exist_ok=True)
    path = os.path.join("./output/temp", Path(skel).stem)

    files = []
    for file, suffix in [(skel, ".skel"), (atlas, ".atlas"), (png, ".png")]:
        shutil.copyfile(file, path + suffix)
        files.append(path + suffix)

    result = subprocess.run(["./preview/preview.exe", *files[:2], path + "_output.png"], capture_output=True, text=True)

    for file in files:
        os.remove(file)

    if result.returncode == 0:
        return 0, path + "_output.png"
    else:
        return result.returncode, "模型解析失败"
