import os
import time
import httpx
import zipfile
from pathlib import Path

from PyQt5.QtCore import Qt, QThread, pyqtSignal
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import QHBoxLayout, QApplication, QWidget

from qfluentwidgets import (
    MessageBoxBase, SubtitleLabel, InfoBar, InfoBarPosition,  ProgressBar, StrongBodyLabel, IndeterminateProgressBar
)

from DeskpetETO.Setting import cfg


class DownloadWorker(QThread):
    """下载线程"""
    progressChanged = pyqtSignal(int)
    speedChanged = pyqtSignal(str)
    timeChanged = pyqtSignal(str)
    downloadFinished = pyqtSignal()
    downloadError = pyqtSignal(str)

    def __init__(self, url, totalSize, fileName, parent=None):
        super().__init__(parent)
        self.url = url
        self.totalSize = totalSize
        self.fileName = fileName
        self._is_running = True

    def run(self):
        try:
            with httpx.stream("GET", self.url, follow_redirects=True) as response:
                response.raise_for_status()
                total_size = self.totalSize
                downloaded_size = 0
                start_time = time.time()

                with open(os.path.join(cfg.get(cfg.downloadFolder), self.fileName), 'wb') as f:
                    for chunk in response.iter_bytes():
                        if not self._is_running:
                            break
                        if chunk:
                            f.write(chunk)
                            downloaded_size += len(chunk)

                            # 计算进度
                            progress = int(100 * downloaded_size / total_size) if total_size > 0 else 0
                            self.progressChanged.emit(progress)

                            # 计算速度
                            elapsed_time = time.time() - start_time
                            speed = downloaded_size / elapsed_time / 1024  # KB/s
                            speed_text = f"{speed:.2f} KB/s"
                            self.speedChanged.emit(speed_text)

                            # 计算剩余时间
                            remaining = (total_size - downloaded_size) / (speed * 1024)
                            mins, secs = divmod(remaining, 60)
                            time_text = f"{int(mins)}分{int(secs)}秒"
                            self.timeChanged.emit(time_text)

                if self._is_running:
                    self.downloadFinished.emit()

        except Exception as e:
            self.downloadError.emit(str(e))

        finally:
            self._is_running = False

    def stop(self):
        self._is_running = False


class CustomDownloadWindow(MessageBoxBase):
    """自定义下载窗口"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.url = None
        self.totalSize = None
        self.fileName = None

        self.downloadThread = None
        self.download_success = False
        self.cancelButton.setVisible(False)

        self.yesButton.setText("取 消 下 载")
        self.yesButton.setFont(QFont('萝莉体', 10))
        self.yesButton.clicked.disconnect()  # 解除父类绑定
        self.yesButton.clicked.connect(self._handle_cancel)  # 绑定到新的取消方法

        # 初始化UI
        self._init_widgets()
        self._setup_layout()

    def _init_widgets(self):
        """初始化组件"""
        self.titleLabel = SubtitleLabel("文件下载")
        self.titleLabel.setAlignment(Qt.AlignCenter)
        self.titleLabel.setFixedSize(240, 30)

        self.speedLabel = StrongBodyLabel("计算中...")
        self.speedLabel.setAlignment(Qt.AlignLeft)

        self.timeLabel = StrongBodyLabel("计算中...")
        self.speedLabel.setAlignment(Qt.AlignRight)

        # 水平容器布局
        self.infoLayout = QHBoxLayout()
        self.infoLayout.addStretch(1)
        self.infoLayout.addWidget(self.speedLabel, 0, Qt.AlignLeft)
        self.infoLayout.addStretch(1)
        self.infoLayout.addWidget(self.timeLabel, 0, Qt.AlignRight)
        self.infoLayout.addStretch(1)

        self.progressBar = ProgressBar()
        self.progressBar.setRange(0, 100)
        self.progressBar.setValue(0)

    def _setup_layout(self):
        """使用父类的布局结构"""
        # 添加组件到内容区域
        self.viewLayout.addWidget(self.titleLabel)
        self.viewLayout.addSpacing(15)
        self.viewLayout.addWidget(self.progressBar)
        self.viewLayout.addSpacing(10)
        self.viewLayout.addLayout(self.infoLayout)

    def setDownloadUrl(self, url, totalSize, fileName):
        """设置下载地址"""
        self.url = url
        self.totalSize = totalSize
        self.fileName = fileName

    def startDownload(self):
        """开始下载"""
        self.downloadThread = DownloadWorker(self.url, self.totalSize, self.fileName)
        self.downloadThread.progressChanged.connect(self.progressBar.setValue)
        self.downloadThread.speedChanged.connect(lambda s: self.speedLabel.setText(f"{s}"))
        self.downloadThread.timeChanged.connect(lambda t: self.timeLabel.setText(f"{t}"))
        self.downloadThread.downloadFinished.connect(self._download_success)
        self.downloadThread.downloadError.connect(self._show_error)
        self.downloadThread.start()
        self.exec_()
        return self.download_success

    def _handle_cancel(self):
        """处理取消下载"""
        if self.downloadThread and self.downloadThread.isRunning():
            self.downloadThread.stop()
            self.downloadThread.quit()
            self.downloadThread.wait(3000)

        InfoBar.warning(
            title="下载已取消",
            content="用户中断了下载过程",
            parent=self.parent(),
            duration=3000,
            position=InfoBarPosition.TOP
        )

        super().reject()

    def _download_success(self):
        """下载成功处理"""
        self.accept()

        InfoBar.success(
            title="下载完成",
            content="文件已保存到指定目录",
            parent=self.parent(),
            duration=3000,
            position=InfoBarPosition.TOP
        )

        zip_path = os.path.join(cfg.get(cfg.downloadFolder), self.fileName)

        # 显示解压窗口
        unzip_window = UnzipWindow(self.parent())
        if res := unzip_window.start_unzip(zip_path):  # 自动开始解压
            os.remove(zip_path)
        self.download_success = res

    def _show_error(self, message):
        """显示错误信息"""
        InfoBar.error(
            title="下载错误",
            content=message,
            parent=self.parent(),
            duration=3000,
            position=InfoBarPosition.TOP
        )
        self.reject()


class UnzipWorker(QThread):
    finished = pyqtSignal(bool, str)

    def __init__(self, zip_path, parent=None):
        super().__init__(parent)
        self.zip_path = Path(zip_path)
        self.extract_dir = str(self.zip_path.parent)

    def run(self):
        try:
            extract_path = Path(self.extract_dir)
            extract_path.mkdir(parents=True, exist_ok=True)

            with zipfile.ZipFile(self.zip_path, 'r') as zip_ref:
                for file in zip_ref.infolist():
                    decoded_name = file.filename.encode('cp437').decode('gbk')
                    target_path = extract_path / decoded_name

                    # 创建目录（如果是目录条目）
                    if decoded_name.endswith('/'):
                        target_path.mkdir(parents=True, exist_ok=True)
                        continue

                    # 确保父目录存在
                    target_path.parent.mkdir(parents=True, exist_ok=True)

                    # 写入文件内容
                    with open(target_path, 'wb') as f:
                        f.write(zip_ref.read(file.filename))

            self.finished.emit(True, f"文件解压完成")

        except zipfile.BadZipFile:
            self.finished.emit(False, "压缩文件损坏或格式不正确")

        except PermissionError:
            self.finished.emit(False, "没有写入权限，请检查目录权限")

        except Exception as e:
            self.finished.emit(False, f"解压失败：{str(e)}")


class UnzipWindow(MessageBoxBase):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("文件解压中")
        self.setWindowFlags(self.windowFlags() & ~Qt.WindowCloseButtonHint)  # 禁用关闭按钮

        self.unzip_success = False  # 返回状态

        # 隐藏所有按钮
        self.yesButton.setVisible(False)
        self.cancelButton.setVisible(False)
        self.buttonGroup.setVisible(False)

        # 创建界面组件
        self.titleLabel = SubtitleLabel("正在解压")
        self.titleLabel.setAlignment(Qt.AlignCenter)
        self.titleLabel.setFixedSize(240, 30)

        # 不知名进度条
        self.progressBar = IndeterminateProgressBar(self)

        # 设置布局
        self.viewLayout.addWidget(self.titleLabel)
        self.viewLayout.addSpacing(15)
        self.viewLayout.addWidget(self.progressBar)
        self.viewLayout.addSpacing(10)

    def start_unzip(self, zip_path):
        """启动解压流程"""
        self.worker = UnzipWorker(zip_path)
        self.worker.finished.connect(self._handle_result)
        self.worker.start()
        self.exec_()
        return self.unzip_success

    def _handle_result(self, success, message):
        """处理解压结果"""
        if success:
            self.accept()
            InfoBar.success(
                title="解压完成",
                content=message,
                parent=self.parent(),
                duration=3000,
                position=InfoBarPosition.TOP
            )
            self.unzip_success = True
        else:
            self.reject()
            InfoBar.error(
                title="解压失败",
                content=message,
                parent=self.parent(),
                duration=3000,
                position=InfoBarPosition.TOP
            )


def show_download_dialog(parent, url, totalSize, fileName):
    """显示下载对话框"""
    window = CustomDownloadWindow(parent)
    window.setDownloadUrl(url, totalSize, fileName)
    return window.startDownload()


if __name__ == "__main__":
    import sys

    app = QApplication(sys.argv)

    # 测试下载链接（替换为实际有效的URL）
    test_url = "https://github.com/ETO-QSH/DeskpetETO-download/releases/download/2025.5.2/DeskpetETO.zip"

    # 创建主窗口
    main_window = QWidget()
    main_window.setWindowTitle("下载测试")
    main_window.resize(400, 300)
    main_window.show()

    # 显示下载窗口
    show_download_dialog(main_window, test_url, 170655456, "DeskpetETO.zip")

    sys.exit(app.exec_())
